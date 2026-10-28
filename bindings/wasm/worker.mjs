/**
 * Simulation worker for the browser demo (issue #171).
 *
 * A whole run costs a few hundred milliseconds for a modest fire and a few
 * seconds for one that burns eight hundred hectares, so the worker computes
 * the entire timeline up front and posts it in one go; the page then scrubs
 * through frames rather than waiting on the solver. Running it here rather
 * than on the main thread is what keeps the page answering during that.
 *
 * The fuel map and terrain arrive from the main thread, which is where they
 * have to be built: decoding map tiles needs a canvas, and workers have no DOM.
 *
 * Each worker runs exactly one simulation. The engine's state is process-wide
 * (#175): a second FireDomain in the same module keeps the first one's
 * propagation model, so changing the model between runs would silently do
 * nothing. A fresh worker is a fresh module, and costs a few tens of ms.
 */

import createForeFire from "./forefire.mjs";

let Module = null;

// The parameters from tests/runff/params.ff. The page overrides the ones its
// research panel exposes.
const SOLVER = {
	spatialIncrement: 3,
	minimalPropagativeFrontDepth: 20,
	perimeterResolution: 10,
	initialFrontDepth: 0.1,
	relax: 0.5,
	smoothing: 0,
	minSpeed: 0.009,
	bmapLayer: 1,
	windReductionFactor: 0.4,
	propagationSpeedAdjustmentFactor: 0.6,
};

function build(land, model, solver) {
	const { side, nx, ny } = land;
	const ff = new Module.ForeFire();

	ff.setString("fuelsTable", land.fuelsTable);
	ff.setDouble("defaultFuelType", land.defaultFuel);
	for (const [key, value] of Object.entries(solver)) ff.setDouble(key, value);
	ff.setString("propagationModel", model);

	ff.execute(`FireDomain[sw=(0,0,0);ne=(${side},${side},0);t=0]`);

	const fuel = land.fuel ?? new Int32Array(nx * ny).fill(land.defaultFuel);
	// Two-plane wind layers: plane 0 carries the u response, plane 1 the v.
	const windU = new Float64Array(2 * nx * ny);
	windU.fill(1, 0, nx * ny);
	const windV = new Float64Array(2 * nx * ny);
	windV.fill(1, nx * ny);

	const S = side;
	// Altitude first, and before the propagation layer: registering a layer
	// named "altitude" is what makes the DataBroker derive the slope layer, and
	// a propagation model that asks for "slope" before one exists gets a
	// constant zero-altitude stand-in that never updates.
	if (land.altitude)
		ff.addScalarLayer("data", "altitude", 0, 0, 0, S, S, 0, nx, ny, 1, 1, land.altitude);

	const ok = [
		ff.addLayer("propagation", model, "propagationModel"),
		ff.addIndexLayer("table", "fuel", 0, 0, 0, S, S, 0, nx, ny, 1, 1, fuel),
		ff.addScalarLayer("windScalDir", "windU", 0, 0, 0, S, S, 0, nx, ny, 2, 1, windU),
		ff.addScalarLayer("windScalDir", "windV", 0, 0, 0, S, S, 0, nx, ny, 2, 1, windV),
	];
	if (ok.some((v) => !v)) throw new Error("a layer failed to register");
	return ff;
}

/*! Split print[] into one coordinate array per front, with each node's
 *  rate of spread alongside.
 *
 *  Fronts nest — an inner front is printed indented inside its parent — so
 *  every FireFront line starts a fresh polygon. Running them together is what
 *  draws spurious chords straight across the burn. The speed is the length of
 *  the node's `vel`, in m/s.
 */
function parseFronts(text) {
	const fronts = [];
	let current = null;
	for (const line of text.split("\n")) {
		if (line.includes("FireFront[")) {
			current = { coords: [], speeds: [] };
			fronts.push(current);
		} else if (current && line.includes("FireNode[")) {
			const loc = /loc=\(([-\d.eE+]+),([-\d.eE+]+),/.exec(line);
			const vel = /vel=\(([-\d.eE+]+),([-\d.eE+]+),/.exec(line);
			if (loc) {
				current.coords.push(parseFloat(loc[1]), parseFloat(loc[2]));
				current.speeds.push(vel ? Math.hypot(parseFloat(vel[1]), parseFloat(vel[2])) : 0);
			}
		}
	}
	// A front of one or two nodes is a degenerate sliver, not a perimeter.
	const kept = fronts.filter((f) => f.coords.length >= 6);
	return {
		fronts: kept.map((f) => Float32Array.from(f.coords)),
		speeds: kept.map((f) => Float32Array.from(f.speeds)),
	};
}

/*! Shoelace area, summed over fronts. Inner fronts wind the other way, so
 *  signed areas cancel and holes are subtracted for free. */
function burntArea(fronts) {
	let total = 0;
	for (const f of fronts) {
		let a = 0;
		for (let i = 0, n = f.length; i < n; i += 2) {
			const j = (i + 2) % n;
			a += f[i] * f[j + 1] - f[j] * f[i + 1];
		}
		total += a / 2;
	}
	return Math.abs(total);
}

/*! Did this front run into the edge of the domain? */
function touchesEdge(fronts, side, margin) {
	for (const f of fronts)
		for (let i = 0; i < f.length; i += 2)
			if (
				f[i] < margin ||
				f[i] > side - margin ||
				f[i + 1] < margin ||
				f[i + 1] > side - margin
			)
				return true;
	return false;
}

/*!
 * Run the whole timeline and return it, along with how it ended.
 *
 * A fire that stops does not linger as a stationary perimeter: ForeFire
 * discards the front and print[] comes back with nothing at all, which on a
 * canvas looks identical to a bug. So the end of the run is detected on the
 * front list emptying, the empty frame is dropped, and the cause is reported —
 * a front that was touching the domain boundary left the map, anything else
 * burnt out. A slower death, where the perimeter survives but stops advancing,
 * is caught by the area-plateau test below.
 */
function simulate({ land, model, params, steps, dt, wind, ignition }) {
	const solver = { ...SOLVER, ...params };
	const ff = build(land, model, solver);
	const [ix, iy] = ignition;
	ff.execute(`startFire[loc=(${ix},${iy},0.);t=0]`);

	const frames = [];
	const started = performance.now();
	let outcome = { kind: "running" };
	// The plateau test is judged over a window, not step to step: a young
	// Rothermel fire adds only a few hundred square metres per step, which is
	// indistinguishable from noise one step at a time.
	const WINDOW = 12;

	const push = (step, ms) => {
		const { fronts, speeds } = parseFronts(ff.execute("print[]"));
		frames.push({
			step,
			t: ff.getTime(),
			ms,
			fronts,
			speeds,
			nodes: fronts.reduce((n, f) => n + f.length / 2, 0),
			area: burntArea(fronts),
			windDeg: windAt(step, wind).deg,
		});
	};
	push(0, 0);

	for (let step = 1; step <= steps; step++) {
		const t0 = performance.now();
		const { vx, vy } = windAt(step, wind);
		ff.execute(`trigger[wind;loc=(0.,0.,0.);vel=(${vx.toFixed(4)},${vy.toFixed(4)},0)]`);
		ff.execute(`step[dt=${dt}]`);
		push(step, performance.now() - t0);

		if (frames.at(-1).fronts.length === 0) {
			const last = frames.at(-2);
			frames.pop(); // nothing to draw, and it would blank the canvas
			outcome = {
				kind:
					last && touchesEdge(last.fronts, land.side, 3 * solver.perimeterResolution)
						? "left-domain"
						: "burnt-out",
				t: last ? last.t : 0,
			};
			break;
		}

		if (frames.length > WINDOW) {
			const now = frames.at(-1).area;
			const then = frames.at(-1 - WINDOW).area;
			// Stalled when the burnt area has stopped growing in any meaningful
			// sense — under half a percent over the window, and under one fuel
			// cell's worth of ground.
			const cell = (land.side / land.nx) ** 2;
			if (now - then < Math.max(cell, 0.005 * then)) {
				outcome = { kind: "stalled", t: frames.at(-1 - WINDOW).t };
				break;
			}
		}
		if (step % 25 === 0)
			self.postMessage({ type: "progress", done: step, total: steps });
	}

	if (outcome.kind === "running") outcome = { kind: "step-limit", t: frames.at(-1).t };
	// An ignition on bare ground never becomes a fire at all, which is worth
	// saying plainly rather than reporting as a fire that stopped.
	const peak = Math.max(...frames.map((f) => f.area));
	if (outcome.kind === "stalled" && peak < 10 * (land.side / land.nx) ** 2)
		outcome = { kind: "never-caught", t: outcome.t };

	return {
		frames,
		outcome,
		wallMs: performance.now() - started,
		domain: land.side,
		peakArea: peak,
	};
}

function windAt(step, wind) {
	// Wrapped, or a turning run reports "864°" after an hour of simulated time.
	const deg = (wind.turning ? wind.angleDeg + step * wind.turnPerStep : wind.angleDeg) % 360;
	const rad = (deg * Math.PI) / 180;
	return { vx: wind.speed * Math.cos(rad), vy: wind.speed * Math.sin(rad), deg };
}

self.onmessage = ({ data: msg }) => {
	if (msg.type === "simulate") {
		try {
			const result = simulate(msg.run);
			self.postMessage(
				{ type: "timeline", ...result },
				result.frames.flatMap((f) => [...f.fronts, ...f.speeds].map((c) => c.buffer))
			);
		} catch (err) {
			self.postMessage({ type: "failed", message: String(err && err.message) });
		}
	}
};

Module = await createForeFire();
self.postMessage({ type: "ready", version: Module.version() });
