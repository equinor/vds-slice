import { getMaxIndexInDimension } from "./helpers/request-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendSequentialFenceRequest } from "./helpers/fence-helpers.js";
import { createSummary, thresholds } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    constantSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: "1m",
    },
  },
  thresholds: thresholds(),
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
}

export function handleSummary(data) {
  return createSummary(data);
}
