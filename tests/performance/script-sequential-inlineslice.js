import {
  getRandomIndexInDimension,
  getSampleSizeInDimension,
  convertDimension,
} from "./helpers/request-helpers.js";
import { sendSetupMetadataRequest } from "./helpers/metadata-helpers.js";
import { sendSliceRequest } from "./helpers/slice-helpers.js";
import { createSummary, thresholds, summaryTrendStats } from "./helpers/report-helpers.js";
import { sleep } from "k6";

import exec from "k6/execution";

/*
 Scenario can be useful for investigations:
 1. Pool handle - would it make sense to store DataHandles in memory and thus
    benefit from openvds caching already downloaded blobs.
 2. Effects Azure blob "warming" has on response time.

 In both cases comparison between "sequential" and "random" would be of
    interest.
*/

const duration = __ENV.SCRIPT_DURATION || "2m";

export const options = {
  scenarios: {
    sequentialInlineSlice: {
      executor: "constant-vus",
      vus: 1,
      duration: duration,
    },
  },
  thresholds: thresholds(),
  summaryTrendStats: summaryTrendStats(),
};

export function setup() {
  const annotatedDimension = "Inline";
  const metadata = sendSetupMetadataRequest();
  const startIndex = getRandomIndexInDimension(metadata, annotatedDimension);
  const indexDimension = convertDimension(annotatedDimension);
  const samples = getSampleSizeInDimension(metadata, annotatedDimension);
  return [startIndex, samples, indexDimension];
}

export default function (params) {
  const [startIndex, samples, indexDimension] = params;
  const index = (startIndex + exec.scenario.iterationInInstance) % samples;
  sendSliceRequest(indexDimension, index);

  let sleepSeconds = __ENV.ITERATION_SLEEP_SECONDS || 0;
  sleep(sleepSeconds);
}

export function handleSummary(data) {
  return createSummary(data);
}
