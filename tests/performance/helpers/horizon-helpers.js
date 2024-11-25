import {
  sendRequest,
  getAxisInDimension,
  createEquidistantPoints
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
 * Creates horizon request of size rows/columns where points create a continuous
 * depth plane.
 */
function createFlatIncreasingHorizon(
  rows, columns, depthMin, depthMax
) {
  function makeNoise(weight) {
    return (Math.random() - 0.5) * weight;
  }

  let points = createEquidistantPoints(depthMin, depthMax, rows + columns - 1, false)
  let values = Array.from({ length: rows }, (_, i) =>
    Array.from({ length: columns }, (_, j) => points[i + j] + makeNoise(1))
  );
  return values
}


export function sendFlatIncreasingHorizonRequest(
  rows, columns, depthMin, depthMax, surface, above, below, stepsize, attributes
) {
  let values = createFlatIncreasingHorizon(rows, columns, depthMin, depthMax)
  console.log(`Requesting horizon of size ${columns * rows}`);
  return sendHorizonRequest(values, surface, above, below, stepsize, attributes);
}

