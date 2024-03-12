import { getRandomIndexInDimension } from "./helpers/request-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendSequentialFenceRequest } from "./helpers/fence-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    randomFence: {
      executor: "constant-vus",
      vus: 1,
      duration: "30s",
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  const metadata = sendSetupMetadataRequest();
  return metadata;
}

export default function (metadata) {
  const il1 = getRandomIndexInDimension(metadata, "Inline");
  const il2 = getRandomIndexInDimension(metadata, "Inline");
  const xl1 = getRandomIndexInDimension(metadata, "Crossline");
  const xl2 = getRandomIndexInDimension(metadata, "Crossline");
  sendSequentialFenceRequest(il1, il2, xl1, xl2);
}

export function handleSummary(data) {
  return createSummary(data);
}
