import {
  getRandomIndexInDimension,
  convertDimension,
} from "./helpers/request-helpers.js";
import { sendSliceRequest } from "./helpers/slice-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    constantSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: "30s",
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  const annotatedDimension = "Sample";
  const metadata = sendSetupMetadataRequest();
  const index = getRandomIndexInDimension(metadata, annotatedDimension);
  const indexDimension = convertDimension(annotatedDimension);
  return [index, indexDimension];
}

export default function (params) {
  const [index, indexDimension] = params;
  sendSliceRequest(indexDimension, index);
}

export function handleSummary(data) {
  return createSummary(data);
}
