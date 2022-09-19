import http from "k6/http";
import { check, fail, sleep } from "k6";
import { Trend } from "k6/metrics";

const responseLengthTrend = new Trend("response_length");
const requestTimeTrend = new Trend("request_time", true);

export function getRandomInt(max) {
  return Math.floor(Math.random() * max);
}

/**
 * Return max (exclusive) dimension index aka number of samples
 */
export function getMaxIndexInDimension(metadata, annotationDimension) {
  const axis = metadata.axis.find((ax) => {
    return ax.annotation == annotationDimension;
  });
  return axis.samples;
}

export function getRandomIndexInDimension(metadata, annotationDimension) {
  return getRandomInt(getMaxIndexInDimension(metadata, annotationDimension));
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
    { "status checks": "query" }
  );
  if (!queryResStatusCheck) {
    fail(`Wrong 'query' response status: ${res.status}`);
  }
  responseLengthTrend.add(res.body.byteLength);
  requestTimeTrend.add(res.timings.duration);
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
