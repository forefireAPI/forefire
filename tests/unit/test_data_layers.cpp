/**
 * @file test_data_layers.cpp
 * @brief XYZTDataLayer interpolation in time.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "FFPoint.h"
#include "NCXYZTDataLayer.h"

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

}
