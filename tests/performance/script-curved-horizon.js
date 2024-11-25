import {
  sendSetupMetadataRequest,
  getStepsize
} from "./helpers/metadata-helpers.js";
import {
  sendCurvedHorizonRequest,
  retrieveMetadataForHorizonRequest,
} from "./helpers/horizon-helpers.js";
import {
  createSummary,
  thresholds,
  summaryTrendStats,
} from "./helpers/report-helpers.js";
import { sleep } from "k6";

const duration = __ENV.SCRIPT_DURATION || "1m";

const SCENARIO = __ENV.SCENARIO || 'curvedHorizonDefault';
const scenarios = {
  curvedHorizonDefault: {
    exec: "curvedHorizonDefault",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
  curvedHorizon75: {
    exec: "curvedHorizon75",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
  curvedHorizonTenthStepsize: {
    exec: "curvedHorizonTenthStepsize",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
};

export const options = {
  scenarios: {},
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};
options.scenarios[SCENARIO] = scenarios[SCENARIO];

export function setup() {
  const metadata = sendSetupMetadataRequest();
  return retrieveMetadataForHorizonRequest(metadata);
}

function horizonRequest(metadata, resamplingStepsize=null, halfVerticalSamples=50) {
  const [surface, inlineAxis, xlineAxis, sampleAxis] = metadata;
  const sampleAxisStepSize = getStepsize(sampleAxis);

  if (resamplingStepsize === null) {
    resamplingStepsize = sampleAxisStepSize;
  }

  const depthMiddle =
    Math.floor(sampleAxis.samples / 2) * sampleAxisStepSize + sampleAxis.min;

  // We assume that for curved horizon difference between min and max point could be
  // up to several km, regardless of the data stepsize, which is usually
  // 2-5 units. However our testdata is often created with stepsize=1. So 2 *
  // samplesDifference * realStepsize should roughly correspond to this "several
  // kilometers" case for any real sample axis step size.
  const samplesDifference = 400
  const depthMin = depthMiddle - samplesDifference * sampleAxisStepSize;
  const depthMax = depthMiddle + samplesDifference * sampleAxisStepSize;

  // We assume that for curved horizon above+below (or distance between
  // horizons) is ranging from several hundred meters to 1 km, regardless of the
  // data stepsize, which is usually 2-5 units. For us number of samples that
  // needs to be downloaded is important. So provided values for above/below
  // should roughly correspond to that number even for stepsize=1.
  const above = halfVerticalSamples * sampleAxisStepSize;
  const below = halfVerticalSamples * sampleAxisStepSize;

  if (depthMin - above < sampleAxis.min) {
    throw new Error("Adjust the script: minimum depth value out of bounds.");
  }

  if (depthMax + below > sampleAxis.max) {
    throw new Error("Adjust the script: maximum depth value out of bounds.");
  }

  const stepsize = resamplingStepsize;
  const attributes = ["min", "max", "rms", "var"]
  sendCurvedHorizonRequest(
    inlineAxis.samples,
    xlineAxis.samples,
    depthMin,
    depthMax,
    surface,
    below,
    above,
    stepsize,
    attributes
  );

  let sleepSeconds = __ENV.ITERATION_SLEEP_SECONDS || 0;
  sleep(sleepSeconds);
}

export function curvedHorizonDefault(metadata) {
  horizonRequest(metadata);
}

export function curvedHorizon75(metadata) {
  horizonRequest(metadata, null, 75);
}

export function curvedHorizonTenthStepsize(metadata) {
  const [, , , sampleAxis] = metadata;
  const sampleAxisStepSize = getStepsize(sampleAxis);

  // tenth of stepsize is a reasonable resampling value
  const resamplingStepsize = sampleAxisStepSize / 10;
  horizonRequest(metadata, resamplingStepsize);
}

export function handleSummary(data) {
  return createSummary(data);
}
