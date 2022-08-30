import { sendRequest } from "./request-helpers.js";

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

function createPoints(start, end, segmentCount) {
  const length = end - start;
  const segment = length / segmentCount;
  var points = [];
  for (let i = 0; i <= segmentCount; i++) {
    const position = start + i * segment;
    points.push(Math.floor(position));
  }
  return points;
}

export function sendSequentialFenceRequest(il1, il2, xl1, xl2) {
  const count = Math.max(Math.abs(il1 - il2), Math.abs(xl1 - xl2));
  const pointsInline = createPoints(il1, il2, count);
  const pointsCrossline = createPoints(xl1, xl2, count);

  const zip = (a, b) => a.map((k, i) => [k, b[i]]);
  const fence = zip(pointsInline, pointsCrossline);
  return sendFenceRequest(fence);
}
