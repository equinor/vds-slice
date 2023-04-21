import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import {
  sendConstantHorizonRequest,
  retrieveMetadataForHorizonRequest,
} from "./helpers/horizon-helpers.js";
import {
  createSummary,
  thresholds,
  summaryTrendStats,
} from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    constantHorizon: {
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
  return retrieveMetadataForHorizonRequest(metadata);
}

export default function (params) {
  const [surface, inlineAxis, xlineAxis, sampleAxis] = params;
  const sampleAxisStride =
    (sampleAxis.max - sampleAxis.min) / (sampleAxis.samples - 1);
  const depth =
    Math.floor(sampleAxis.samples / 2) * sampleAxisStride + sampleAxis.min;
  const above = 10;
  const below = 10;
  const stepsize = 1;
  const attributes = ["samplevalue"]
  sendConstantHorizonRequest(
    inlineAxis.samples,
    xlineAxis.samples,
    depth,
    surface,
    below,
    above,
    stepsize,
    attributes
  );
}

export function handleSummary(data) {
  return createSummary(data);
}
