import {
  getRandomIndexInDimension,
  getSampleSizeInDimension,
  convertDimension,
} from "./helpers/request-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendSliceRequest } from "./helpers/slice-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";

import exec from "k6/execution";

export const options = {
  scenarios: {
    sequentialInlineSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: "2m",
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  const annotatedDimension = "Inline";
  const metadata = sendSetupMetadataRequest();
  const startIndex = getRandomIndexInDimension(metadata, annotatedDimension);
  const indexDimension = convertDimension(annotatedDimension);
  const samples = getSampleSizeInDimension(metadata, annotatedDimension);
  return [startIndex, samples, indexDimension];
}

export default function (params) {
  const [startIndex, samples, indexDimension] = params;
  const index = (startIndex + exec.scenario.iterationInInstance) % samples;
  sendSliceRequest(indexDimension, index);
}

export function handleSummary(data) {
  return createSummary(data);
}
