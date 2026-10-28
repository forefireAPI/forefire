/**
 * Builds a ForeFire landscape out of public map tiles, in the browser.
 *
 * There is no NetCDF here and no GIS stack. Satellite tiles go onto a canvas
 * and their pixels are classified into fuel indices; terrain tiles go onto a
 * second canvas and decode to metres. The two arrays that come out are exactly
 * what addIndexLayer and addScalarLayer take.
 *
 * Both are load-bearing. Rothermel's rate of spread reads the fuel bed and the
 * slope, so the classified imagery stops the fire at clearings and roads, and
 * the terrain makes it run uphill: on a bare slope with no wind at all, spread
 * grows from 47 m to 210 m over ten minutes between flat ground and a 60%
 * grade, while the downhill side stays put.
 */

const TILE = 256;
const IMAGERY =
	"https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile";
// Terrarium-encoded SRTM/NED, elevation = r*256 + g + b/256 - 32768 metres.
const TERRAIN = "https://s3.amazonaws.com/elevation-tiles-prod/terrarium";

/*! The three fuel classes, as rows of tests/runff/fuels.csv — the repository's
 *  own Rothermel table, not invented numbers. Index 0 has a fuel bed depth of
 *  zero, which is what makes it a barrier. */
const FUEL_ROWS = {
	5: "5;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3",
	4: "4;613.0;538.0;0.1;1.0;4357.0;6524.0;0.19;1.286;0.085;8.3;1.0;300;70000;18677000.0;18677000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3",
	0: "0;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3",
};
const FUEL_HEADER =
	"Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me";

export const FUELS = [
	{ index: 5, label: "shrub / maquis", note: "depth 0.6 m", rgb: [46, 125, 50] },
	{ index: 4, label: "sparse / grass", note: "depth 0.19 m", rgb: [190, 180, 70] },
	{ index: 0, label: "bare, built, water", note: "no fuel bed", rgb: [110, 100, 95] },
];

export const FUELS_TABLE = [FUEL_HEADER, ...FUELS.map((f) => FUEL_ROWS[f.index])].join("\n");

/*! The defaults below are the values in tests/runff/fuels.csv. */
export const FUEL_DEFAULTS = { deadMoisture: 0.1, liveMoisture: 1.0, depth: { 5: 0.6, 4: 0.19 } };

/*!
 * The fuel table with dead and live moisture (Md, Ml) and the burnable classes'
 * bed depth (e) overridden. Index 0 keeps its zero depth: it is the barrier.
 */
export function fuelsTable({ deadMoisture, liveMoisture, depth }) {
	const columns = FUEL_HEADER.split(";");
	const col = (name) => columns.indexOf(name);
	const rows = FUELS.map((f) => {
		const cells = FUEL_ROWS[f.index].split(";");
		cells[col("Md")] = String(deadMoisture);
		cells[col("Ml")] = String(liveMoisture);
		if (depth[f.index] !== undefined) cells[col("e")] = String(depth[f.index]);
		return cells.join(";");
	});
	return [FUEL_HEADER, ...rows].join("\n");
}

/*! Excess-green cut-offs between the classes; see classify(). */
export const CLASS_DEFAULTS = { shrub: 20, grass: 10 };

/*!
 * Fuel indices from the per-cell excess green `g - (r+b)/2`. Excess green
 * separates vegetation from rock, road and roof about as well as anything this
 * cheap can; water lands below both cuts.
 */
export function classify(greenness, { shrub, grass }) {
	const fuel = new Int32Array(greenness.length);
	const counts = { 5: 0, 4: 0, 0: 0 };
	for (let i = 0; i < greenness.length; i++) {
		const g = greenness[i];
		const index = g >= shrub ? 5 : g >= grass ? 4 : 0;
		fuel[i] = index;
		counts[index]++;
	}
	return { fuel, counts };
}

/*! Web Mercator: fractional pixel coordinates at a given zoom. */
function project(lon, lat, zoom) {
	const n = 2 ** zoom;
	const rad = (lat * Math.PI) / 180;
	return [
		((lon + 180) / 360) * n * TILE,
		((1 - Math.log(Math.tan(rad) + 1 / Math.cos(rad)) / Math.PI) / 2) * n * TILE,
	];
}

/*! Ground resolution in metres per pixel, which is what ties the tile grid to
 *  the simulation's Cartesian metres. */
function resolution(lat, zoom) {
	return (156543.03392804097 * Math.cos((lat * Math.PI) / 180)) / 2 ** zoom;
}

/*! Metres per degree, near enough for panning a few kilometres. */
export const M_PER_DEG_LAT = 111320;
export const mPerDegLon = (lat) => 111320 * Math.cos((lat * Math.PI) / 180);

export const SIDE_LIMITS = { min: 500, max: 30000 };

/*!
 * Pick a tile zoom for a domain of `side` metres.
 *
 * Chosen so the mosaic stays around a thousand pixels whatever the domain
 * size: the tile count per fetch stays roughly constant, and the imagery keeps
 * pace with the ground it covers. Capped at 15, past which the terrain tiles
 * are upsampled rather than more detailed.
 */
export function zoomFor(lat, side, targetPx = 1024) {
	const ideal = Math.log2((156543.03392804097 * Math.cos((lat * Math.PI) / 180) * targetPx) / side);
	return Math.max(10, Math.min(15, Math.round(ideal)));
}

function loadImage(url) {
	return new Promise((resolve, reject) => {
		const img = new Image();
		// Required to read the pixels back out. Both hosts send
		// Access-Control-Allow-Origin, so the canvas stays untainted.
		img.crossOrigin = "anonymous";
		img.onload = () => resolve(img);
		img.onerror = () => reject(new Error(`tile ${url} failed`));
		img.src = url;
	});
}

/*! Fetch a tiled layer into one canvas covering the square. `url` differs per
 *  host: Esri numbers its path z/row/col, the terrain tiles z/x/y. */
async function mosaic(url, { zoom, left, top, sidePx }) {
	const canvas = document.createElement("canvas");
	canvas.width = canvas.height = sidePx;
	const ctx = canvas.getContext("2d");
	const jobs = [];
	for (let ty = Math.floor(top / TILE); ty <= Math.floor((top + sidePx) / TILE); ty++)
		for (let tx = Math.floor(left / TILE); tx <= Math.floor((left + sidePx) / TILE); tx++)
			jobs.push(
				loadImage(url(zoom, tx, ty)).then((img) =>
					ctx.drawImage(img, tx * TILE - left, ty * TILE - top)
				)
			);
	await Promise.all(jobs);
	return canvas;
}

/*! Resample a canvas to the simulation grid and hand back its pixels. */
function resample(canvas, grid) {
	const small = document.createElement("canvas");
	small.width = small.height = grid;
	const ctx = small.getContext("2d");
	// Nearest neighbour for the DEM: averaging terrarium's packed bytes across
	// pixels is meaningless, since the red channel carries 256 m per unit.
	ctx.imageSmoothingEnabled = false;
	ctx.drawImage(canvas, 0, 0, grid, grid);
	return ctx.getImageData(0, 0, grid, grid).data;
}

/*!
 * Fetch the imagery and terrain covering a `side`-metre square centred on
 * (lon, lat), and derive a fuel map and an elevation field from them.
 *
 * Returns the display canvas plus the two layer arrays, both in the layers'
 * (t,z,y,x) order with row 0 at the domain's south edge. Throws if the tiles
 * cannot be fetched, so the caller can fall back.
 */
export async function buildLandscape({ lon, lat, side, grid, zoom = zoomFor(lat, side) }) {
	const metresPerPx = resolution(lat, zoom);
	const sidePx = Math.round(side / metresPerPx);
	const [cx, cy] = project(lon, lat, zoom);
	// Whole pixels: a tile drawn at a fractional offset is antialiased along
	// its edges, which shows as a seam in the imagery and, in the terrain,
	// decodes the half-blended edge pixels to spikes hundreds of metres high.
	const box = { zoom, left: Math.round(cx - sidePx / 2), top: Math.round(cy - sidePx / 2), sidePx };

	const [image, terrain] = await Promise.all([
		mosaic((z, x, y) => `${IMAGERY}/${z}/${y}/${x}`, box),
		mosaic((z, x, y) => `${TERRAIN}/${z}/${x}/${y}.png`, box),
	]);

	const rgb = resample(image, grid);
	const dem = resample(terrain, grid);

	const greenness = new Float32Array(grid * grid);
	const altitude = new Float64Array(grid * grid);
	let lo = Infinity;
	let hi = -Infinity;

	for (let row = 0; row < grid; row++) {
		// Image rows run north to south; the layers' row 0 is the domain's
		// south edge, so the vertical order flips here.
		const y = grid - 1 - row;
		for (let x = 0; x < grid; x++) {
			const i = (row * grid + x) * 4;

			greenness[x + grid * y] = rgb[i + 1] - (rgb[i] + rgb[i + 2]) / 2;

			const metres = dem[i] * 256 + dem[i + 1] + dem[i + 2] / 256 - 32768;
			altitude[x + grid * y] = metres;
			if (metres < lo) lo = metres;
			if (metres > hi) hi = metres;
		}
	}

	// Kept alongside the fuel map so the class cut-offs can move without
	// fetching the tiles again.
	const { fuel, counts } = classify(greenness, CLASS_DEFAULTS);

	return {
		image,
		greenness,
		fuel,
		altitude,
		nx: grid,
		ny: grid,
		counts,
		metresPerPx,
		sidePx,
		zoom,
		// The square this landscape actually covers, which the page needs to
		// keep straight from the square the user is currently looking at.
		site: { lon, lat, side },
		elevation: { lo, hi },
	};
}

/*! A translucent raster of the fuel classes, drawn at simulation resolution and
 *  scaled up by the caller. */
export function fuelOverlay(fuel, grid) {
	const canvas = document.createElement("canvas");
	canvas.width = canvas.height = grid;
	const ctx = canvas.getContext("2d");
	const img = ctx.createImageData(grid, grid);
	const palette = Object.fromEntries(FUELS.map((f) => [f.index, f.rgb]));
	for (let row = 0; row < grid; row++) {
		const y = grid - 1 - row; // back to image order
		for (let x = 0; x < grid; x++) {
			const [r, g, b] = palette[fuel[x + grid * y]];
			const i = (row * grid + x) * 4;
			img.data[i] = r;
			img.data[i + 1] = g;
			img.data[i + 2] = b;
			img.data[i + 3] = 255;
		}
	}
	ctx.putImageData(img, 0, 0);
	return canvas;
}

/*! Relief shading from the elevation field, as a multiply layer over the
 *  imagery. Satellite pictures taken near local noon carry almost no shadow,
 *  so without this the terrain is invisible in plan view. */
export function hillshade(altitude, grid, cellMetres, { azimuth = 315, altitudeAngle = 40 } = {}) {
	const canvas = document.createElement("canvas");
	canvas.width = canvas.height = grid;
	const ctx = canvas.getContext("2d");
	const img = ctx.createImageData(grid, grid);

	const az = ((360 - azimuth + 90) * Math.PI) / 180;
	const alt = (altitudeAngle * Math.PI) / 180;
	const at = (x, y) =>
		altitude[Math.min(grid - 1, Math.max(0, x)) + grid * Math.min(grid - 1, Math.max(0, y))];

	for (let row = 0; row < grid; row++) {
		const y = grid - 1 - row;
		for (let x = 0; x < grid; x++) {
			const dzdx = (at(x + 1, y) - at(x - 1, y)) / (2 * cellMetres);
			const dzdy = (at(x, y + 1) - at(x, y - 1)) / (2 * cellMetres);
			const slope = Math.atan(Math.hypot(dzdx, dzdy));
			const aspect = Math.atan2(dzdy, -dzdx);
			let shade =
				Math.cos(alt) * Math.cos(slope) +
				Math.sin(alt) * Math.sin(slope) * Math.cos(az - aspect);
			shade = Math.max(0, Math.min(1, shade));
			// Rendered as black at varying alpha, to be composited with
			// "multiply" — mid-grey means no change, so alpha tracks how far
			// the shade falls below neutral.
			const i = (row * grid + x) * 4;
			img.data[i] = img.data[i + 1] = img.data[i + 2] = 0;
			img.data[i + 3] = Math.round(255 * Math.max(0, 0.85 - shade) * 0.9);
		}
	}
	ctx.putImageData(img, 0, 0);
	return canvas;
}
