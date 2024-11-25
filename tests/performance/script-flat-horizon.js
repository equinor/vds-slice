import {
  sendSetupMetadataRequest,
  getStepsize
} from "./helpers/metadata-helpers.js";
import {
  sendFlatIncreasingHorizonRequest,
  retrieveMetadataForHorizonRequest,
} from "./helpers/horizon-helpers.js";
import {
  createSummary,
  thresholds,
  summaryTrendStats,
} from "./helpers/report-helpers.js";
import { sleep } from "k6";

const duration = __ENV.SCRIPT_DURATION || "90s";

const SCENARIO = __ENV.SCENARIO || 'flatHorizonDefault';
const scenarios = {
  flatHorizonDefault: {
    exec: "flatHorizonDefault",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
  flatHorizonTenthStepsize: {
    exec: "flatHorizonTenthStepsize",
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

function horizonRequest(metadata, resamplingStepsize = null) {
  const [surface, inlineAxis, xlineAxis, sampleAxis] = metadata;
  const sampleAxisStepSize = getStepsize(sampleAxis);

  if (resamplingStepsize === null) {
    resamplingStepsize = sampleAxisStepSize;
  }

  const depthMiddle =
    Math.floor(sampleAxis.samples / 2) * sampleAxisStepSize + sampleAxis.min;

  // We assume that for flat horizon difference between min and max point is
  // several hundred meters, regardless of the data stepsize, which is usually
  // 2-5 units. However our testdata is often created with stepsize=1. So 2 *
  // samplesDifference * realStepsize should roughly correspond to this "several
  // hundred meters" case for any real sample axis step size.
  const samplesDifference = 75
  const depthMin = depthMiddle - samplesDifference * sampleAxisStepSize;
  const depthMax = depthMiddle + samplesDifference * sampleAxisStepSize;

  // We assume that for flat horizon above+below is in range [50-100],
  // regardless of the data stepsize, which is usually 2-5 units. For us number
  // of samples that needs to be downloaded is important. So provided values for
  // above/below should roughly correspond to that number even for stepsize=1.
  const above = 10 * sampleAxisStepSize;
  const below = 10 * sampleAxisStepSize;

  if (depthMin - above < sampleAxis.min) {
    throw new Error("Adjust the script: minimum depth value out of bounds.");
  }

  if (depthMax + below > sampleAxis.max) {
    throw new Error("Adjust the script: maximum depth value out of bounds.");
  }

  const stepsize = resamplingStepsize;
  const attributes = ["min", "max", "rms", "var"]
  sendFlatIncreasingHorizonRequest(
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

export function flatHorizonDefault(metadata) {
  horizonRequest(metadata);
}

export function flatHorizonTenthStepsize(metadata) {
  const [, , , sampleAxis] = metadata;
  const sampleAxisStepSize = getStepsize(sampleAxis);

  // tenth of stepsize is a reasonable resampling value
  const resamplingStepsize = sampleAxisStepSize / 10;
  horizonRequest(metadata, resamplingStepsize);
}

export function handleSummary(data) {
  return createSummary(data);
}
