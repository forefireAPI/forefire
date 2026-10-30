/**
 * @file test_data_broker.cpp
 * @brief DataBroker: layers that miss the domain, and the layers a model needs.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "DataBroker.h"
#include "FFPoint.h"
#include "FFVector.h"
#include "FireDomain.h"
#include "FireFront.h"
#include "FireNode.h"
#include "FuelDataLayer.h"
#include "PropagationModel.h"
#include "TimeTable.h"

#include <cstdio>
#include <string>

using libforefire::DataBroker;
using libforefire::FFPoint;
using libforefire::FFVector;
using libforefire::FireDomain;
using libforefire::FireFront;
using libforefire::FireNode;
using libforefire::FuelDataLayer;
using libforefire::PropagationModel;
using libforefire::TimeTable;
using ff_test::ModelSandbox;

namespace {

/* One fire node at the centre of the sandbox's 1 km domain. */
FireNode* addNode(FireDomain* domain) {
    FireFront* front = domain->addFireFront(0.);
    FFPoint loc(500., 500., 0.);
    FFVector vel;
    return domain->addFireNode(loc, vel, 0., 20., 0., front, 0);
}

}

TEST_SUITE("data broker") {

TEST_CASE("a model added after the domain is built gets the wind layers it needs") {
    ModelSandbox sandbox;
    FireDomain* domain = sandbox.fireDomain();
    TimeTable schedule;
    domain->setTimeTable(&schedule);
    DataBroker* broker = domain->getDataBroker();
    REQUIRE_FALSE(broker->hasLayer("windU"));

    // The domain has already checked its layers; this model asks for normalWind afterwards.
    PropagationModel* model = sandbox.propagation("TroisPourcent");
    REQUIRE(model != nullptr);
    broker->getPropagationData(model, addNode(domain));

    CHECK(broker->hasLayer("windU"));
    CHECK(broker->hasLayer("windV"));
}

TEST_CASE("a model added late leaves front depth computation as the parameters set it") {
    ModelSandbox sandbox;
    FireDomain* domain = sandbox.fireDomain();
    TimeTable schedule;
    domain->setTimeTable(&schedule);
    REQUIRE(domain->getFluxLayer("heatFlux") == nullptr);

    // Balbi2015 asks for frontDepth. Switching its computation on would also
    // add a heat flux layer, and change the model's speeds.
    PropagationModel* model = sandbox.propagation("Balbi2015");
    REQUIRE(model != nullptr);
    domain->getDataBroker()->getPropagationData(model, addNode(domain));

    CHECK(domain->getFluxLayer("heatFlux") == nullptr);
}

TEST_CASE("a second live domain gets its own altitude and wind layers") {
    // The first domain stays alive, its layers set up, while the second is built.
    ModelSandbox first;
    PropagationModel* firstModel = first.propagation("TroisPourcent");
    TimeTable firstSchedule;
    first.fireDomain()->setTimeTable(&firstSchedule);
    first.fireDomain()->getDataBroker()->getPropagationData(firstModel, addNode(first.fireDomain()));

    ModelSandbox second;
    FireDomain* domain = second.fireDomain();
    TimeTable schedule;
    domain->setTimeTable(&schedule);
    DataBroker* broker = domain->getDataBroker();
    CHECK(broker->hasLayer("altitude"));

    PropagationModel* model = second.propagation("TroisPourcent");
    REQUIRE(model != nullptr);
    broker->getPropagationData(model, addNode(domain));
    CHECK(broker->hasLayer("windU"));
    CHECK(broker->hasLayer("windV"));
}

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
