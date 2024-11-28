import { sendSetupRequest } from "./request-helpers.js";

export function getMetadataPayload() {
  const vds = __ENV.VDS;
  const sas = __ENV.SAS;
  const payload = {
    vds: vds,
    sas: sas,
  };
  return payload;
}

export function sendSetupMetadataRequest() {
  const payload = getMetadataPayload();
  const metadata = sendSetupRequest("metadata", payload);
  return JSON.parse(metadata.body);
}

export function getStepsize(axis) {
  return (axis.max - axis.min) / (axis.samples - 1);
}
