import { textSummary } from "https://jslib.k6.io/k6-summary/0.0.1/index.js";
import * as metrics from "./metrics-helper.js";

const basicThresholds = {
  "checks{status checks:query}": [{ threshold: "rate == 1.00" }],
};

export function thresholds() {
  // note: if med limit fails, run fails, but threshold misleadingly shows green
  let thresholds = basicThresholds;
  let request_time_thresholds = [
    `med < ${metrics.request_time_med_threshold()}`,
    `p(95) < ${metrics.request_time_p95_threshold()}`,
  ];
  thresholds[metrics.requestTimeTrend.name] = request_time_thresholds;
  return thresholds;
}

export function summaryTrendStats() {
  return ["avg", "min", "med", "max", "p(95)", "p(99)", "count"];
}

export function createSummary(data) {
  const logpath = __ENV.LOGPATH;
  if (logpath) {
    const logname = `${logpath}/summary.json`;
    return {
      stdout: textSummary(data, { indent: " ", enableColors: false }),
      [logname]: JSON.stringify(data),
    };
  } else {
    return {
      stdout: textSummary(data, { indent: " " }),
    };
  }
}
