/**
 * Smoke test for the WebAssembly build (issue #171).
 *
 * The JS twin of docs/source/user_guide/python_arrays.rst: a 4 km square
 * domain, a uniform fuel map and a wind field, all built as typed arrays, with
 * no NetCDF and no files. Run it with:
 *
 *   node bindings/wasm/smoke.mjs path/to/forefire.mjs
 */

import { pathToFileURL } from "node:url";
import { resolve } from "node:path";

const modulePath = process.argv[2];
if (!modulePath) {
  console.error("usage: node smoke.mjs <path to forefire.mjs>");
  process.exit(2);
}

// Resolved against the working directory, the way a path typed on a command
// line is meant to be — not against this file, which would send
// `bin/forefire.mjs` looking inside bindings/wasm/.
const { default: createForeFire } = await import(pathToFileURL(resolve(modulePath)).href);

const Module = await createForeFire();
console.log("ForeFire version:", Module.version());

const ff = new Module.ForeFire();

// Fuel table inline: WindDriven only reads vv_coeff, Kcurv and beta.
ff.setString("fuelsTable", "Index;vv_coeff;Kcurv;beta\n1;1.0;1.0;1.0\n2;0.3;1.0;1.0");
ff.setDouble("defaultFuelType", 1.0);

const solver = {
  spatialIncrement: 0.5,
  minimalPropagativeFrontDepth: 10,
  perimeterResolution: 10,
  initialFrontDepth: 0.1,
  relax: 1.0,
  smoothing: 0,
  minSpeed: 0.0,
  bmapLayer: 1,
  windReductionFactor: 1.0,
};
for (const [key, value] of Object.entries(solver)) ff.setDouble(key, value);
ff.setString("propagationModel", "WindDriven");

const L = 4000.0;
console.log(ff.execute(`FireDomain[sw=(0,0,0);ne=(${L},${L},0);t=0]`));

const W = 200;
const H = 200;

// Fuel index 1 everywhere. Shape (t=1, z=1, y=W, x=H), flattened C-order.
const fuel = new Int32Array(W * H).fill(1);

// Wind layers are (t=1, z=2, y=W, x=H): plane 0 is the u-ward response, plane
// 1 the v-ward one, matching the NumPy example's windU/windV split.
const windU = new Float64Array(2 * W * H);
windU.fill(1.0, 0, W * H);
const windV = new Float64Array(2 * W * H);
windV.fill(1.0, W * H);

const layers = {
  propagation: ff.addLayer("propagation", "WindDriven", "propagationModel"),
  fuel: ff.addIndexLayer("table", "fuel", 0, 0, 0, L, L, 0, H, W, 1, 1, fuel),
  windU: ff.addScalarLayer("windScalDir", "windU", 0, 0, 0, L, L, 0, H, W, 2, 1, windU),
  windV: ff.addScalarLayer("windScalDir", "windV", 0, 0, 0, L, L, 0, H, W, 2, 1, windV),
};
console.log("layers:", layers);
const missing = Object.entries(layers).filter(([, ok]) => !ok);
if (missing.length) {
  console.error("layers not registered: " + missing.map(([k]) => k).join(", "));
  process.exit(1);
}

ff.execute("startFire[loc=(2000,2000,0.);t=0]");

let t = 0;
for (let i = 0; i < 8; i++) {
  ff.execute(`trigger[wind;loc=(0.,0.,0.);vel=(8,0,0);t=${t}]`);
  ff.execute("step[dt=30]");
  t += 30;
}

const out = ff.execute("print[]");
console.log(`\nsimulated t = ${ff.getTime()} s`);

// The print output is the shell's own front format: lines of "x, y, z" triples
// inside FireNode[...] entries. Pull the coordinates out to check the front
// actually moved downwind.
const coords = [...out.matchAll(/loc=\(([-\d.eE+]+),([-\d.eE+]+),/g)].map((m) => [
  parseFloat(m[1]),
  parseFloat(m[2]),
]);

if (coords.length === 0) {
  console.error("no front nodes in print[] output:\n" + out.slice(0, 800));
  process.exit(1);
}

const xs = coords.map((c) => c[0]);
const ys = coords.map((c) => c[1]);
console.log(`front nodes: ${coords.length}`);
console.log(
  `x range: ${Math.min(...xs).toFixed(0)}..${Math.max(...xs).toFixed(0)}   ` +
    `y range: ${Math.min(...ys).toFixed(0)}..${Math.max(...ys).toFixed(0)}`
);

// The wind blows along +x at 8 m/s for 240 s, so the front should be stretched
// well downwind and only modestly across it. Checking both axes catches a
// transposed layer, which otherwise looks like a perfectly healthy fire that
// happens to run at ninety degrees to the wind.
const spreadX = Math.max(...xs) - Math.min(...xs);
const spreadY = Math.max(...ys) - Math.min(...ys);
if (spreadX < 1000 || spreadX < 2 * spreadY) {
  console.error(
    `front did not run downwind: x spread ${spreadX.toFixed(0)} m, ` +
      `y spread ${spreadY.toFixed(0)} m`
  );
  process.exit(1);
}
console.log("\nOK");
