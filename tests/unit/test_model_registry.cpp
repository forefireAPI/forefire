/**
 * @file test_model_registry.cpp
 * @brief The model registry contains what it is supposed to contain.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * Every propagation and flux model registers itself from a static initialiser
 * that nothing references by name. That makes the whole set vulnerable to a
 * build change: link the core out of a static archive rather than an object
 * library and the linker drops the objects, the registry comes up empty, and
 * the failure only shows up much later as "model not recognized" — or as a
 * segfault. CMakeLists.txt has a comment about this and test_wheel.py checks
 * for it on the Python side; these are the same checks for the C++ build.
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "FluxModel.h"
#include "PropagationModel.h"

#include <string>
#include <vector>

using libforefire::FluxModel;
using libforefire::PropagationModel;
using ff_test::ModelSandbox;

namespace {

/** Every propagation model that builds without an external resource.
 *
 * ANNPropagationModel and BMapLoggerForANNTraining are deliberately absent:
 * both read a .ffann network in their constructor and abort the process when
 * it is missing, so they cannot be covered here. tests/runANN exercises them.
 */
const std::vector<std::string>& propagationModels() {
    static std::vector<std::string> names;
    if (!names.empty()) return names;
    names.push_back("Balbi2015");
    names.push_back("Balbi2020");
    names.push_back("BalbiNov2011");
    names.push_back("BalbiNov2011Curv");
    names.push_back("BalbiNov2011TMdMl");
    names.push_back("BalbiUnsteady");
    names.push_back("CurvatureDriven");
    names.push_back("Farsite");
    names.push_back("FrontDepthDriven");
    names.push_back("Iso");
    names.push_back("IsotropicFuel");
    names.push_back("LavaPropagationModel");
    names.push_back("Rothermel");
    names.push_back("RothermelAndrews2018");
    names.push_back("SamplePropagationModel");
    names.push_back("TroisPourcent");
    names.push_back("WindDriven");
    return names;
}

const std::vector<std::string>& fluxModels() {
    static std::vector<std::string> names;
    if (!names.empty()) return names;
    names.push_back("BurnUpHeatFlux");
    names.push_back("CraterHeatFluxModel");
    names.push_back("CraterVaporFluxModel");
    names.push_back("factorChemFlux");
    names.push_back("ForeFireV1HeatFlux");
    names.push_back("ForeFireV1VaporFlux");
    names.push_back("heatFluxBasic");
    names.push_back("heatFluxFromObs");
    names.push_back("heatFluxNominal");
    names.push_back("LavaSO2Flux");
    names.push_back("SFMod");
    names.push_back("SFObs");
    names.push_back("SpottingFluxBasic");
    names.push_back("vaporFluxBasic");
    names.push_back("vaporFluxFromObs");
    names.push_back("vaporFluxNominal");
    return names;
}

} /* anonymous namespace */

TEST_SUITE("model registry") {

TEST_CASE("every propagation model is registered and names itself") {
    ModelSandbox sandbox;
    const std::vector<std::string>& names = propagationModels();
    for (size_t i = 0; i < names.size(); i++) {
        CAPTURE(names[i]);
        PropagationModel* model = sandbox.propagation(names[i]);
        REQUIRE(model != 0);
        // A model whose getName() disagrees with the key it registered under
        // cannot be found again by the name it reports.
        CHECK(model->getName() == names[i]);
    }
}

TEST_CASE("every flux model is registered and names itself") {
    ModelSandbox sandbox;
    const std::vector<std::string>& names = fluxModels();
    for (size_t i = 0; i < names.size(); i++) {
        CAPTURE(names[i]);
        FluxModel* model = sandbox.flux(names[i]);
        REQUIRE(model != 0);
        CHECK(model->getName() == names[i]);
    }
}

TEST_CASE("an unknown model name is refused rather than fatal") {
    ModelSandbox sandbox;
    PropagationModel* propagation = sandbox.propagation("NoSuchPropagationModel");
    FluxModel* flux = sandbox.flux("NoSuchFluxModel");
    CHECK(propagation == 0);
    CHECK(flux == 0);
}

TEST_CASE("every model can be destroyed") {
    // `properties` is allocated by each model's own constructor with
    // new double[numProperties] and freed by ~ForeFireModel. Freeing it in a
    // derived destructor as well is a double free, and this case is what says
    // so: run it under a build that reintroduces one and it aborts.
    //
    // The models are freed by ~FireDomain, which is what owns them: it records
    // the entries it put in the shared model tables and releases those. So
    // each model gets its own sandbox here, and destroying the sandbox is the
    // destruction under test. Deleting one by hand instead would be the double
    // free this case exists to catch.
    const std::vector<std::string>& props = propagationModels();
    for (size_t i = 0; i < props.size(); i++) {
        CAPTURE(props[i]);
        ModelSandbox sandbox;
        PropagationModel* model = sandbox.propagation(props[i]);
        REQUIRE(model != 0);
    }

    const std::vector<std::string>& fluxes = fluxModels();
    for (size_t i = 0; i < fluxes.size(); i++) {
        CAPTURE(fluxes[i]);
        ModelSandbox sandbox;
        FluxModel* model = sandbox.flux(fluxes[i]);
        REQUIRE(model != 0);
    }
}

TEST_CASE("destroying a domain's models does not disturb the next domain") {
    // A double free often shows up as the *next* allocation coming back
    // corrupted rather than as an immediate abort, so allocate across the
    // destruction and check the new model is intact.
    std::vector<std::string> wanted;
    size_t count = 0;
    {
        ModelSandbox sandbox;
        PropagationModel* first = sandbox.propagation("Rothermel");
        REQUIRE(first != 0);
        wanted = first->wantedProperties;
        count = first->numProperties;
    }

    ModelSandbox sandbox;
    PropagationModel* second = sandbox.propagation("Rothermel");
    REQUIRE(second != 0);
    CHECK(second->numProperties == count);
    CHECK(second->wantedProperties == wanted);
}

TEST_CASE("property registration order is stable within a model") {
    // The property array is addressed by position, so two instances of the
    // same model must number their properties identically — otherwise a
    // DataBroker filling the array for one would mis-feed the other.
    ModelSandbox sandbox;
    PropagationModel* first = sandbox.propagation("Rothermel");
    PropagationModel* second = sandbox.propagation("Rothermel");
    REQUIRE(first != 0);
    REQUIRE(second != 0);
    CHECK(first->wantedProperties == second->wantedProperties);
    CHECK(first->numProperties == second->numProperties);
}

} /* TEST_SUITE */
