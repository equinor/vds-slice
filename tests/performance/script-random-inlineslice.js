import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendRandomSliceRequest } from "./helpers/slice-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    randomInlineSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: "2m",
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  return sendSetupMetadataRequest();
}

export default function (metadata) {
  sendRandomSliceRequest(metadata, "Inline");
}

export function handleSummary(data) {
  return createSummary(data);
}
