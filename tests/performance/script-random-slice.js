import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendRandomSliceRequest } from "./helpers/slice-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";
import { sleep } from "k6";

const duration = __ENV.SCRIPT_DURATION || "2m";

const SCENARIO = __ENV.SCENARIO || 'randomInlineSlice';
const scenarios = {
  randomInlineSlice: {
    exec: "inlineSlice",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
  randomXlineSlice: {
    exec: "xlineSlice",
    executor: "constant-vus",
    vus: 1,
    duration: duration,
  },
  randomSampleSlice: {
    exec: "sampleSlice",
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
  return sendSetupMetadataRequest();
}

function sliceRequest(type, metadata) {
  sendRandomSliceRequest(metadata, type);
  let sleepSeconds = __ENV.ITERATION_SLEEP_SECONDS || 0;
  sleep(sleepSeconds);
}

export function inlineSlice(metadata) {
  sliceRequest("Inline", metadata);
}

export function xlineSlice(metadata) {
  sliceRequest("Crossline", metadata);
}

export function sampleSlice(metadata) {
  sliceRequest("Sample", metadata);
}

export function handleSummary(data) {
  return createSummary(data);
}
