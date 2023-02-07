import { Trend } from "k6/metrics";

export const responseLengthTrend = new Trend("response_length");
export const requestTimeTrend = new Trend(
  `request_time_med_${request_time_med_threshold()}_p95_${request_time_p95_threshold()}`,
  true
);

export function request_time_med_threshold() {
  let medTime = __ENV.MEDTIME;
  medTime = medTime ? medTime : 30000;
  return medTime;
}

export function request_time_p95_threshold() {
  // keep referring to p95 as "maxtime" until we figure out what is useful here (if something is)
  let maxTime = __ENV.MAXTIME;
  maxTime = maxTime ? maxTime : 60000;
  return maxTime;
}
