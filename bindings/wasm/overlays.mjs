/**
 * Reference layers for reading the map: the DFCI grid, water points and DFCI
 * tracks, from the IGN Géoplateforme (France only, Licence Ouverte 2.0).
 *
 * They are drawn and nothing else. The simulation never sees them: no fuel, no
 * barrier, nothing goes to the worker. Their job is to let someone name a place
 * on the map the way a firefighter would.
 *
 * Nothing is fetched until a layer is switched on, so a visitor who never asks
 * for one makes no request to IGN.
 */

const WFS = "https://data.geopf.fr/wfs/ows";
// WFS CQL takes south,west,north,east in this CRS, and answers lon,lat.
const CRS = "urn:ogc:def:crs:EPSG::4326";
const FRANCE = { west: -5.5, south: 41, east: 10, north: 51.5 };
// The 1 km grid is only readable on a small domain.
const FINE_GRID_MAX_SIDE = 8000;

// The server caps a response at 5000 features and says nothing about the rest, so
// a dense area has to be read page by page or it comes back cut off.
const PAGE = 5000;

async function features(layer, cql, properties) {
	const all = [];
	for (let start = 0; ; start += PAGE) {
		const query = new URLSearchParams({
			SERVICE: "WFS",
			VERSION: "2.0.0",
			REQUEST: "GetFeature",
			OUTPUTFORMAT: "application/json",
			SRSNAME: "EPSG:4326",
			TYPENAMES: layer,
			COUNT: PAGE,
			STARTINDEX: start,
			cql_filter: cql,
		});
		if (properties) query.set("PROPERTYNAME", properties);
		const response = await fetch(`${WFS}?${query}`);
		if (!response.ok) throw new Error(`${layer}: HTTP ${response.status}`);
		const page = (await response.json()).features;
		all.push(...page);
		if (page.length < PAGE) return all; // a short page is the last one
	}
}

const box = (field, b) => `BBOX(${field},${b.south},${b.west},${b.north},${b.east},'${CRS}')`;

/*! Each layer: how to fetch it for a box, and what to keep. The key is the id of
 *  the page's chip. */
export const REFERENCE = {
	showGrid: async (b, side) => {
		const size = side <= FINE_GRID_MAX_SIDE ? "1x1" : "2x2";
		const found = await features(`GEOGRAPHICALGRIDSYSTEM.DFCI:carro_dfci_${size}_l93`, box("the_geom", b));
		return found.map((f) => ({ name: f.properties.nom, rings: f.geometry.coordinates.flat() }));
	},
	showWater: async (b) => {
		const cql = `nature IN ('Citerne','Point d''eau') AND ${box("geometrie", b)}`;
		const found = await features("BDTOPO_V3:detail_hydrographique", cql, "nature,geometrie");
		return found.map((f) => ({ cistern: f.properties.nature === "Citerne", at: f.geometry.coordinates }));
	},
	showTracks: async (b) => {
		const cql = `piste_dfci=true AND ${box("geometrie", b)}`;
		const found = await features("BDTOPO_V3:troncon_de_route", cql, "cpx_numero,geometrie");
		return found.map((f) => ({ number: f.properties.cpx_numero, line: f.geometry.coordinates }));
	},
};

export function inFrance(b) {
	return b.east > FRANCE.west && b.west < FRANCE.east && b.north > FRANCE.south && b.south < FRANCE.north;
}

function path(ctx, points, project) {
	ctx.beginPath();
	points.forEach(([lon, lat], i) => {
		const [x, y] = project(lon, lat);
		i ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
	});
}

function halo(ctx, text, x, y) {
	ctx.strokeText(text, x, y);
	ctx.fillText(text, x, y);
}

/*! `project` takes lon, lat to canvas pixels. */
export function drawReference(ctx, layers, project) {
	ctx.save();
	ctx.font = "600 11px ui-monospace, Menlo, monospace";
	ctx.textAlign = "center";
	ctx.lineWidth = 3;
	ctx.lineJoin = "round";

	for (const track of layers.showTracks ?? []) {
		path(ctx, track.line, project);
		ctx.strokeStyle = "#ffd166";
		ctx.lineWidth = 1.5;
		ctx.stroke();
	}

	for (const cell of layers.showGrid ?? []) {
		let first = null;
		ctx.strokeStyle = "rgba(255,255,255,0.75)";
		ctx.lineWidth = 1;
		for (const ring of cell.rings) {
			path(ctx, ring, project);
			ctx.stroke();
			first ??= ring;
		}
		const corners = first.map(([lon, lat]) => project(lon, lat));
		const xs = corners.map((c) => c[0]);
		const ys = corners.map((c) => c[1]);
		const wide = Math.max(...xs) - Math.min(...xs);
		if (wide < 58) continue; // a label wider than its square is noise
		ctx.lineWidth = 3;
		ctx.strokeStyle = "rgba(0,0,0,0.75)";
		ctx.fillStyle = "#fff";
		halo(ctx, cell.name, (Math.max(...xs) + Math.min(...xs)) / 2, (Math.max(...ys) + Math.min(...ys)) / 2);
	}

	for (const point of layers.showWater ?? []) {
		const [x, y] = project(...point.at);
		ctx.fillStyle = point.cistern ? "#29b6f6" : "#b3e5fc";
		ctx.strokeStyle = "#04293a";
		ctx.lineWidth = 1.5;
		ctx.beginPath();
		// A cistern is a square, any other water point a circle, so the two read
		// apart without a legend's colours having to carry it.
		if (point.cistern) ctx.rect(x - 4, y - 4, 8, 8);
		else ctx.arc(x, y, 4, 0, 2 * Math.PI);
		ctx.fill();
		ctx.stroke();
	}
	ctx.restore();
}
