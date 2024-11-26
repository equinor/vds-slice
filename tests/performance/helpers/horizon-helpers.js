import {
  sendRequest,
  getAxisInDimension,
} from "./request-helpers.js";

export function sendHorizonRequest(
  values, surface, above, below, stepsize, attributes
) {
  const vds = __ENV.VDS;
  const sas = __ENV.SAS;

  const surfaceProperties = {
    fillValue: -999.25,
    values: values,
  }

  const regularSurface = Object.assign({}, surfaceProperties, surface);

  let properties = {
    surface : regularSurface,
    interpolation: "linear",
    vds: vds,
    sas: sas,
    above: below,
    below: above,
    stepsize: stepsize,
    attributes: attributes
  };
  let payload = Object.assign({}, properties, surface);
  sendRequest("attributes/surface/along", payload);
}

export function retrieveMetadataForHorizonRequest(metadata) {
  const inlineAxis = getAxisInDimension(metadata, "Inline");
  const xlineAxis = getAxisInDimension(metadata, "Crossline");
  const samplesAxis = getAxisInDimension(metadata, "Sample");

  const cdp = metadata.boundingBox.cdp;

  const ilineStepX = (cdp[1][0] - cdp[0][0]) / (inlineAxis.samples - 1);
  const ilineStepY = (cdp[1][1] - cdp[0][1]) / (inlineAxis.samples - 1);
  const xlineStepX = (cdp[3][0] - cdp[0][0]) / (xlineAxis.samples - 1);
  const xlineStepY = (cdp[3][1] - cdp[0][1]) / (xlineAxis.samples - 1);

  const rotation = (Math.atan2(ilineStepY, ilineStepX) * 180) / Math.PI;
  const xinc = Math.hypot(ilineStepX, ilineStepY);
  const yinc = Math.hypot(xlineStepX, xlineStepY);

  let surface = {
    xinc: xinc,
    yinc: yinc,
    xori: cdp[0][0],
    yori: cdp[0][1],
    rotation: rotation,
  };
  return [surface, inlineAxis, xlineAxis, samplesAxis];
}

/**
 * Sends horizon request of size rows/columns where every point is at provided depth.
 */
export function sendConstantHorizonRequest(
  rows, columns, depth, surface, above, below, stepsize, attributes
){
  let rowArray = new Array(columns).fill(depth);
  let values = new Array(rows).fill(rowArray);
  console.log(`Requesting horizon of size ${columns * rows}`);
  return sendHorizonRequest(values, surface, above, below, stepsize, attributes);
}

