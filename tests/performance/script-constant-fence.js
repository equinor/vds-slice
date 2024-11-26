import { getMaxIndexInDimension } from "./helpers/request-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendSequentialFenceRequest } from "./helpers/fence-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";
import { sleep } from "k6";

const duration = __ENV.SCRIPT_DURATION || "1m";

export const options = {
  scenarios: {
    constantFence: {
      executor: "constant-vus",
      vus: 1,
      duration: duration,
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};
export function setup() {
  const metadata = sendSetupMetadataRequest();
  const maxInline = getMaxIndexInDimension(metadata, "Inline");
  const maxCrossline = getMaxIndexInDimension(metadata, "Crossline");
  return [maxInline, maxCrossline];
}
export default function (params) {
  const [maxInline, maxCrossline] = params;
  sendSequentialFenceRequest(0, maxInline, 0, maxCrossline);

  let sleepSeconds = __ENV.ITERATION_SLEEP_SECONDS || 0;
  sleep(sleepSeconds);
}

export function handleSummary(data) {
  return createSummary(data);
}
