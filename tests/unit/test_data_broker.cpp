/**
 * @file test_data_broker.cpp
 * @brief DataBroker::loadFromNCFile on layers that miss the domain.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "DataBroker.h"
#include "FFPoint.h"
#include "FireDomain.h"
#include "FuelDataLayer.h"

#include <cstdio>
#include <string>

using libforefire::FFPoint;
using libforefire::FireDomain;
using libforefire::FuelDataLayer;
using ff_test::ModelSandbox;

TEST_SUITE("data broker") {

#ifndef FF_NO_NETCDF
TEST_CASE("a fuel map that misses the domain is stretched over it like the other layers") {
    ModelSandbox sandbox;
    sandbox.propagation("Rothermel");
    FireDomain* domain = sandbox.fireDomain();

    // 2x2 fuel map 100 km away from the 1 km domain: no overlap, so the
    // failsafe reprojection applies. Its first cell is fuel 0, the others 1.
    const std::string path = "test_data_broker_fuel.nc";
    {
        NcFile file(path, NcFile::replace);
        NcVar dom = file.addVar("domain", ncInt);
        dom.putAtt("type", "domain");
        dom.putAtt("SWx", ncDouble, 100000.);
        dom.putAtt("SWy", ncDouble, 100000.);
        dom.putAtt("Lx", ncDouble, 1000.);
        dom.putAtt("Ly", ncDouble, 1000.);
        std::vector<NcDim> dims = {file.addDim("ft", 1), file.addDim("fz", 1),
                                   file.addDim("fy", 2), file.addDim("fx", 2)};
        NcVar fuel = file.addVar("fuel", ncShort, dims);
        fuel.putAtt("type", "fuel");
        const short cells[4] = {0, 1, 1, 1};
        fuel.putVar(cells);
    }
    domain->getDataBroker()->loadFromNCFile(path);
    std::remove(path.c_str());

    FuelDataLayer<double>* layer =
        dynamic_cast<FuelDataLayer<double>*>(domain->getDataBroker()->getLayer("fuel"));
    REQUIRE(layer != nullptr);
    // Stretched over the domain, (750, 750) falls in the last cell. Left at
    // its own extent the map is outside the domain and every point reads cell 0.
    CHECK(layer->getValueAt(FFPoint(750., 750., 0.), 0.) == 1.);
}
#endif

}
