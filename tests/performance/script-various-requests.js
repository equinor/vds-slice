import { sendRandomSliceRequest } from "./helpers/slice-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    ilineSlice: {
      exec: "randomIlineSliceRequest",
      executor: "ramping-vus",
      startVUs: 0,
      stages: [
        { duration: "1m", target: 3 },
        { duration: "3m", target: 3 },
      ],
    },

    xlineSlice: {
      exec: "randomXlineSliceRequest",
      executor: "ramping-vus",
      startVUs: 0,
      stages: [
        { duration: "50s", target: 3 },
        { duration: "3m", target: 3 },
      ],
      startTime: "10s",
    },

    timeSlice: {
      exec: "randomTimeSliceRequest",
      executor: "ramping-vus",
      startVUs: 0,
      stages: [
        { duration: "30s", target: 3 },
        { duration: "3m", target: 3 },
      ],
      startTime: "30s",
    },
  },

  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  return sendSetupMetadataRequest();
}

export function randomIlineSliceRequest(metadata) {
  sendRandomSliceRequest(metadata, "Inline");
}

export function randomXlineSliceRequest(metadata) {
  sendRandomSliceRequest(metadata, "Crossline");
}

export function randomTimeSliceRequest(metadata) {
  sendRandomSliceRequest(metadata, "Sample");
}

export function handleSummary(data) {
  return createSummary(data);
}
