/**
 * @file test_data_layers.cpp
 * @brief XYZTDataLayer interpolation in time and near its edges.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "FFPoint.h"
#include "NCXYZTDataLayer.h"

#include <cmath>
#include <cstddef>
#include <vector>

using libforefire::FFPoint;
using libforefire::XYZTDataLayer;

namespace {

// A 2x2 layer over 1 km, uniform in space, one value per time slice.
double valueAt(const std::vector<double>& slices, double t0, double timespan, double t) {
    std::size_t nx = 2, ny = 2, nz = 1, nt = slices.size();
    std::vector<double> vals(nx * ny * nz * nt);
    for (std::size_t c = 0; c < nx * ny; ++c)
        for (std::size_t k = 0; k < nt; ++k)
            vals[c * nt + k] = slices[k];   // time is the fastest index
    FFPoint sw(0., 0., 0.), extent(1000., 1000., 0.);
    XYZTDataLayer<double> layer("windU", sw, t0, extent, timespan, nx, ny, nz, nt, vals.data());
    return layer.getValueAt(FFPoint(500., 500., 0.), t);
}

// A 2x2 layer over 2 km, 0 in the west cells and 10 in the east ones.
// Values sit at the cell centres, x = 500 and x = 1500.
double eastRampAt(double x, double y) {
    std::vector<double> vals = {0., 0., 10., 10.};   // x is the slowest index
    FFPoint sw(0., 0., 0.), extent(2000., 2000., 0.);
    double t0 = 0., timespan = 0.;
    std::size_t nx = 2, ny = 2, nz = 1, nt = 1;
    XYZTDataLayer<double> layer("altitude", sw, t0, extent, timespan, nx, ny, nz, nt, vals.data());
    return layer.getValueAt(FFPoint(x, y, 0.), 0.);
}

}

TEST_SUITE("data layers") {

TEST_CASE("a layer with a time axis returns the slice it is closest to") {
    const std::vector<double> ramp = {0., 8.};
    CHECK(valueAt(ramp, 0., 2000., 0.) == doctest::Approx(0.));
    CHECK(valueAt(ramp, 0., 2000., 500.) == doctest::Approx(2.));
    CHECK(valueAt(ramp, 0., 2000., 2000.) == doctest::Approx(8.));
}

TEST_CASE("the time axis starts at the layer's start time") {
    const std::vector<double> ramp = {0., 8.};
    CHECK(valueAt(ramp, 1000., 2000., 1250.) == doctest::Approx(1.));
}

TEST_CASE("a time axis longer than a day still interpolates") {
    const std::vector<double> hourly = {0., 4., 8.};
    CHECK(valueAt(hourly, 0., 2 * 86400., 86400.) == doctest::Approx(4.));
    CHECK(valueAt(hourly, 0., 2 * 86400., 1.5 * 86400.) == doctest::Approx(6.));
}

TEST_CASE("outside its time axis a layer holds the first or last slice") {
    const std::vector<double> ramp = {0., 8.};
    CHECK(valueAt(ramp, 1000., 2000., 0.) == doctest::Approx(0.));
    CHECK(valueAt(ramp, 1000., 2000., 5000.) == doctest::Approx(8.));
}

TEST_CASE("between its edge and the outermost cell centres a layer holds the edge cells") {
    // This band used to return NaN: half a cell along every edge, a quarter
    // of a 2x2 layer. A slope taken there turned the front geometry to NaN.
    // Lookups are nudged by EPSILONX, hence the tolerance mid-ramp.
    CHECK(eastRampAt(1000., 1000.) == doctest::Approx(5.).epsilon(0.01));
    CHECK(eastRampAt(1900., 1000.) == doctest::Approx(10.));
    CHECK(eastRampAt(2000., 1000.) == doctest::Approx(10.));
    CHECK(eastRampAt(100., 1900.) == doctest::Approx(0.));
    CHECK(eastRampAt(0., 0.) == doctest::Approx(0.));
}

TEST_CASE("outside its extent a layer has no data") {
    CHECK(std::isnan(eastRampAt(2100., 1000.)));
    CHECK(std::isnan(eastRampAt(-100., 1000.)));
    CHECK(std::isnan(eastRampAt(1000., 2100.)));
}

}
