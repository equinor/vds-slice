import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendRandomSliceRequest } from "./helpers/slice-helpers.js";
import {
  createSummary,
  thresholds,
  summaryTrendStats,
} from "./helpers/report-helpers.js";
import { sleep } from "k6";

export const options = {
  scenarios: {
    randomInlineSliceWait: {
      executor: "constant-vus",
      vus: 1,
      duration: "20m",
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
  sleep(150);
}

export function handleSummary(data) {
  return createSummary(data);
}
