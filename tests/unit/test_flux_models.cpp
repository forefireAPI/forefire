/**
 * @file test_flux_models.cpp
 * @brief Flux models, and the energy they are supposed to conserve.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * A flux model answers "what was the mean flux over [bt, et] for a cell that
 * ignited at at?". The atmospheric coupling calls it once per timestep with
 * whatever window the atmospheric model happens to be using, and adds the
 * result onto the surface fluxes. So the quantity that has to be right is not
 * any single call but the integral: a cell must release the same total energy
 * whether Meso-NH asks for it in one step or in fifty.
 *
 * tools/TODO.md lists exactly this as a wanted test — "mass conservation for
 * heat fluxes".
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "FluxModel.h"
#include "SimulationParameters.h"

#include <cmath>
#include <string>
#include <vector>

using libforefire::FluxModel;
using ff_test::Inputs;
using ff_test::ModelSandbox;

namespace {

const double BURNING_DURATION = 300.0;
const double NOMINAL_FLUX = 1000000.0;
const double ARRIVAL_TIME = 1000.0;

/** The two flux models with the same "constant nominal flux for a fixed
 *  duration" shape, and the parameter each one reads for its magnitude. */
struct BasicFluxModel {
    const char* name;
    const char* magnitudeParameter;
};

const BasicFluxModel BASIC_MODELS[] = {
    {"heatFluxBasic", "nominalHeatFlux"},
    {"vaporFluxBasic", "nominalVaporFlux"},
};

const size_t BASIC_MODEL_COUNT = sizeof(BASIC_MODELS) / sizeof(BASIC_MODELS[0]);

/** A sandbox with the burn duration and flux magnitude pinned, so the tests
 *  do not depend on the defaults, which differ between the two models. */
void configure(ModelSandbox& sandbox, const BasicFluxModel& model) {
    sandbox.parameters()->setDouble("burningDuration", BURNING_DURATION);
    sandbox.parameters()->setDouble(model.magnitudeParameter, NOMINAL_FLUX);
}

} /* anonymous namespace */

TEST_SUITE("flux models") {

TEST_CASE("total released energy does not depend on how the window is cut") {
    // The window runs from well before ignition to well after burnout, so
    // every partition of it must integrate to the same total: the full burn.
    const double windowStart = ARRIVAL_TIME - 137.0;
    const double windowEnd = ARRIVAL_TIME + BURNING_DURATION + 211.0;
    const double expectedTotal = NOMINAL_FLUX * BURNING_DURATION;

    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        ModelSandbox sandbox;
        configure(sandbox, BASIC_MODELS[m]);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        for (int steps = 1; steps <= 64; steps++) {
            CAPTURE(BASIC_MODELS[m].name);
            CAPTURE(steps);

            const double dt = (windowEnd - windowStart) / steps;
            double total = 0.;
            for (int i = 0; i < steps; i++) {
                const double bt = windowStart + i * dt;
                const double et = bt + dt;
                total += model->getValue(inputs.data(), bt, et, ARRIVAL_TIME) * dt;
            }
            CHECK(total == doctest::Approx(expectedTotal).epsilon(1e-9));
        }
    }
}

TEST_CASE("energy is conserved across unevenly sized steps too") {
    // An atmospheric model does not have to use a constant timestep, and the
    // partial-overlap branches are where an off-by-one in the interval
    // arithmetic would hide.
    const double cuts[] = {
        ARRIVAL_TIME - 500.0, ARRIVAL_TIME - 1.0, ARRIVAL_TIME,
        ARRIVAL_TIME + 0.5,   ARRIVAL_TIME + 17.0, ARRIVAL_TIME + 299.5,
        ARRIVAL_TIME + BURNING_DURATION, ARRIVAL_TIME + BURNING_DURATION + 0.25,
        ARRIVAL_TIME + BURNING_DURATION + 900.0,
    };
    const size_t cutCount = sizeof(cuts) / sizeof(cuts[0]);

    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        CAPTURE(BASIC_MODELS[m].name);
        ModelSandbox sandbox;
        configure(sandbox, BASIC_MODELS[m]);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        double total = 0.;
        for (size_t i = 0; i + 1 < cutCount; i++) {
            const double bt = cuts[i];
            const double et = cuts[i + 1];
            total += model->getValue(inputs.data(), bt, et, ARRIVAL_TIME) * (et - bt);
        }
        CHECK(total == doctest::Approx(NOMINAL_FLUX * BURNING_DURATION).epsilon(1e-9));
    }
}

TEST_CASE("nothing is released outside the burning interval") {
    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        CAPTURE(BASIC_MODELS[m].name);
        ModelSandbox sandbox;
        configure(sandbox, BASIC_MODELS[m]);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        // Entirely before ignition.
        CHECK(model->getValue(inputs.data(), ARRIVAL_TIME - 100.0,
                              ARRIVAL_TIME - 10.0, ARRIVAL_TIME) == doctest::Approx(0.0));
        // Entirely after burnout.
        CHECK(model->getValue(inputs.data(), ARRIVAL_TIME + BURNING_DURATION + 10.0,
                              ARRIVAL_TIME + BURNING_DURATION + 100.0,
                              ARRIVAL_TIME) == doctest::Approx(0.0));
    }
}

TEST_CASE("a window inside the burn reports the nominal flux") {
    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        CAPTURE(BASIC_MODELS[m].name);
        ModelSandbox sandbox;
        configure(sandbox, BASIC_MODELS[m]);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        CHECK(model->getValue(inputs.data(), ARRIVAL_TIME + 10.0, ARRIVAL_TIME + 20.0,
                              ARRIVAL_TIME) == doctest::Approx(NOMINAL_FLUX));
    }
}

TEST_CASE("the instantaneous flux switches on at ignition and off at burnout") {
    // bt == et is the separate branch every one of these models carries.
    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        CAPTURE(BASIC_MODELS[m].name);
        ModelSandbox sandbox;
        configure(sandbox, BASIC_MODELS[m]);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        const double before = ARRIVAL_TIME - 0.001;
        const double during = ARRIVAL_TIME + BURNING_DURATION / 2.0;
        const double after = ARRIVAL_TIME + BURNING_DURATION;

        CHECK(model->getValue(inputs.data(), before, before, ARRIVAL_TIME)
              == doctest::Approx(0.0));
        CHECK(model->getValue(inputs.data(), ARRIVAL_TIME, ARRIVAL_TIME, ARRIVAL_TIME)
              == doctest::Approx(NOMINAL_FLUX));
        CHECK(model->getValue(inputs.data(), during, during, ARRIVAL_TIME)
              == doctest::Approx(NOMINAL_FLUX));
        CHECK(model->getValue(inputs.data(), after, after, ARRIVAL_TIME)
              == doctest::Approx(0.0));
    }
}

TEST_CASE("a zero-length burn releases nothing") {
    for (size_t m = 0; m < BASIC_MODEL_COUNT; m++) {
        CAPTURE(BASIC_MODELS[m].name);
        ModelSandbox sandbox;
        sandbox.parameters()->setDouble("burningDuration", 0.0);
        sandbox.parameters()->setDouble(BASIC_MODELS[m].magnitudeParameter, NOMINAL_FLUX);
        FluxModel* model = sandbox.flux(BASIC_MODELS[m].name);
        REQUIRE(model != 0);
        Inputs inputs(model);

        const double value = model->getValue(inputs.data(), ARRIVAL_TIME - 10.0,
                                             ARRIVAL_TIME + 10.0, ARRIVAL_TIME);
        CHECK(std::isfinite(value));
        CHECK(value == doctest::Approx(0.0));
    }
}

} /* TEST_SUITE */
