import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import {
  sendRandomHorizonRequest,
  retrieveMetadataForHorizonRequest,
} from "./helpers/horizon-helpers.js";
import {
  createSummary,
  thresholds,
  summaryTrendStats,
} from "./helpers/report-helpers.js";

export const options = {
  scenarios: {
    randomHorizon: {
      executor: "constant-vus",
      vus: 1,
      duration: "3m",
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
  const depth = [sampleAxis.min, sampleAxis.max, sampleAxisStride];
  const attributes = ["samplevalue", "min", "max", "mean", "rms", "sd"]
  const above = 20;
  const below = 20;
  const stepsize = 0.1;
  sendRandomHorizonRequest(
    inlineAxis.samples,
    xlineAxis.samples,
    depth,
    surface,
    above,
    below,
    stepsize,
    attributes
  );
}

export function handleSummary(data) {
  return createSummary(data);
}
