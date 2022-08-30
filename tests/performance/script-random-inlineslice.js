import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendRandomSliceRequest } from "./helpers/slice-helpers.js";
import { createSummary, thresholds } from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    constantSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: "2m",
    },
  },
  thresholds: thresholds(),
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
