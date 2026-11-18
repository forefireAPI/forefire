// Live 3D view of a ForeFire run coupled to Meso-NH, served by listenHTTP[]
// (tools/htdocs/mnh3d.html, see tests/mnh/README.md).
// Reads the fire front with print[], plume and wind with plot[], terrain and
// fuel from the case's data.nc. Everything is placed in ForeFire metres.
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import * as hdf5 from 'jsfive';

const params = new URLSearchParams(location.search);
const DATA_URL = params.get('data') || 'ForeFire/data.nc';
// HTTP commands run on the server thread, concurrently with the time step, so
// poll gently: front every 5 s, plume and wind every 30 s.
const POLL_MS = 5000;
const FIELDS_EVERY = 6;
const MAX_ISO = 200;           // past fronts kept on screen
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
    .catch(() => { throw Object.assign(new Error('no server'), { offline: true }); })
    .then((r) => { if (!r.ok) throw new Error(`HTTP ${r.status} on ${cmd}`); return r.text(); });
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

// ESRI ASCII grid: six header lines, then rows from north to south.
function parseAsc(txt) {
  const lines = txt.split('\n');
  const h = {};
  for (const line of lines.slice(0, 6)) {
    const [key, val] = line.trim().split(/\s+/);
    h[key.toLowerCase()] = Number(val);
  }
  const v = Float32Array.from(lines.slice(6).join(' ').trim().split(/\s+/), Number);
  if (v.length !== h.ncols * h.nrows) throw new Error('truncated plume grid');
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
  if (lastPlume) info.plume = drawPlume(lastPlume, lastPlumeBottom); // first plume was drawn on flat ground
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

let plumeSim = null;

const PLUME_VERT = `
  attribute float aSize;
  attribute vec4 aColor;
  varying vec4 vColor;
  void main() {
    vColor = aColor;
    vec4 mv = modelViewMatrix * vec4(position, 1.0);
    gl_Position = projectionMatrix * mv;
    gl_PointSize = clamp(aSize * (projectionMatrix[1][1] * 420.0) / max(-mv.z, 0.2), 3.0, 260.0);
  }
`;

const PLUME_FRAG = `
  varying vec4 vColor;
  void main() {
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(p, p);
    if (r2 > 1.0) discard;
    float nz = sqrt(max(0.0, 1.0 - r2));
    // Soft multi-lobe cumulus edge + sunlit top / shadowed base on each puff
    float lobe = 0.88 + 0.12 * sin(p.x * 5.0 + p.y * 3.0) * cos(p.y * 4.5 - p.x * 2.5);
    float falloff = pow(max(0.0, 1.0 - r2 * lobe), 1.85);
    float light = 0.72 + 0.34 * clamp(-0.35 * p.x - 0.65 * p.y + 0.65 * nz, -0.6, 1.0);
    gl_FragColor = vec4(vColor.rgb * light, vColor.a * falloff);
  }
`;

const plumeMat = new THREE.ShaderMaterial({
  vertexShader: PLUME_VERT,
  fragmentShader: PLUME_FRAG,
  transparent: true,
  depthWrite: false,
});

// Unmix ForeFire's bilinear blend with the -999 sentinel on .asc grids, or read
// the raw 36x36 atmospheric grid directly when available.
function cleanPlumeGrid(g, botGrid = null) {
  if (g.raw36) {
    // 36x36 matrix from MODEL2/plumeTopHeight_uint8_36_36.dat (1-cell halo around 34x34 domain)
    const nx = 34, ny = 34, dx = D.lx / nx, dy = D.ly / ny;
    const top = new Float32Array(nx * ny), bot = new Float32Array(nx * ny), w = new Float32Array(nx * ny);
    for (let j = 0; j < ny; j++) {
      for (let i = 0; i < nx; i++) {
        const val = g.u8[(i + 1) * 36 + (j + 1)] * (6000 / 255);
        const X = D.x0 + (i + 0.5) * dx, Y = D.y0 + (j + 0.5) * dy, ground = altitudeAt(X, Y);
        const k = j * nx + i;
        if (val > ground + 40) { top[k] = val; bot[k] = ground; w[k] = 1; }
      }
    }
    return { nx, ny, dx, dy, top, bot, w };
  }
  const nx = g.nc, ny = g.nr, dx = D.lx / nx, dy = D.ly / ny;
  const top = new Float32Array(nx * ny), bot = new Float32Array(nx * ny), w = new Float32Array(nx * ny);
  const R = 4; // ~1 atmospheric cell (800 m) to recover the un-blended top height
  for (let r = 0; r < ny; r++) {
    for (let c = 0; c < nx; c++) {
      const j = ny - 1 - r; // row 0 in .asc is north; j=0 here is south
      const X = D.x0 + (c + 0.5) * dx, Y = D.y0 + (j + 0.5) * dy, ground = altitudeAt(X, Y);
      let topLoc = -999;
      for (let rr = Math.max(0, r - R); rr <= Math.min(ny - 1, r + R); rr++) {
        for (let cc = Math.max(0, c - R); cc <= Math.min(nx - 1, c + R); cc++) {
          const v = g.v[rr * nx + cc];
          if (v > topLoc) topLoc = v;
        }
      }
      if (topLoc <= ground + 40) continue;
      const raw = g.v[r * nx + c];
      const cov = Math.max(0, Math.min(1, (raw + 999) / (topLoc + 999)));
      if (cov < 0.16) continue;
      const k = j * nx + c;
      top[k] = topLoc;
      w[k] = cov;
      let bVal = ground;
      if (botGrid && botGrid.nc === nx && botGrid.nr === ny) {
        const bv = botGrid.v[r * nx + c];
        if (bv > ground) bVal = Math.min(topLoc - 80, bv);
      }
      bot[k] = bVal;
    }
  }
  return { nx, ny, dx, dy, top, bot, w };
}

function samplePlumeField(field, X, Y) {
  const gx = (X - D.x0) / field.dx - 0.5, gy = (Y - D.y0) / field.dy - 0.5;
  const i0 = Math.floor(gx), j0 = Math.floor(gy);
  let wSum = 0, actSum = 0, topSum = 0, botSum = 0;
  for (let dj = -1; dj <= 2; dj++) {
    for (let di = -1; di <= 2; di++) {
      const ii = i0 + di, jj = j0 + dj;
      if (ii < 0 || jj < 0 || ii >= field.nx || jj >= field.ny) continue;
      const dx = gx - ii, dy = gy - jj;
      const kern = Math.exp(-(dx * dx + dy * dy) * 0.85);
      const k = jj * field.nx + ii, cw = field.w[k];
      wSum += kern;
      if (cw > 0) {
        const aw = kern * cw;
        actSum += aw;
        topSum += aw * field.top[k];
        botSum += aw * field.bot[k];
      }
    }
  }
  if (!wSum || actSum / wSum < 0.14) return null;
  return { w: Math.min(1, actSum / wSum), top: topSum / actSum, bot: botSum / actSum };
}

function buildPlumeCanopy(field) {
  const N = 88;
  const topZ = new Float32Array(N * N), cov = new Float32Array(N * N), gnd = new Float32Array(N * N);
  for (let j = 0; j < N; j++) {
    const Y = D.y0 + (j / (N - 1)) * D.ly;
    for (let i = 0; i < N; i++) {
      const X = D.x0 + (i / (N - 1)) * D.lx, k = j * N + i;
      gnd[k] = altitudeAt(X, Y);
      const s = samplePlumeField(field, X, Y);
      if (s && s.top > gnd[k] + 60) {
        topZ[k] = s.top;
        cov[k] = s.w;
      }
    }
  }
  // Fill 1-cell border outside the plume with neighbor's topZ (with cov=0) so edge
  // triangles feather out horizontally in the sky instead of dropping to the ground!
  for (let pass = 0; pass < 2; pass++) {
    const nextZ = Float32Array.from(topZ);
    for (let j = 1; j < N - 1; j++) {
      for (let i = 1; i < N - 1; i++) {
        const k = j * N + i;
        if (nextZ[k] > 0) continue;
        let sum = 0, cnt = 0;
        for (const nb of [k - 1, k + 1, k - N, k + N]) {
          if (topZ[nb] > 0) { sum += topZ[nb]; cnt++; }
        }
        if (cnt) nextZ[k] = sum / cnt;
      }
    }
    topZ.set(nextZ);
  }

  const pos = new Float32Array(N * N * 3), col = new Float32Array(N * N * 4);
  for (let j = 0; j < N; j++) {
    const Y = D.y0 + (j / (N - 1)) * D.ly;
    for (let i = 0; i < N; i++) {
      const X = D.x0 + (i / (N - 1)) * D.lx, k = j * N + i;
      const z = topZ[k] > 0 ? topZ[k] : gnd[k];
      const agl = Math.max(0, z - gnd[k]);
      pos.set(toScene(X, Y, z), k * 3);
      const rgb = ramp(PLUME_RAMP, agl);
      const alpha = cov[k] > 0.05 ? Math.min(0.38, 0.38 * Math.pow(cov[k], 1.35)) : 0;
      col.set([...rgb, alpha], k * 4);
    }
  }
  const idx = [];
  for (let j = 0; j < N - 1; j++) {
    for (let i = 0; i < N - 1; i++) {
      const a = j * N + i, b = a + 1, c = a + N, d = c + 1;
      if (cov[a] + cov[b] + cov[c] + cov[d] < 0.08) continue;
      if (topZ[a] <= 0 || topZ[b] <= 0 || topZ[c] <= 0 || topZ[d] <= 0) continue;
      idx.push(a, b, c, b, d, c);
    }
  }
  if (!idx.length) return null;
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('color', new THREE.BufferAttribute(col, 4));
  geo.setIndex(idx);
  geo.computeVertexNormals();
  const mesh = new THREE.Mesh(geo, new THREE.MeshLambertMaterial({
    vertexColors: true, transparent: true, side: THREE.DoubleSide, depthWrite: false,
  }));
  mesh.frustumCulled = false;
  return mesh;
}

function drawPlume(g, botGrid = null) {
  disposeChildren(plumeGroup);
  plumeSim = null;
  if (!g || !D) return 0;
  const field = cleanPlumeGrid(g, botGrid);
  plumeField = field;
  const firePts = lastFronts.flat();
  const distToFire = (X, Y) => {
    if (!firePts.length) return 900;
    let d2 = Infinity;
    for (let i = 0; i < firePts.length; i += 2) {
      const dx = X - firePts[i][0], dy = Y - firePts[i][1];
      const dd = dx * dx + dy * dy;
      if (dd < d2) d2 = dd;
    }
    return Math.sqrt(d2);
  };

  // 1) Translucent lit canopy surface of plumeTopHeight (floating, no skirt to ground)
  const canopy = buildPlumeCanopy(field);
  if (canopy) plumeGroup.add(canopy);

  // 2) Medium-density volumetric smoke puffs rising from the fire into the canopy
  const step = 335, cols = [];
  let maxAgl = 0;
  for (let Y = D.y0 + step * 0.5; Y < D.y0 + D.ly; Y += step) {
    for (let X = D.x0 + step * 0.5; X < D.x0 + D.lx; X += step) {
      const s = samplePlumeField(field, X, Y);
      if (!s) continue;
      const ground = altitudeAt(X, Y);
      const agl = Math.max(0, s.top - ground);
      if (agl < 60) continue;
      if (s.w > 0.35) maxAgl = Math.max(maxAgl, agl);
      const dFire = distToFire(X, Y);
      const lift = Math.min(1, Math.max(0, (dFire - 260) / 1900));
      const baseAgl = Math.max(s.bot - ground, agl * 0.48 * lift * lift);
      const edgeTaper = Math.sqrt(s.w);
      const midAgl = baseAgl * 0.42 + agl * 0.58;
      const z0 = ground + midAgl - (midAgl - baseAgl) * edgeTaper;
      const z1 = ground + midAgl + (agl - midAgl) * edgeTaper;
      if (z1 - z0 > 50) cols.push({ X, Y, ground, z0, z1, agl, w: s.w, dFire });
    }
  }
  if (!cols.length) return 0;

  const puffs = [];
  const hash = (n) => { const x = Math.sin(n * 127.1 + 311.7) * 43758.5453; return x - Math.floor(x); };

  cols.forEach((c, idx) => {
    const thick = c.z1 - c.z0;
    const layers = Math.max(2, Math.min(6, Math.round(thick / 390)));
    for (let l = 0; l < layers; l++) {
      const seed = idx * 13 + l * 7;
      const phase = (l + 0.15 + 0.7 * hash(seed)) / layers;
      const jx = (hash(seed + 1) - 0.5) * step * 1.15;
      const jy = (hash(seed + 2) - 0.5) * step * 1.15;
      puffs.push({ X: c.X + jx, Y: c.Y + jy, ground: c.ground, z0: c.z0, z1: c.z1,
        w: c.w, dFire: c.dFire, phase, seed });
    }
  });

  // Extra rising hot-smoke puffs right along the active fire fronts
  for (let f = 0; f < firePts.length; f += 2) {
    const [fx, fy, fz] = firePts[f];
    const s = samplePlumeField(field, fx, fy);
    const topZ = s ? Math.max(fz + 450, s.top) : fz + 650;
    for (let m = 0; m < 3; m++) {
      const seed = 50000 + f * 5 + m;
      puffs.push({
        X: fx + (hash(seed) - 0.5) * 140,
        Y: fy + (hash(seed + 1) - 0.5) * 140,
        ground: fz, z0: fz + 25, z1: Math.min(topZ, fz + 1100),
        w: 0.92, dFire: 0, phase: (m + hash(seed + 2) * 0.8) / 3, seed, hot: true,
      });
    }
  }

  const N = puffs.length;
  const pos = new Float32Array(N * 3), col = new Float32Array(N * 4), sz = new Float32Array(N);
  const geo = new THREE.BufferGeometry();
  geo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  geo.setAttribute('aColor', new THREE.BufferAttribute(col, 4));
  geo.setAttribute('aSize', new THREE.BufferAttribute(sz, 1));
  plumeSim = { puffs, geo };
  updatePlumePuffs(0);
  const ptsMesh = new THREE.Points(geo, plumeMat);
  ptsMesh.frustumCulled = false;
  plumeGroup.add(ptsMesh);
  return maxAgl;
}

function updatePlumePuffs(dt) {
  if (!plumeSim || !D || !plumeGroup.visible) return;
  const { puffs, geo } = plumeSim;
  const pos = geo.attributes.position.array;
  const col = geo.attributes.aColor.array;
  const sz = geo.attributes.aSize.array;
  for (let i = 0; i < puffs.length; i++) {
    const p = puffs[i];
    p.phase = (p.phase + dt * (p.hot ? 0.11 : 0.045)) % 1;
    const rel = p.phase;
    const Z = p.z0 + rel * (p.z1 - p.z0);
    const agl = Math.max(0, Z - p.ground);
    let wx = 0, wy = 0;
    if (wind || windU8Frame) [wx, wy] = windAt(p.X, p.Y);
    const drift = (rel - 0.3) * (p.hot ? 18 : 42);
    const i3 = i * 3;
    pos[i3]     =  (p.X + wx * drift - D.cx) * S;
    pos[i3 + 1] =  Z * S;
    pos[i3 + 2] = -(p.Y + wy * drift - D.cy) * S;

    const baseRgb = ramp(PLUME_RAMP, agl);
    const shade = 0.54 + 0.46 * Math.pow(rel, 0.75);
    const fireGlow = Math.max(0, 1 - p.dFire / 520) * Math.pow(1 - rel, 1.6);
    const i4 = i * 4;
    col[i4]     = Math.min(1, baseRgb[0] * shade * (1 - 0.35 * fireGlow) + 0.98 * fireGlow);
    col[i4 + 1] = Math.min(1, baseRgb[1] * shade * (1 - 0.55 * fireGlow) + 0.42 * fireGlow);
    col[i4 + 2] = Math.min(1, baseRgb[2] * shade * (1 - 0.70 * fireGlow) + 0.14 * fireGlow);
    const env = Math.sin(Math.PI * Math.min(1, Math.max(0.08, rel)));
    col[i4 + 3] = (p.hot ? 0.19 : 0.145) * p.w * (0.45 + 0.55 * env);

    sz[i] = (p.hot ? (0.24 + 0.30 * rel) : (0.50 + 0.46 * rel) * (0.75 + 0.35 * p.w));
  }
  geo.attributes.position.needsUpdate = true;
  geo.attributes.aColor.needsUpdate = true;
  geo.attributes.aSize.needsUpdate = true;
}

// ---------- wind particles (Windy-style 3D coupled streamlines) ----------

let wind = null, windU8Frame = null, windV8Frame = null, tkeU8Frame = null, plumeField = null;
const NP = 1600, TRAIL = 15, SEG_PER_P = TRAIL - 1;
const SPEEDUP = 55; // visual: 1 s on screen = 55 s of wind
const part = {
  x: new Float32Array(NP), y: new Float32Array(NP), z: new Float32Array(NP),
  heat: new Float32Array(NP), age: new Float32Array(NP), life: new Float32Array(NP),
  // Ring buffer of scene coordinates (sx, sy, sz) and pre-blended base color (r, g, b)
  sx: new Float32Array(NP * TRAIL), sy: new Float32Array(NP * TRAIL), sz: new Float32Array(NP * TRAIL),
  cr: new Float32Array(NP * TRAIL), cg: new Float32Array(NP * TRAIL), cb: new Float32Array(NP * TRAIL),
  head: new Uint8Array(NP),
};
const TAIL_FADE = Float32Array.from({ length: TRAIL }, (_, s) => Math.pow(1 - s / SEG_PER_P, 1.55));
const windGeo = new THREE.BufferGeometry();
windGeo.setAttribute('position', new THREE.BufferAttribute(new Float32Array(NP * SEG_PER_P * 6), 3));
windGeo.setAttribute('color', new THREE.BufferAttribute(new Float32Array(NP * SEG_PER_P * 6), 3));
const windLines = new THREE.LineSegments(windGeo, new THREE.LineBasicMaterial({
  vertexColors: true, transparent: true, opacity: 0.92, depthWrite: false, blending: THREE.AdditiveBlending,
}));
windLines.frustumCulled = false; // Never cull when viewing from low/side angles
windGroup.add(windLines);

// Fast zero-allocation wind color lookup (matches WIND_RAMP + thermal heat blend)
function windColorR(sp, h) {
  const t = sp <= 6 ? Math.max(0, sp / 6) : Math.min(1, (sp - 6) / 6);
  const w = sp <= 6 ? 0.310 + (0.663 - 0.310) * t : 0.663 + (1.0 - 0.663) * t;
  return w * (1 - h) + 1.0 * h;
}
function windColorG(sp, h) {
  const t = sp <= 6 ? Math.max(0, sp / 6) : Math.min(1, (sp - 6) / 6);
  const w = sp <= 6 ? 0.525 + (0.847 - 0.525) * t : 0.847 + (1.0 - 0.847) * t;
  return w * (1 - h) + (0.38 + 0.45 * (1 - h)) * h;
}
function windColorB(sp, h) {
  const t = sp <= 6 ? Math.max(0, sp / 6) : Math.min(1, (sp - 6) / 6);
  const w = sp <= 6 ? 0.788 + (1.0 - 0.788) * t : 1.0;
  return w * (1 - h) + 0.08 * h;
}

// The json header carries the bounds, so nothing is read from the command's stdout.
function setWind(json) {
  const [u, v] = json;
  const h = u.header;
  const b = { SWlon: h.lo1, NElon: h.lo2, SWlat: h.la2, NElat: h.la1 };
  wind = { nx: h.nx, ny: h.ny, lo1: h.lo1, la1: h.la1, dx: h.dx, dy: h.dy, u: u.data, v: v.data, b };
  return u.data.reduce((m, x, k) => Math.max(m, Math.hypot(x, v.data[k])), 0);
}

function setWindFromU8(u8, v8, tke8 = null) {
  windU8Frame = u8; windV8Frame = v8; tkeU8Frame = tke8;
  let maxSp = 0;
  for (let i = 1; i <= 34; i++) {
    for (let j = 1; j <= 34; j++) {
      const k = i * 36 + j;
      const u = (u8[k] / 255) * 20 - 10, v = (v8[k] / 255) * 20 - 10;
      const sp = Math.hypot(u, v);
      if (sp > maxSp) maxSp = sp;
    }
  }
  return maxSp;
}

function sampleAtmo36(arr, scale, offset, X, Y) {
  const fx = ((X - D.x0) / D.lx) * 34 + 0.5;
  const fy = ((Y - D.y0) / D.ly) * 34 + 0.5;
  if (fx < 0.5 || fy < 0.5 || fx > 34.5 || fy > 34.5) return 0;
  const i = Math.min(34, Math.max(1, Math.floor(fx))), j = Math.min(34, Math.max(1, Math.floor(fy)));
  const a = fx - i, c = fy - j;
  const g = (ii, jj) => (arr[ii * 36 + jj] / 255) * scale + offset;
  return g(i, j) * (1 - a) * (1 - c) + g(i + 1, j) * a * (1 - c) + g(i, j + 1) * (1 - a) * c + g(i + 1, j + 1) * a * c;
}

function windAt(X, Y) {
  if (windU8Frame && windV8Frame && D) {
    return [sampleAtmo36(windU8Frame, 20, -10, X, Y), sampleAtmo36(windV8Frame, 20, -10, X, Y)];
  }
  if (!wind) return [0, 0];
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

function tkeAt(X, Y) {
  if (tkeU8Frame && D) return sampleAtmo36(tkeU8Frame, 10, 0, X, Y);
  return 0;
}

function respawn(k) {
  let X, Y;
  if (flatFirePts.length && k < NP * 0.58) {
    const pick = flatFirePts[(k * 7) % flatFirePts.length];
    const span = k < NP * 0.28 ? 2200 : 5400;
    X = pick[0] - 350 + (Math.random() - 0.5) * span;
    Y = pick[1] + 250 + (Math.random() - 0.5) * span;
    X = Math.min(D.x0 + D.lx, Math.max(D.x0, X));
    Y = Math.min(D.y0 + D.ly, Math.max(D.y0, Y));
  } else {
    X = D.x0 + Math.random() * D.lx;
    Y = D.y0 + Math.random() * D.ly;
  }
  const Z = altitudeAt(X, Y) + 36;
  part.x[k] = X; part.y[k] = Y; part.z[k] = Z;
  part.heat[k] = 0;
  part.age[k] = 0;
  part.life[k] = 2.6 + Math.random() * 4.2;
  part.head[k] = 0;
  const sx = (X - D.cx) * S, sy = Z * S, sz = -(Y - D.cy) * S;
  const base = k * TRAIL;
  for (let s = 0; s < TRAIL; s++) {
    part.sx[base + s] = sx; part.sy[base + s] = sy; part.sz[base + s] = sz;
    part.cr[base + s] = 0.31; part.cg[base + s] = 0.52; part.cb[base + s] = 0.79;
  }
}

function stepWind(dt) {
  if ((!wind && !windU8Frame) || !D || !windGroup.visible) return;
  const pos = windGeo.attributes.position.array, col = windGeo.attributes.color.array;
  const nFire = flatFirePts.length;
  const fStride = Math.max(1, (nFire / 40) | 0);

  for (let k = 0; k < NP; k++) {
    if (part.life[k] === 0) respawn(k);
    let X = part.x[k], Y = part.y[k], Z = part.z[k], heat = part.heat[k];
    const ground = altitudeAt(X, Y);
    const agl = Math.max(0, Z - ground);
    let [u, v] = windAt(X, Y);

    // Nearest active fire node for convective indraft & updraft coupling
    let dFire = Infinity, fx = X, fy = Y;
    for (let i = 0; i < nFire; i += fStride) {
      const dx = flatFirePts[i][0] - X, dy = flatFirePts[i][1] - Y;
      const d2 = dx * dx + dy * dy;
      if (d2 < dFire) { dFire = d2; fx = flatFirePts[i][0]; fy = flatFirePts[i][1]; }
    }
    dFire = Math.sqrt(dFire);

    const tke = tkeAt(X, Y);
    let vz = 0;

    // 1) Low-level fire indraft: surface streamlines bend and accelerate into the flame front
    if (dFire > 35 && dFire < 950 && agl < 260) {
      const pull = 3.8 * Math.exp(-dFire / 380) * (1 - agl / 260);
      u += ((fx - X) / dFire) * pull;
      v += ((fy - Y) / dFire) * pull;
    }

    // 2) Convective updraft in the fire front & high-TKE plume core
    const inCore = dFire < 340 || tke > 1.1;
    if (inCore || heat > 0.08) {
      const maxTop = ground + 2500;
      if (Z < maxTop) {
        const coreBoost = Math.max(Math.exp(-dFire / 220), Math.min(1, tke / 4.5));
        if (dFire < 320 && agl < 450) heat = Math.min(1, heat + dt * 1.9);
        else heat = Math.max(0, heat - dt * 0.18);
        vz = (5.5 * coreBoost + 7.5 * heat) * (1 - Math.min(0.85, agl / 2500));
        if (dFire > 20 && dFire < 450) {
          const swirl = 2.2 * heat * Math.exp(-dFire / 260);
          u += -(fy - Y) / dFire * swirl;
          v +=  (fx - X) / dFire * swirl;
        }
      } else {
        heat = Math.max(0, heat - dt * 0.35);
      }
      const shear = 1 + Math.min(0.65, agl / 1800);
      u *= shear; v *= shear;
    }

    const sp = Math.hypot(u, v, vz * 0.5);
    X += u * dt * SPEEDUP;
    Y += v * dt * SPEEDUP;
    const nextGround = altitudeAt(X, Y);
    if (vz > 0.1 || agl > 55) {
      Z = Math.max(nextGround + 36, Z + vz * dt * SPEEDUP * 0.65);
    } else {
      Z = nextGround + 36;
    }

    part.x[k] = X; part.y[k] = Y; part.z[k] = Z; part.heat[k] = heat;
    part.age[k] += dt;

    const out = X < D.x0 || Y < D.y0 || X > D.x0 + D.lx || Y > D.y0 + D.ly;
    if (out || sp < 0.05 || part.age[k] > part.life[k]) {
      respawn(k);
    } else {
      const base = k * TRAIL;
      const h = (part.head[k] + TRAIL - 1) % TRAIL;
      part.head[k] = h;
      part.sx[base + h] = (X - D.cx) * S;
      part.sy[base + h] = Z * S;
      part.sz[base + h] = -(Y - D.cy) * S;
      part.cr[base + h] = windColorR(sp, heat);
      part.cg[base + h] = windColorG(sp, heat);
      part.cb[base + h] = windColorB(sp, heat);
    }

    const lifeFade = Math.sin(Math.PI * Math.min(1, part.age[k] / part.life[k]));
    const base = k * TRAIL, h0 = part.head[k];
    let i0 = base + h0;
    for (let s = 0; s < SEG_PER_P; s++) {
      const i1 = base + ((h0 + s + 1) % TRAIL);
      const segIdx = (k * SEG_PER_P + s) * 6;
      pos[segIdx]     = part.sx[i0]; pos[segIdx + 1] = part.sy[i0]; pos[segIdx + 2] = part.sz[i0];
      pos[segIdx + 3] = part.sx[i1]; pos[segIdx + 4] = part.sy[i1]; pos[segIdx + 5] = part.sz[i1];
      const f0 = TAIL_FADE[s] * lifeFade, f1 = TAIL_FADE[s + 1] * lifeFade;
      col[segIdx]     = part.cr[i0] * f0; col[segIdx + 1] = part.cg[i0] * f0; col[segIdx + 2] = part.cb[i0] * f0;
      col[segIdx + 3] = part.cr[i1] * f1; col[segIdx + 4] = part.cg[i1] * f1; col[segIdx + 5] = part.cb[i1] * f1;
      i0 = i1;
    }
  }
  windGeo.attributes.position.needsUpdate = true;
  windGeo.attributes.color.needsUpdate = true;
}

// ---------- camera ----------

let lastFronts = [], flatFirePts = [];

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

// ---------- polling & replay ----------

let t0 = null, lastT = -Infinity, lastIsoT = -Infinity, firstFocus = true;
let lastPlume = null, lastPlumeBottom = null;
const info = { nodes: 0, fronts: 0, plume: 0, windMax: 0 };

// Snapshots for timeline scrubbing (populated live or from ForeFire/Outputs + MODEL2)
const snapshots = [];
let plumeFramesU8 = null, windUFramesU8 = null, windVFramesU8 = null, tkeFramesU8 = null;
let replayMode = false, replayTimer = null;

function formatUtcFromSec(tSec) {
  const sec = Math.round(((tSec % 86400) + 86400) % 86400);
  const hh = String(Math.floor(sec / 3600)).padStart(2, '0');
  const mm = String(Math.floor((sec % 3600) / 60)).padStart(2, '0');
  const ss = String(sec % 60).padStart(2, '0');
  return `2017-06-17 ${hh}:${mm}:${ss} UTC`;
}

function updateStatsUi() {
  $('stats').textContent = `fronts ${info.fronts}   nodes ${info.nodes}\n`
    + `plume max ${info.plume.toFixed(0)} m above ground\nwind max ${info.windMax.toFixed(1)} m/s`;
}

function syncReplayUi(idx) {
  if (!snapshots.length) return;
  $('replayBox').hidden = false;
  const s = $('timeSlider');
  s.max = String(snapshots.length - 1);
  s.value = String(idx);
  const snap = snapshots[idx];
  const dtMin = ((snap.t - snapshots[0].t) / 60).toFixed(0);
  $('replayStep').textContent = `+${dtMin} min (${idx + 1}/${snapshots.length})`;
}

function applySnapshot(idx) {
  const snap = snapshots[idx];
  if (!snap) return;
  if (!D) {
    const [x0, y0] = snap.sw, [x1, y1] = snap.ne;
    D = { x0, y0, lx: x1 - x0, ly: y1 - y0, cx: (x0 + x1) / 2, cy: (y0 + y1) / 2 };
    loadTerrain();
  }
  t0 ??= snapshots[0].t;
  lastFronts = snap.fronts;
  flatFirePts = snap.fronts.flat();
  drawFronts(snap.fronts);
  disposeChildren(isoGroup);
  for (let k = 0; k < idx; k++) {
    for (const f of snapshots[k].fronts) isoGroup.add(frontLine(f, 0xa3341c));
  }
  if (firstFocus && snap.fronts.length) { firstFocus = false; focusFire(); }
  info.fronts = snap.fronts.length;
  info.nodes = snap.fronts.reduce((s, f) => s + f.length, 0);
  $('clock').textContent = snap.iso || formatUtcFromSec(snap.t);
  $('elapsed').textContent = `t = ${snap.t.toFixed(0)} s  (+${(snap.t - t0).toFixed(0)} s)`;

  // Select matching plume & wind frame (10 s raw frames from MODEL2 or final .asc/.json)
  if (plumeFramesU8) {
    const nFrames = Math.floor(plumeFramesU8.length / (36 * 36));
    const fIdx = Math.max(0, Math.min(nFrames - 1, Math.round((snap.t - snapshots[0].t) / 10)));
    const u8 = plumeFramesU8.subarray(fIdx * 36 * 36, (fIdx + 1) * 36 * 36);
    if (windUFramesU8 && windVFramesU8) {
      const wu = windUFramesU8.subarray(fIdx * 36 * 36, (fIdx + 1) * 36 * 36);
      const wv = windVFramesU8.subarray(fIdx * 36 * 36, (fIdx + 1) * 36 * 36);
      const tk = tkeFramesU8 ? tkeFramesU8.subarray(fIdx * 36 * 36, (fIdx + 1) * 36 * 36) : null;
      info.windMax = setWindFromU8(wu, wv, tk);
    }
    if (idx === snapshots.length - 1 && lastPlume && !lastPlume.raw36) {
      info.plume = drawPlume(lastPlume, lastPlumeBottom);
    } else {
      lastPlume = { raw36: true, u8 };
      info.plume = drawPlume(lastPlume, null);
    }
  } else if (snap.plume || lastPlume) {
    info.plume = drawPlume(snap.plume || lastPlume, snap.plumeBottom || lastPlumeBottom);
  }
  syncReplayUi(idx);
  updateStatsUi();
}

async function loadSavedRun() {
  if (snapshots.length) return true;
  // Probe ForeFire/Outputs/output.0.<t> (every 60 s from 54000 to 55200)
  const times = [];
  for (let t = 54000; t <= 55200; t += 60) times.push(t);
  const dumps = await Promise.all(times.map((t) =>
    fetch(`ForeFire/Outputs/output.0.${t}`, { cache: 'no-store' })
      .then((r) => (r.ok ? r.text() : null)).catch(() => null)));
  for (const txt of dumps) {
    if (!txt) continue;
    const d = parseDump(txt);
    if (d) snapshots.push({ ...d, iso: formatUtcFromSec(d.t) });
  }
  if (!snapshots.length) return false;
  const getU8 = (p) => fetch(p, { cache: 'no-store' }).then((r) => (r.ok ? r.arrayBuffer() : null)).then((b) => (b ? new Uint8Array(b) : null)).catch(() => null);
  [plumeFramesU8, windUFramesU8, windVFramesU8, tkeFramesU8] = await Promise.all([
    getU8('MODEL2/plumeTopHeight_uint8_36_36.dat'),
    getU8('MODEL2/windU_uint8_36_36.dat'),
    getU8('MODEL2/windV_uint8_36_36.dat'),
    getU8('MODEL2/tke_uint8_36_36.dat'),
  ]);
  try { lastPlume = parseAsc(await getText('mnh3d_plumeTop.asc')); } catch {}
  try { lastPlumeBottom = parseAsc(await getText('mnh3d_plumeBottom.asc')); } catch {}
  try { info.windMax = setWind(JSON.parse(await getText('mnh3d_wind.json'))); } catch {}
  replayMode = true;
  setStatus('replay (saved run — scrub timeline)', 'ok');
  applySnapshot(snapshots.length - 1); // Jump straight to the most advanced step
  return true;
}

async function pollFront() {
  await ff('setParameter[dumpMode=ff]');
  const dump = parseDump(await ff('print[]'));
  if (!dump) throw new Error('no FireDomain in print[] (another tab may have set dumpMode)');
  if (dump.t < lastT) { location.reload(); return; } // a new run started on the same port
  lastT = dump.t;
  if (!D) {
    const [x0, y0] = dump.sw, [x1, y1] = dump.ne;
    D = { x0, y0, lx: x1 - x0, ly: y1 - y0, cx: (x0 + x1) / 2, cy: (y0 + y1) / 2 };
    loadTerrain();
  }
  t0 ??= dump.t;
  lastFronts = dump.fronts;
  flatFirePts = dump.fronts.flat();
  drawFronts(dump.fronts);
  if (dump.t - lastIsoT >= ISO_STEP && dump.fronts.length) {
    lastIsoT = dump.t;
    for (const f of dump.fronts) isoGroup.add(frontLine(f, 0xa3341c));
    while (isoGroup.children.length > MAX_ISO) {
      const old = isoGroup.children[0];
      isoGroup.remove(old); old.geometry.dispose(); old.material.dispose();
    }
  }
  if (firstFocus && dump.fronts.length) { firstFocus = false; focusFire(); }
  info.fronts = dump.fronts.length;
  info.nodes = dump.fronts.reduce((s, f) => s + f.length, 0);
  const iso = (await ff('getParameter[ISOdate]')).trim();
  const isoFmt = iso.replace('T', ' ').replace('Z', ' UTC');
  $('clock').textContent = isoFmt;
  $('elapsed').textContent = `t = ${dump.t.toFixed(0)} s  (+${(dump.t - t0).toFixed(0)} s on this page)`;
  if (!snapshots.length || dump.t - snapshots[snapshots.length - 1].t >= 30) {
    snapshots.push({ ...dump, iso: isoFmt, plume: lastPlume, plumeBottom: lastPlumeBottom });
    syncReplayUi(snapshots.length - 1);
  }
}

async function pollFields() {
  const ny = Math.max(2, Math.round(128 * D.ly / D.lx));
  await ff(`plot[parameter=plumeTopHeight;filename=mnh3d_plumeTop.asc;size=128,${ny}]`);
  lastPlume = parseAsc(await getText('mnh3d_plumeTop.asc'));
  try {
    await ff(`plot[parameter=plumeBottomHeight;filename=mnh3d_plumeBottom.asc;size=128,${ny}]`);
    lastPlumeBottom = parseAsc(await getText('mnh3d_plumeBottom.asc'));
  } catch {}
  info.plume = drawPlume(lastPlume, lastPlumeBottom);
  if (snapshots.length) {
    snapshots[snapshots.length - 1].plume = lastPlume;
    snapshots[snapshots.length - 1].plumeBottom = lastPlumeBottom;
  }
  await ff('plot[parameter=wind;filename=mnh3d_wind.json;size=48,48]');
  info.windMax = setWind(JSON.parse(await getText('mnh3d_wind.json')));
}

async function loop() {
  for (let i = 0; ; i++) {
    if (!replayMode) {
      try {
        await pollFront();
        if (i % FIELDS_EVERY === 0) await pollFields();
        setStatus('live', 'ok');
        $('liveBtn').hidden = true;
      } catch (e) {
        const loaded = await loadSavedRun();
        if (!loaded) {
          setStatus(e.offline ? 'no server (run ended or not started)' : `error: ${e.message}`, 'err');
        }
      }
      updateStatsUi();
    }
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

function stopPlay() {
  if (replayTimer) { clearInterval(replayTimer); replayTimer = null; }
  $('playBtn').textContent = '▶ Play';
}

$('timeSlider').addEventListener('input', (e) => {
  stopPlay();
  replayMode = true;
  $('liveBtn').hidden = false;
  setStatus('replay (paused)', 'ok');
  applySnapshot(Number(e.target.value));
});

$('playBtn').addEventListener('click', () => {
  if (!snapshots.length) return;
  if (replayTimer) { stopPlay(); return; }
  replayMode = true;
  $('liveBtn').hidden = false;
  setStatus('replay (playing)', 'ok');
  $('playBtn').textContent = '⏸ Pause';
  let idx = Number($('timeSlider').value);
  if (idx >= snapshots.length - 1) idx = 0;
  applySnapshot(idx);
  replayTimer = setInterval(() => {
    idx++;
    if (idx >= snapshots.length) { stopPlay(); setStatus('replay (end of run)', 'ok'); return; }
    applySnapshot(idx);
  }, 450);
});

$('liveBtn').addEventListener('click', () => {
  stopPlay();
  replayMode = false;
  $('liveBtn').hidden = true;
});

const clock = new THREE.Clock();
renderer.setAnimationLoop(() => {
  const dt = Math.min(clock.getDelta(), 0.05);
  stepWind(dt);
  updatePlumePuffs(dt);
  controls.update();
  renderer.render(scene, camera);
});

loop();
