// Live 3D view of a ForeFire run coupled to Meso-NH, served by listenHTTP[]
// (tools/htdocs/mnh3d.html, see docs/mnh/coupledMNHInstall.md).
// Reads the fire front with print[], plume and wind with plot[], terrain and
// fuel from the case's data.nc. Everything is placed in ForeFire metres.
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import * as hdf5 from 'jsfive';

const params = new URLSearchParams(location.search);
const DATA_URL = params.get('data') || 'ForeFire/data.nc';
const POLL_MS = 2000;
const FIELDS_EVERY = 4;        // plume and wind every N polls (plot[] is heavier)
const ISO_STEP = 60;           // seconds of simulation between kept past fronts
const S = 0.001;               // metres -> scene units (km)

const PLUME_RAMP = [[0, '#ff8a3d'], [400, '#e0a070'], [1000, '#cfc8c0'], [2500, '#f2f4fa']];
const WIND_RAMP = [[0, '#4f86c9'], [6, '#a9d8ff'], [12, '#ffffff']];
const FUEL_COLORS = ['#6f8f4e', '#9a8f4a', '#4f7a5a', '#b0a070', '#7d6b45', '#5e8c7a',
  '#a3b36b', '#8a5a3c', '#c2b280', '#567d46', '#93785b', '#6b8e8e'];

const $ = (id) => document.getElementById(id);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function setStatus(text, kind = '') {
  $('status').textContent = text;
  $('status').dataset.kind = kind;
}

// ---------- server ----------

// The server is single-threaded and runs commands mid-step: one request at a time.
let queue = Promise.resolve();
function ff(cmd) {
  const run = () => fetch('/', { method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: 'ff:' + cmd })
    .then((r) => { if (!r.ok) throw new Error(`HTTP ${r.status}`); return r.text(); });
  const p = queue.then(run, run);
  queue = p.catch(() => {});
  return p;
}

async function getText(path) {
  const r = await fetch(path, { cache: 'no-store' });
  if (!r.ok) throw new Error(`${path}: HTTP ${r.status}`);
  return r.text();
}

function parseDump(txt) {
  const m = txt.match(/FireDomain\[sw=\(([^)]*)\);ne=\(([^)]*)\);t=([-+\d.eE]+)/);
  if (!m) return null;
  const sw = m[1].split(',').map(Number);
  const ne = m[2].split(',').map(Number);
  const fronts = new Map();
  for (const n of txt.matchAll(/FireNode\[[^\]]*?loc=\(([^)]*)\)[^\]]*?frontId=(\d+)/g)) {
    if (!fronts.has(n[2])) fronts.set(n[2], []);
    fronts.get(n[2]).push(n[1].split(',').map(Number));
  }
  return { sw, ne, t: Number(m[3]), fronts: [...fronts.values()].filter((f) => f.length > 2) };
}

function parseAsc(txt) {
  const tok = txt.trim().split(/\s+/);
  const h = {};
  let k = 0;
  while (k < tok.length && /^[a-zA-Z_]/.test(tok[k])) { h[tok[k].toLowerCase()] = Number(tok[k + 1]); k += 2; }
  const v = Float32Array.from(tok.slice(k), Number);
  return { nc: h.ncols, nr: h.nrows, xll: h.xllcorner, yll: h.yllcorner, cs: h.cellsize, v };
}

// ---------- geometry helpers ----------

let D = null;   // ForeFire domain: x0, y0, lx, ly, cx, cy
let alt = null; // altitude grid from data.nc

const toScene = (X, Y, Z) => [(X - D.cx) * S, Z * S, -(Y - D.cy) * S];

// ForeFire maps a data layer onto its own domain: fx = (X - sw) / L * N, bilinear.
// Checked against the z of the fire nodes on Pedrógão (RMS 4 cm).
function altitudeAt(X, Y) {
  if (!alt) return 0;
  const fx = Math.min(Math.max((X - D.x0) / D.lx * alt.nx, 0), alt.nx - 1.0001);
  const fy = Math.min(Math.max((Y - D.y0) / D.ly * alt.ny, 0), alt.ny - 1.0001);
  const i = Math.floor(fx), j = Math.floor(fy), a = fx - i, b = fy - j;
  const g = (ii, jj) => alt.v[jj * alt.nx + ii];
  return g(i, j) * (1 - a) * (1 - b) + g(i + 1, j) * a * (1 - b) + g(i, j + 1) * (1 - a) * b + g(i + 1, j + 1) * a * b;
}

function hexRgb(hex) { return [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16) / 255); }
function ramp(stops, v) {
  if (v <= stops[0][0]) return hexRgb(stops[0][1]);
  for (let k = 1; k < stops.length; k++) {
    if (v <= stops[k][0]) {
      const t = (v - stops[k - 1][0]) / (stops[k][0] - stops[k - 1][0]);
      const a = hexRgb(stops[k - 1][1]), b = hexRgb(stops[k][1]);
      return a.map((x, i) => x + (b[i] - x) * t);
    }
  }
  return hexRgb(stops[stops.length - 1][1]);
}
function cssRamp(stops) {
  const max = stops[stops.length - 1][0];
  return `linear-gradient(90deg, ${stops.map(([v, c]) => `${c} ${(v / max) * 100}%`).join(', ')})`;
}

function disposeChildren(group) {
  for (const c of [...group.children]) { group.remove(c); c.geometry?.dispose(); c.material?.dispose(); }
}

// ---------- scene ----------

const renderer = new THREE.WebGLRenderer({ canvas: $('view'), antialias: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
const scene = new THREE.Scene();
scene.background = new THREE.Color(0x0b0e14);
scene.fog = new THREE.Fog(0x0b0e14, 40, 140);
const camera = new THREE.PerspectiveCamera(45, 1, 0.01, 500);
camera.position.set(-12, 14, 18);
const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.maxPolarAngle = Math.PI * 0.49;
scene.add(new THREE.HemisphereLight(0xcfd8ff, 0x2a2016, 0.9));
const sun = new THREE.DirectionalLight(0xffffff, 1.8);
sun.position.set(-20, 18, 10);
scene.add(sun);

const world = new THREE.Group();
const terrainGroup = new THREE.Group(), frontGroup = new THREE.Group(), isoGroup = new THREE.Group();
const plumeGroup = new THREE.Group(), windGroup = new THREE.Group();
world.add(terrainGroup, isoGroup, frontGroup, plumeGroup, windGroup);
world.scale.y = 2;
scene.add(world);

function resize() {
  const w = innerWidth, h = innerHeight;
  renderer.setSize(w, h, false);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
}
addEventListener('resize', resize);
resize();

// ---------- terrain ----------

let terrainMat = null, fuelTex = null;

// Terrain first, fuel texture after: decoding the fuel grid (millions of cells)
// is what makes the first view slow, so the front never waits for it.
async function loadTerrain() {
  let f = null;
  try {
    const r = await fetch(DATA_URL, { cache: 'no-store' });
    if (!r.ok) throw new Error(`${DATA_URL}: HTTP ${r.status}`);
    f = new hdf5.File(await r.arrayBuffer(), DATA_URL);
    const a = f.get('altitude');
    const [ny, nx] = a.shape.slice(-2);
    alt = { nx, ny, v: Float32Array.from(a.value) };
  } catch (e) { console.warn('terrain', e); }
  buildTerrain();
  focusFire();
  if (!f) return;
  await sleep(50);
  try {
    const g = f.get('fuel');
    const [fny, fnx] = g.shape.slice(-2);
    fuelTex = fuelTexture({ nx: fnx, ny: fny, v: g.value });
    applyFuel();
  } catch (e) { console.warn('fuel', e); }
}

function fuelTexture(fuel) {
  const step = Math.max(1, Math.ceil(Math.max(fuel.nx, fuel.ny) / 1400));
  const w = Math.floor(fuel.nx / step), h = Math.floor(fuel.ny / step);
  const cv = document.createElement('canvas');
  cv.width = w; cv.height = h;
  const ctx = cv.getContext('2d');
  const img = ctx.createImageData(w, h);
  const classes = new Map();
  for (let j = 0; j < h; j++) {
    for (let i = 0; i < w; i++) {
      const v = fuel.v[j * step * fuel.nx + i * step];
      let c = classes.get(v);
      if (!c) {
        c = v <= 0 ? [58, 58, 62] : hexRgb(FUEL_COLORS[classes.size % FUEL_COLORS.length]).map((x) => x * 255);
        classes.set(v, c);
      }
      const o = ((h - 1 - j) * w + i) * 4; // row 0 of the grid is south
      img.data[o] = c[0]; img.data[o + 1] = c[1]; img.data[o + 2] = c[2]; img.data[o + 3] = 255;
    }
  }
  ctx.putImageData(img, 0, 0);
  const tex = new THREE.CanvasTexture(cv);
  tex.colorSpace = THREE.SRGBColorSpace;
  tex.anisotropy = 8;
  return tex;
}

function buildTerrain() {
  const M = 171;
  const pos = new Float32Array(M * M * 3), uv = new Float32Array(M * M * 2);
  for (let j = 0; j < M; j++) {
    for (let i = 0; i < M; i++) {
      const X = D.x0 + (i / (M - 1)) * D.lx, Y = D.y0 + (j / (M - 1)) * D.ly;
      pos.set(toScene(X, Y, altitudeAt(X, Y)), (j * M + i) * 3);
      uv.set([i / (M - 1), j / (M - 1)], (j * M + i) * 2);
    }
  }
  const idx = [];
  for (let j = 0; j < M - 1; j++) {
    for (let i = 0; i < M - 1; i++) {
      const a = j * M + i, b = a + 1, c = a + M, d = c + 1;
      idx.push(a, b, c, b, d, c);
    }
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
  geo.setIndex(idx);
  geo.computeVertexNormals();
  terrainMat = new THREE.MeshStandardMaterial({ roughness: 0.95, metalness: 0 });
  applyFuel();
  disposeChildren(terrainGroup);
  terrainGroup.add(new THREE.Mesh(geo, terrainMat));
}

function applyFuel() {
  if (!terrainMat) return;
  const on = $('lyrFuel').checked && fuelTex;
  terrainMat.map = on ? fuelTex : null;
  terrainMat.color.set(on ? 0xffffff : 0x5d6658);
  terrainMat.needsUpdate = true;
}

// ---------- fire front ----------

const LIFT = 6, CURTAIN = 80; // metres

function frontRibbon(pts) {
  const n = pts.length;
  const pos = new Float32Array(n * 2 * 3), col = new Float32Array(n * 2 * 4);
  pts.forEach(([X, Y, Z], k) => {
    pos.set(toScene(X, Y, Z + LIFT), k * 6);
    pos.set(toScene(X, Y, Z + LIFT + CURTAIN), k * 6 + 3);
    col.set([1, 0.85, 0.35, 0.95, 1, 0.25, 0.05, 0], k * 8);
  });
  const idx = [];
  for (let k = 0; k < n; k++) {
    const a = 2 * k, b = 2 * ((k + 1) % n);
    idx.push(a, b, a + 1, b, b + 1, a + 1);
  }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(col, 4));
  geo.setIndex(idx);
  const mat = new THREE.MeshBasicMaterial({ vertexColors: true, transparent: true, side: THREE.DoubleSide,
    depthWrite: false, blending: THREE.AdditiveBlending, fog: false });
  return new THREE.Mesh(geo, mat);
}

function frontLine(pts, color) {
  const geo = new THREE.BufferGeometry().setFromPoints(pts.map(([X, Y, Z]) => new THREE.Vector3(...toScene(X, Y, Z + LIFT))));
  return new THREE.LineLoop(geo, new THREE.LineBasicMaterial({ color, fog: false }));
}

function drawFronts(fronts) {
  disposeChildren(frontGroup);
  for (const f of fronts) {
    frontGroup.add(frontRibbon(f));
    frontGroup.add(frontLine(f, 0xffd36b));
  }
}

// ---------- plume ----------

function drawPlume(g) {
  const n = g.nc * g.nr;
  const pos = new Float32Array(n * 3), col = new Float32Array(n * 4);
  let maxAgl = 0;
  for (let r = 0; r < g.nr; r++) {
    for (let c = 0; c < g.nc; c++) {
      const k = r * g.nc + c;
      const X = g.xll + (c + 0.5) * g.cs, Y = g.yll + (g.nr - 1 - r + 0.5) * g.cs; // rows run north to south
      const ground = altitudeAt(X, Y), v = g.v[k];
      const has = v > ground;
      const agl = has ? v - ground : 0;
      maxAgl = Math.max(maxAgl, agl);
      pos.set(toScene(X, Y, has ? v : ground), k * 3);
      const rgb = ramp(PLUME_RAMP, agl);
      col.set([...rgb, has ? Math.min(0.75, 0.25 + agl / 1500) : 0], k * 4);
    }
  }
  const idx = [];
  for (let r = 0; r < g.nr - 1; r++) {
    for (let c = 0; c < g.nc - 1; c++) {
      const a = r * g.nc + c, b = a + 1, d = a + g.nc, e = d + 1;
      if (col[a * 4 + 3] + col[b * 4 + 3] + col[d * 4 + 3] + col[e * 4 + 3] === 0) continue;
      idx.push(a, d, b, b, d, e);
    }
  }
  disposeChildren(plumeGroup);
  if (!idx.length) return 0;
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(col, 4));
  geo.setIndex(idx);
  plumeGroup.add(new THREE.Mesh(geo, new THREE.MeshBasicMaterial({
    vertexColors: true, transparent: true, side: THREE.DoubleSide, depthWrite: false })));
  return maxAgl;
}

// ---------- wind particles ----------

let wind = null;
const NP = 3000, SPEEDUP = 60, TAIL = 25; // visual: 1 s on screen = 60 s of wind
const part = { x: new Float32Array(NP), y: new Float32Array(NP), age: new Float32Array(NP), life: new Float32Array(NP) };
const windGeo = new THREE.BufferGeometry();
windGeo.setAttribute('position', new THREE.BufferAttribute(new Float32Array(NP * 6), 3));
windGeo.setAttribute('color', new THREE.BufferAttribute(new Float32Array(NP * 6), 3));
windGroup.add(new THREE.LineSegments(windGeo, new THREE.LineBasicMaterial({ vertexColors: true, transparent: true, opacity: 0.85 })));

function setWind(json, bounds) {
  const [u, v] = json;
  const h = u.header;
  wind = { nx: h.nx, ny: h.ny, lo1: h.lo1, la1: h.la1, dx: h.dx, dy: h.dy, u: u.data, v: v.data, b: bounds };
}

function windAt(X, Y) {
  const b = wind.b;
  const lon = b.SWlon + (X - D.x0) / D.lx * (b.NElon - b.SWlon);
  const lat = b.SWlat + (Y - D.y0) / D.ly * (b.NElat - b.SWlat);
  const fx = (lon - wind.lo1) / wind.dx, fy = (wind.la1 - lat) / wind.dy;
  if (fx < 0 || fy < 0 || fx > wind.nx - 1 || fy > wind.ny - 1) return [0, 0];
  const i = Math.min(Math.floor(fx), wind.nx - 2), j = Math.min(Math.floor(fy), wind.ny - 2), a = fx - i, c = fy - j;
  const at = (arr) => {
    const g = (ii, jj) => arr[jj * wind.nx + ii];
    return g(i, j) * (1 - a) * (1 - c) + g(i + 1, j) * a * (1 - c) + g(i, j + 1) * (1 - a) * c + g(i + 1, j + 1) * a * c;
  };
  return [at(wind.u), at(wind.v)];
}

function respawn(k) {
  part.x[k] = D.x0 + Math.random() * D.lx;
  part.y[k] = D.y0 + Math.random() * D.ly;
  part.age[k] = 0;
  part.life[k] = 2 + Math.random() * 4;
}

function stepWind(dt) {
  if (!wind || !D || !windGroup.visible) return;
  const pos = windGeo.attributes.position.array, col = windGeo.attributes.color.array;
  for (let k = 0; k < NP; k++) {
    if (part.life[k] === 0) respawn(k);
    const [u, v] = windAt(part.x[k], part.y[k]);
    const sp = Math.hypot(u, v);
    part.x[k] += u * dt * SPEEDUP;
    part.y[k] += v * dt * SPEEDUP;
    part.age[k] += dt;
    const out = part.x[k] < D.x0 || part.y[k] < D.y0 || part.x[k] > D.x0 + D.lx || part.y[k] > D.y0 + D.ly;
    if (out || sp < 0.05 || part.age[k] > part.life[k]) { respawn(k); continue; }
    const X = part.x[k], Y = part.y[k], tx = X - u * TAIL, ty = Y - v * TAIL;
    pos.set(toScene(X, Y, altitudeAt(X, Y) + 40), k * 6);
    pos.set(toScene(tx, ty, altitudeAt(tx, ty) + 40), k * 6 + 3);
    const fade = Math.sin(Math.PI * part.age[k] / part.life[k]);
    const c = ramp(WIND_RAMP, sp);
    col.set(c.map((x) => x * fade), k * 6);
    col.set(c.map((x) => x * fade * 0.15), k * 6 + 3);
  }
  windGeo.attributes.position.needsUpdate = true;
  windGeo.attributes.color.needsUpdate = true;
}

// ---------- camera ----------

let lastFronts = [];

function focusOn(minX, minY, maxX, maxY, minDist) {
  const cx = (minX + maxX) / 2, cy = (minY + maxY) / 2;
  const [x, y, z] = toScene(cx, cy, altitudeAt(cx, cy));
  const target = new THREE.Vector3(x, y * world.scale.y, z);
  const dist = Math.max(minDist, Math.max(maxX - minX, maxY - minY) * S * 2.2);
  const dir = camera.position.clone().sub(controls.target).normalize();
  controls.target.copy(target);
  camera.position.copy(target).addScaledVector(dir, dist);
}

function focusFire() {
  const pts = lastFronts.flat();
  if (!pts.length) return focusDomain();
  const xs = pts.map((p) => p[0]), ys = pts.map((p) => p[1]);
  focusOn(Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys), 4);
}

function focusDomain() {
  if (D) focusOn(D.x0, D.y0, D.x0 + D.lx, D.y0 + D.ly, 10);
}

// ---------- polling ----------

let t0 = null, lastIsoT = -Infinity, firstFocus = true;
const info = { nodes: 0, fronts: 0, plume: 0, windMax: 0 };

async function pollFront() {
  await ff('setParameter[dumpMode=ff]');
  const dump = parseDump(await ff('print[]'));
  if (!dump) throw new Error('no FireDomain in print[]');
  if (!D) {
    const [x0, y0] = dump.sw, [x1, y1] = dump.ne;
    D = { x0, y0, lx: x1 - x0, ly: y1 - y0, cx: (x0 + x1) / 2, cy: (y0 + y1) / 2 };
    loadTerrain();
  }
  t0 ??= dump.t;
  lastFronts = dump.fronts;
  drawFronts(dump.fronts);
  if (dump.t - lastIsoT >= ISO_STEP && dump.fronts.length) {
    lastIsoT = dump.t;
    for (const f of dump.fronts) isoGroup.add(frontLine(f, 0xa3341c));
  }
  if (firstFocus && dump.fronts.length) { firstFocus = false; focusFire(); }
  info.fronts = dump.fronts.length;
  info.nodes = dump.fronts.reduce((s, f) => s + f.length, 0);
  const iso = (await ff('getParameter[ISOdate]')).trim();
  $('clock').textContent = iso.replace('T', ' ').replace('Z', ' UTC');
  $('elapsed').textContent = `t = ${dump.t.toFixed(0)} s  (+${(dump.t - t0).toFixed(0)} s on this page)`;
}

async function pollFields() {
  await ff('plot[parameter=plumeTopHeight;filename=mnh3d_plumeTop.asc;size=128,128]');
  info.plume = drawPlume(parseAsc(await getText('mnh3d_plumeTop.asc')));
  const bounds = JSON.parse(await ff('plot[parameter=wind;filename=mnh3d_wind.json;size=48,48;projectionOut=json]'));
  setWind(JSON.parse(await getText('mnh3d_wind.json')), bounds);
  info.windMax = bounds.maxVal;
}

async function loop() {
  for (let i = 0; ; i++) {
    try {
      await pollFront();
      if (i % FIELDS_EVERY === 0) await pollFields();
      setStatus('live', 'ok');
    } catch (e) {
      setStatus(`no server (run ended or not started): ${e.message}`, 'err');
    }
    $('stats').textContent = `fronts ${info.fronts}   nodes ${info.nodes}\n`
      + `plume max ${info.plume.toFixed(0)} m above ground\nwind max ${info.windMax.toFixed(1)} m/s`;
    await sleep(POLL_MS);
  }
}

// ---------- UI ----------

const bind = (id, group) => $(id).addEventListener('change', (e) => { group.visible = e.target.checked; });
bind('lyrFront', frontGroup);
bind('lyrIso', isoGroup);
bind('lyrPlume', plumeGroup);
bind('lyrWind', windGroup);
$('lyrFuel').addEventListener('change', applyFuel);
$('exag').addEventListener('input', (e) => {
  world.scale.y = Number(e.target.value);
  $('exagVal').textContent = `${e.target.value}×`;
});
$('focusFire').addEventListener('click', focusFire);
$('focusDomain').addEventListener('click', focusDomain);
$('rampPlume').style.background = cssRamp(PLUME_RAMP);
$('rampWind').style.background = cssRamp(WIND_RAMP);

const clock = new THREE.Clock();
renderer.setAnimationLoop(() => {
  stepWind(Math.min(clock.getDelta(), 0.05));
  controls.update();
  renderer.render(scene, camera);
});

loop();
