import { textSummary } from "https://jslib.k6.io/k6-summary/0.0.1/index.js";

const basicThresholds = {
  "checks{status checks:query}": [{ threshold: "rate == 1.00" }],
};

export function thresholds(defaultMed=30000, defaultMax=60000) {
  let medTime = __ENV.MEDTIME;
  medTime = medTime ? medTime : defaultMed
  let maxTime = __ENV.MAXTIME;
  maxTime = maxTime ? maxTime : defaultMax

  // note: if med limit fails, run fails, but threshold misleadingly shows green
  let thresholds = basicThresholds
  thresholds['iteration_duration'] = [`med < ${medTime}`, `p(95) < ${maxTime}`]
  return thresholds
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
      stdout: textSummary(data, { indent: " "}),
    };
  }
}
