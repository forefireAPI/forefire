/**
 * @file forefire_wasm.cpp
 * @brief Emscripten/embind entry point for the browser build.
 *
 * A spike for issue #171. The shape deliberately mirrors the pybind11 module
 * in bindings/python: one Command per instance, commands driven as strings,
 * gridded inputs handed over as typed arrays instead of NumPy arrays.
 *
 * NetCDF is disabled. Landscapes can arrive as arrays or as compressed
 * .fftile files staged by the asynchronous browser transport into MEMFS.
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <sstream>
#include <string>
#include <vector>

#include "Command.h"
#include "FireDomain.h"
#include "fftile.h"
#include <stdexcept>
#include <cmath>
#include "include/Version.h"

using namespace libforefire;

namespace {

/*! \brief Copy a JS TypedArray into a contiguous C++ vector. */
template <typename T>
std::vector<T> toVector(const emscripten::val &array) {
	const size_t length = array["length"].as<size_t>();
	std::vector<T> out(length);
	if (length == 0) return out;
	// A view over the heap slice the vector already owns; the JS side does the
	// copy, which avoids a per-element val round trip.
	emscripten::val heap = emscripten::val(emscripten::typed_memory_view(length, out.data()));
	heap.call<void>("set", array);
	return out;
}

/*! \brief Transpose a (t,z,y,x) C-order buffer into the x-major order the data
 *         layers index with.
 *
 *  XYZTDataLayer and FuelDataLayer read their values as
 *  `x*(ny*nz*nt) + y*(nz*nt) + z*nt + t`, not as the row-major layout a caller
 *  naturally builds. The pybind11 binding does this same shuffle before
 *  handing NumPy data over; skipping it silently transposes the field, which
 *  for a two-plane wind layer swaps the u and v responses and sends the fire
 *  off at ninety degrees to the wind.
 */
template <typename T>
std::vector<T> toLayerOrder(const std::vector<T> &src, size_t nx, size_t ny,
							size_t nz, size_t nt) {
	std::vector<T> out(src.size());
	for (size_t it = 0; it < nt; ++it)
		for (size_t iz = 0; iz < nz; ++iz)
			for (size_t iy = 0; iy < ny; ++iy)
				for (size_t ix = 0; ix < nx; ++ix) {
					const size_t from = ix + nx * (iy + ny * (iz + nz * it));
					const size_t to = ix * (ny * nz * nt) + iy * (nz * nt) + iz * nt + it;
					out[to] = src[from];
				}
	return out;
}

class ForeFireWasm {
	Command executor;

public:
	ForeFireWasm() : executor() {}

	/*! \brief Run one ForeFire command, returning whatever it printed. */
	std::string execute(const std::string &command) {
		std::ostringstream captured;
		executor.setOstringstream(&captured);
		std::string cmd(command);
		executor.ExecuteCommand(cmd);
		return captured.str();
	}

	/*! \brief Current front geometry, in the shell's own dump format. */
	std::string dumpString() { return executor.dumpString(); }

	double getTime() { return executor.getTime(); }

	void setInt(const std::string &name, int value) {
		executor.currentSession.params->setInt(name, value);
	}
	int getInt(const std::string &name) {
		return executor.currentSession.params->getInt(name);
	}
	void setDouble(const std::string &name, double value) {
		executor.currentSession.params->setDouble(name, value);
	}
	double getDouble(const std::string &name) {
		return executor.currentSession.params->getDouble(name);
	}
	void setString(const std::string &name, const std::string &value) {
		executor.currentSession.params->setParameter(name, value);
	}
	std::string getString(const std::string &name) {
		return executor.currentSession.params->getParameter(name);
	}

	/*! \brief Attach a model-backed layer, e.g. ("propagation", "WindDriven",
	 *         "propagationModel"). Distinct from the addLayer *command*, which
	 *         builds constant layers instead. */
	bool addLayer(const std::string &type, const std::string &name,
				  const std::string &key) {
		FireDomain *domain = executor.getDomain();
		if (domain == 0) return false;
		return domain->addLayer(type, name, key);
	}

	/*! \brief Register a continuous field (altitude, wind, moisture, ...). */
	bool addScalarLayer(const std::string &type, const std::string &name,
						double x0, double y0, double t0,
						double width, double height, double timespan,
						int nx, int ny, int nz, int nt,
						const emscripten::val &values) {
		FireDomain *domain = executor.getDomain();
		if (domain == 0) return false;
		std::vector<double> data = toVector<double>(values);
		size_t nnx = nx, nny = ny, nnz = nz, nnt = nt;
		if (data.size() != nnx * nny * nnz * nnt) return false;
		data = toLayerOrder(data, nnx, nny, nnz, nnt);
		// addScalarLayer keeps the pointer, so the layer needs storage that
		// outlives this call; the domain owns it from here on.
		double *owned = new double[data.size()];
		std::copy(data.begin(), data.end(), owned);
		return domain->addScalarLayer(type, name, x0, y0, t0, width, height,
									  timespan, nnx, nny, nnz, nnt, owned);
	}

	/*! \brief Register an index field (fuel map, flux model map). */
	bool addIndexLayer(const std::string &type, const std::string &name,
					   double x0, double y0, double t0,
					   double width, double height, double timespan,
					   int nx, int ny, int nz, int nt,
					   const emscripten::val &values) {
		FireDomain *domain = executor.getDomain();
		if (domain == 0) return false;
		std::vector<int> data = toVector<int>(values);
		size_t nnx = nx, nny = ny, nnz = nz, nnt = nt;
		if (data.size() != nnx * nny * nnz * nnt) return false;
		data = toLayerOrder(data, nnx, nny, nnz, nnt);
		int *owned = new int[data.size()];
		std::copy(data.begin(), data.end(), owned);
		return domain->addIndexLayer(type, name, x0, y0, t0, width, height,
									 timespan, nnx, nny, nnz, nnt, owned);
	}

    emscripten::val tileDomain() {
        auto* domain = executor.getDomain();
        if (!domain || !domain->getDataBroker()->getFFTileManager()) return emscripten::val::null();
        auto manager = domain->getDataBroker()->getFFTileManager();
        auto out = emscripten::val::object();
        out.set("width",domain->getNECorner().getX()-domain->getSWCorner().getX());
        out.set("height",domain->getNECorner().getY()-domain->getSWCorner().getY());
        out.set("x0",domain->getSWCorner().getX());out.set("y0",domain->getSWCorner().getY());
        out.set("longitude",manager->coordinates.longitude);out.set("latitude",manager->coordinates.latitude);
        out.set("originX",manager->coordinates.x);out.set("originY",manager->coordinates.y);
        out.set("cachedTiles",manager->cachedTiles());
        return out;
    }

    double sampleAt(const std::string& name, double x, double y) {
        auto* domain = executor.getDomain();
        if (!domain || !domain->getDataBroker()->hasLayer(name))
            throw std::runtime_error("sampleAt: domain or layer is missing");
        return domain->getDataLayer(name)->getValueAt(FFPoint(x,y,0),domain->getSimulationTime());
    }

    // A bounded preview of a domain, without materializing a whole tile or
    // a potentially 200 km fuel matrix. Rows are south-first, like .fftile.
    emscripten::val sampleLayer(const std::string& name, int nx, int ny) {
        auto* domain = executor.getDomain();
        if (!domain || nx < 1 || ny < 1 || nx > 1024 || ny > 1024)
            throw std::runtime_error("sampleLayer: expected a domain and dimensions in 1..1024");
        double x0=domain->getSWCorner().getX(),y0=domain->getSWCorner().getY();
        return sampleWindow(name,x0,y0,domain->getNECorner().getX()-x0,
                            domain->getNECorner().getY()-y0,nx,ny);
    }

    emscripten::val sampleWindow(const std::string& name, double x0, double y0,
                                 double width, double height, int nx, int ny) {
        auto* domain=executor.getDomain();
        if (!domain || nx<1 || ny<1 || nx>1024 || ny>1024 ||
            !std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(width) ||
            !std::isfinite(height) || width<=0 || height<=0 ||
            x0<domain->getSWCorner().getX() || y0<domain->getSWCorner().getY() ||
            x0+width>domain->getNECorner().getX()+1e-6 ||
            y0+height>domain->getNECorner().getY()+1e-6)
            throw std::runtime_error("sampleWindow: invalid preview bounds or dimensions");
        std::vector<double> pixels(size_t(nx)*ny);
        for (int y=0;y<ny;++y) for(int x=0;x<nx;++x)
            pixels[size_t(y)*nx+x]=sampleAt(name,x0+(x+.5)*width/nx,y0+(y+.5)*height/ny);
        auto out=emscripten::val::object();out.set("width",nx);out.set("height",ny);
        out.set("data",emscripten::val(emscripten::typed_memory_view(pixels.size(),pixels.data())).call<emscripten::val>("slice"));
        return out;
    }

	/*! \brief Read a layer back as {shape:[nt,nz,ny,nx], data:Float64Array}. */
	emscripten::val getDoubleArray(const std::string &name) {
		FireDomain *domain = executor.getDomain();
		if (domain == 0) return emscripten::val::null();
		const double t = domain->getSimulationTime();

		FFArray<double> *src = 0;
		FluxLayer<double> *fluxLayer = domain->getFluxLayer(name);
		if (fluxLayer != 0) {
			fluxLayer->getMatrix(&src, t);
		} else {
			DataLayer<double> *dataLayer = domain->getDataLayer(name);
			if (dataLayer == 0) return emscripten::val::null();
			dataLayer->getMatrix(&src, t);
		}
		if (src == 0) return emscripten::val::null();

		const size_t nnx = src->getDim("x");
		const size_t nny = src->getDim("y");
		const size_t nnz = src->getDim("z");
		const size_t nnt = src->getDim("t");
		const double *data = src->getData();

		// Same Fortran-to-C reshuffle the NumPy binding does, so the JS view
		// indexes as [t][z][y][x].
		std::vector<double> reshaped(nnx * nny * nnz * nnt);
		for (size_t it = 0; it < nnt; ++it)
			for (size_t iz = 0; iz < nnz; ++iz)
				for (size_t iy = 0; iy < nny; ++iy)
					for (size_t ix = 0; ix < nnx; ++ix) {
						const size_t cIndex = ix + nnx * (iy + nny * (iz + nnz * it));
						const size_t fIndex = it + nnt * (iz + nnz * (iy + nny * ix));
						reshaped[cIndex] = data[fIndex];
					}

		emscripten::val shape = emscripten::val::array();
		shape.call<void>("push", nnt);
		shape.call<void>("push", nnz);
		shape.call<void>("push", nny);
		shape.call<void>("push", nnx);

		emscripten::val out = emscripten::val::object();
		out.set("shape", shape);
		// slice() forces a copy out of the heap view, which would otherwise
		// dangle the moment `reshaped` goes out of scope or the heap grows.
		out.set("data", emscripten::val(emscripten::typed_memory_view(
									reshaped.size(), reshaped.data()))
							.call<emscripten::val>("slice"));
		return out;
	}
};

std::string version() { return std::string(ff_version); }

}  // namespace

EMSCRIPTEN_BINDINGS(forefire) {
	emscripten::function("version", &version);
	emscripten::class_<ForeFireWasm>("ForeFire")
		.constructor<>()
		.function("execute", &ForeFireWasm::execute)
		.function("dumpString", &ForeFireWasm::dumpString)
		.function("getTime", &ForeFireWasm::getTime)
		.function("setInt", &ForeFireWasm::setInt)
		.function("getInt", &ForeFireWasm::getInt)
		.function("setDouble", &ForeFireWasm::setDouble)
		.function("getDouble", &ForeFireWasm::getDouble)
		.function("setString", &ForeFireWasm::setString)
		.function("getString", &ForeFireWasm::getString)
		.function("addLayer", &ForeFireWasm::addLayer)
		.function("addScalarLayer", &ForeFireWasm::addScalarLayer)
		.function("addIndexLayer", &ForeFireWasm::addIndexLayer)
		.function("getDoubleArray", &ForeFireWasm::getDoubleArray)
        .function("tileDomain", &ForeFireWasm::tileDomain)
        .function("sampleAt", &ForeFireWasm::sampleAt)
        .function("sampleLayer", &ForeFireWasm::sampleLayer)
        .function("sampleWindow", &ForeFireWasm::sampleWindow);
}
