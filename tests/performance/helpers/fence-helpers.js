import { sendRequest, createEquidistantPoints } from "./request-helpers.js";

export function sendFenceRequest(fence) {
  const vds = __ENV.VDS;
  const sas = __ENV.SAS;
  let payload = {
    coordinateSystem: "ij",
    coordinates: fence,
    vds: vds,
    sas: sas,
  };
  if (fence.length > 20) {
    const fenceStart = JSON.stringify(fence.slice(0, 10)).slice(0, -1);
    const fenceEnd = JSON.stringify(fence.slice(-10)).slice(1);
    console.log(`Requesting fence with coords ${fenceStart} ... ${fenceEnd}`);
  } else {
    console.log(`Requesting fence with coords ${JSON.stringify(fence)}`);
  }

  sendRequest("fence", payload);
}


/**
 * Sends fence request where points are roughly even-spaced between
 * (il1, xl1) and (il2, xl2) inclusively. Points number is maximum coordinate
 * difference across dimensions.
 */
export function sendSequentialFenceRequest(il1, il2, xl1, xl2) {
  const count = Math.max(Math.abs(il1 - il2), Math.abs(xl1 - xl2));
  const pointsInline = createEquidistantPoints(il1, il2, count);
  const pointsCrossline = createEquidistantPoints(xl1, xl2, count);

  const zip = (a, b) => a.map((k, i) => [k, b[i]]);
  const fence = zip(pointsInline, pointsCrossline);
  return sendFenceRequest(fence);
}
