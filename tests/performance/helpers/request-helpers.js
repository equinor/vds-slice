import http from "k6/http";
import { check, fail, sleep } from "k6";
import * as metrics from "./metrics-helper.js";

export function getAxisInDimension(metadata, annotationDimension) {
  const axis = metadata.axis.find((ax) => {
    return ax.annotation == annotationDimension;
  });
  return axis;
}

/**
 * Return number of samples in dimension
 */
export function getSampleSizeInDimension(metadata, annotationDimension) {
  return getAxisInDimension(metadata, annotationDimension).samples;
}

/**
 * Return max (inclusive) dimension index aka number of samples - 1.
 * Function does not return maximum value of annotated dimension,
 * only corresponding index dimension.
 */
export function getMaxIndexInDimension(metadata, annotationDimension) {
  return getSampleSizeInDimension(metadata, annotationDimension) - 1;
}

/**
 * Return random value in range [min : max : step].
 * Min and max are inclusive. (max - min) is expected to be divisible by step.
 */
export function getRandom(min, max, step) {
  const samples = (max + step - min) / step;
  const random = Math.floor(Math.random() * samples);
  return min + random * step;
}

/**
 * Return random index in dimension (int in range [0, number_or_samples])
 */
export function getRandomIndexInDimension(metadata, annotationDimension) {
  return getRandom(0, getMaxIndexInDimension(metadata, annotationDimension), 1);
}

/**
 * Creates set of segmentCount + 1 points that reach from start to end, may be
 * integers and are relatively equally spaced.
 */
export function createEquidistantPoints(start, end, segmentCount, roundToSample = true, sampleStep = 1) {
  const length = end - start;
  const segment = length / segmentCount;
  var points = [];
  for (let i = 0; i <= segmentCount; i++) {
    let position = start + i * segment;
    if (roundToSample) {
      position = Math.round((position - start) / sampleStep) * sampleStep + start;
    }
    points.push(position);
  }
  return points;
}

export function convertDimension(dimension) {
  let converted;
  switch (dimension) {
    case "Inline":
      converted = "I";
      break;
    case "Crossline":
      converted = "J";
      break;
    case "Time":
    case "Sample":
      converted = "K";
      break;
    default:
      throw `Dimension ${dimension} is unhandled`;
  }
  return converted;
}

export function sendRequest(path, payload) {
  const endpoint = __ENV.ENDPOINT.replace(/\/$/, "");
  const url = `${endpoint}/${path}`;

  const options = {
    headers: { "Content-Type": "application/json", "Accept-Encoding": "gzip" },
    responseType: "binary",
  };
  const res = http.post(url, JSON.stringify(payload), options);

  const queryResStatusCheck = check(
    res,
    {
      "query response: status must be 200": (r) => r.status === 200,
    },
    { "status_checks": "query" }
  );
  if (!queryResStatusCheck) {
    fail(`Wrong 'query' response status: ${res.status}`);
  }
  metrics.responseLengthTrend.add(res.body.byteLength);
  metrics.requestTimeTrend.add(res.timings.duration);

  // artificially wait after each request to create a sense of processing-delay
  sleep(0.1);
}

export function sendSetupRequest(path, payload) {
  const endpoint = __ENV.ENDPOINT.replace(/\/$/, "");
  const url = `${endpoint}/${path}`;
  const options = {
    headers: { "Content-Type": "application/json" },
    responseType: "text",
  };
  const res = http.post(url, JSON.stringify(payload), options);
  if (res.status != 200) {
    fail(`Unexpected setup request status: ${res.status}, error: ${res.body}`);
  }
  return res;
}
