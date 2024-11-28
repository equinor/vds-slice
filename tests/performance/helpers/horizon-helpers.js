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


/**
* Creates a synthetic curved seismic surface.
*
* All hard-coded values are mostly chosen such that surface looked somewhat
* similar to real uneven surfaces.
*
* @param {number} rows - Number of rows in the grid.
* @param {number} columns - Number of columns in the grid.
* @param {number} depthMin - Approximate minimum depth value of the horizon.
* @param {number} depthMax - Approximate maximum depth value of the horizon.
* @returns {Array<Array<number>>} - 2D array representing the surface.
*/
function createCurvedHorizon(rows, columns, depthMin, depthMax) {
  /**
   * Calculates all parameters needed for sinus equation.
   *
   * Idea is to create sinus parameters for each x-value such that:
   *  - parameters for each half-period are the same to get a nice curve
   *  - period and amplitude values are adjusted after half-period has passed
   *  - with bigger x-values function becomes flatter than at the smaller ones
   *
   * @param {number} nValues Number of x-values in sinus.
   * @param {number} halfPeriodMax Maximum distance of half-period (value at index 0).
   * @param {number} halfPeriodFactor Reduction factor for the next half-period
   * segment: nextHalfPeriod = currentHalfPeriod / halfPeriodFactor
   * @param {number} amplitudeMax Maximum value for the amplitude (value at index 0).
   * @param {number} amplitudeFactor Reduction factor for the amplitude in the
   * next period: nextAmplitude = currentAmplitude / amplitudeFactor
   */
  function calculateParameters(nValues, halfPeriodMax, halfPeriodFactor, amplitudeMax, amplitudeFactor) {
    const periods = Array(nValues);
    const amplitudes = Array(nValues);
    const phases = Array(nValues);

    // half-periods must be integers so after n=half-period indices we would end up exactly at zero
    let halfPeriod = Math.floor(halfPeriodMax)
    let amplitude = amplitudeMax;
    let phase = 0;

    let currentIndex = 0;
    let nHalfPeriods = 0;

    const minPeriod = 10;
    const minAmplitude = 0.1;

    while (currentIndex < nValues) {
      const nextIndex = currentIndex + halfPeriod;
      for (let index = currentIndex; index < Math.min(nValues, nextIndex); index++) {
        periods[index] = 2 * halfPeriod;
        amplitudes[index] = amplitude;
        phases[index] = phase;
      }
      ++nHalfPeriods;
      halfPeriod = Math.ceil(Math.max(minPeriod, halfPeriod / halfPeriodFactor));
      amplitude = Math.max(minAmplitude, amplitude / amplitudeFactor);

      // After half period has passed we are at 0 again. With new period and old
      // index we need to adjust phase such that curve is continuous and new sinus
      // curve starts from zero position. Also as we adjust data on half-periods
      // we need to track which direction our sinus is going, so we need to
      // solve
      //
      //2 * Math.PI / period * x + phase = nHalfPeriods * Math.PI
      //
      // with regards to phase.
      phase = nHalfPeriods * Math.PI - Math.PI / halfPeriod * nextIndex

      currentIndex = nextIndex;
    }

    return {
      periods: periods,
      amplitudes: amplitudes,
      phases: phases,
    }
  }

  const depthRange = depthMax - depthMin;
  const horizontalRange = rows + columns;

  const shorterDirectionPeriodFactor = 0.42
  const longerDirectionPeriodFactor = 0.295

  const iPeriodFactor = rows < columns ? shorterDirectionPeriodFactor : longerDirectionPeriodFactor;
  const jPeriodFactor = rows < columns ? longerDirectionPeriodFactor : shorterDirectionPeriodFactor;


  const iParameters = calculateParameters(
    rows,
    iPeriodFactor * rows, iPeriodFactor * 4,
    0.3 * depthRange, 3
  );
  const iPeriodArray = iParameters.periods
  const iAmplitudeArray = iParameters.amplitudes
  const iPhaseArray = iParameters.phases

  const jParameters = calculateParameters(
    columns,
    jPeriodFactor * columns, jPeriodFactor * 4,
    0.2 * depthRange, 3
  );
  const jPeriodArray = jParameters.periods
  const jAmplitudeArray = jParameters.amplitudes
  const jPhaseArray = jParameters.phases

  const combinedParameters = calculateParameters(
    horizontalRange,
    0.25 * horizontalRange, 1.5,
    0.3 * depthRange, 2
  );
  const combinedPeriodArray = combinedParameters.periods
  const combinedAmplitudeArray = combinedParameters.amplitudes
  const combinedPhaseArray = combinedParameters.phases

  const baseDepthMin = depthMin + depthRange / 3
  const baseDepthMax = depthMax - depthRange / 3
  const baseDepthRange = baseDepthMax - baseDepthMin

  const values = Array.from({ length: rows }, (_, i) => {
    return Array.from({ length: columns }, (_, j) => {
      // baseDepth has highest values at low i,j and lowest and high i,j
      const baseDepth = baseDepthMin + (1 - baseDepthRange * (i + j) / (horizontalRange - 2));

      const iSineModifier = iAmplitudeArray[i] * Math.sin(i * 2 * Math.PI / iPeriodArray[i] + iPhaseArray[i]);
      const jSineModifier = jAmplitudeArray[j] * Math.sin(j * 2 * Math.PI / jPeriodArray[j] + jPhaseArray[j]);
      const combinedSineModifier = combinedAmplitudeArray[i + j] *
        Math.sin((i + j) * 2 * Math.PI / combinedPeriodArray[i + j] + combinedPhaseArray[i + j]);

      return baseDepth + iSineModifier + jSineModifier + combinedSineModifier;
    })
  }
  );
  return values;
}

export function sendCurvedHorizonRequest(
  rows, columns, depthMin, depthMax, surface, above, below, stepsize, attributes
) {
  const values = createCurvedHorizon(rows, columns, depthMin, depthMax)
  console.log(`Requesting horizon of size ${columns * rows}`);
  return sendHorizonRequest(values, surface, above, below, stepsize, attributes);
}
