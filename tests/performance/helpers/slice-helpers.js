import {
  sendRequest,
  getRandomIndexInDimension,
  convertDimension,
} from "./request-helpers.js";

export function sendSliceRequest(direction, lineno) {
  const vds = __ENV.VDS;
  const sas = __ENV.SAS;
  let payload = {
    direction: direction,
    lineno: lineno,
    vds: vds,
    sas: sas,
  };
  console.log(`Requesting slice by line ${lineno} in direction ${direction}`);
  sendRequest("slice", payload);
}

export function sendRandomSliceRequest(metadata, annotatedDimension) {
  const index = getRandomIndexInDimension(metadata, annotatedDimension);
  const indexDimension = convertDimension(annotatedDimension);
  return sendSliceRequest(indexDimension, index);
}
