/**
 * @file test_propagation_models.cpp
 * @brief Rate-of-spread models, tested one call at a time.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * getSpeedForNode splits into "gather the properties from the simulation" and
 * "do the arithmetic". Only the second half is physics, and it is a function
 * of a plain double array, so it can be checked without a landscape, a fire or
 * a time step.
 *
 * Two kinds of assertion live here. The invariants — no spread without fuel,
 * more wind never means less spread — should hold whatever the implementation
 * does, and are the ones worth trusting. The pinned values are just the
 * current output recorded so a refactor cannot change it unnoticed; they carry
 * no claim of being right, only of being what ForeFire produces today.
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "PropagationModel.h"
#include "SimulationParameters.h"

#include <cmath>
#include <string>
#include <vector>

using libforefire::PropagationModel;
using ff_test::Inputs;
using ff_test::ModelSandbox;

namespace {

/** Runs a model once with the given inputs. */
double speedOf(PropagationModel* model, Inputs& inputs) {
    return model->getSpeed(inputs.data());
}

/** Fuel-driven models whose inputs fillStandardConditions covers. Farsite,
 *  Lava and RothermelAndrews2018 read their own fuel columns and are covered
 *  by the registry tests instead. */
const std::vector<std::string>& standardFuelModels() {
    static std::vector<std::string> names;
    if (!names.empty()) return names;
    names.push_back("Balbi2015");
    names.push_back("Balbi2020");
    names.push_back("BalbiNov2011");
    names.push_back("BalbiNov2011Curv");
    names.push_back("BalbiNov2011TMdMl");
    names.push_back("BalbiUnsteady");
    names.push_back("Rothermel");
    return names;
}

} /* anonymous namespace */

TEST_SUITE("propagation models") {

TEST_CASE("Iso propagates at exactly the configured speed") {
    ModelSandbox sandbox;
    sandbox.parameters()->setParameter("Iso.speed", "2.5");
    PropagationModel* model = sandbox.propagation("Iso");
    REQUIRE(model != 0);

    Inputs inputs(model);
    CHECK(speedOf(model, inputs) == doctest::Approx(2.5));
}

TEST_CASE("Iso falls back to 1 m/s when unconfigured") {
    // Also the check that a sandbox does not inherit the previous one's
    // parameters: the case above sets Iso.speed on its own instance.
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("Iso");
    REQUIRE(model != 0);

    Inputs inputs(model);
    CHECK(speedOf(model, inputs) == doctest::Approx(1.0));
}

TEST_CASE("WindDriven is linear in the normal wind") {
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("WindDriven");
    REQUIRE(model != 0);

    Inputs inputs(model);
    inputs.set("fuel.vv_coeff", 0.5);

    inputs.set("normalWind", 0.0);
    CHECK(speedOf(model, inputs) == doctest::Approx(0.0));

    inputs.set("normalWind", 4.0);
    CHECK(speedOf(model, inputs) == doctest::Approx(2.0));

    inputs.set("normalWind", 8.0);
    CHECK(speedOf(model, inputs) == doctest::Approx(4.0));
}

TEST_CASE("no fuel depth means no spread") {
    // Rothermel divides the fuel load by the depth to get a bulk density, and
    // Balbi2020 does the same for its packing ratio, so both have to special
    // case an empty cell rather than divide by zero. A NaN here would travel
    // straight into the front geometry.
    ModelSandbox sandbox;
    const std::vector<std::string>& names = standardFuelModels();
    for (size_t i = 0; i < names.size(); i++) {
        CAPTURE(names[i]);
        PropagationModel* model = sandbox.propagation(names[i]);
        REQUIRE(model != 0);

        Inputs inputs(model);
        REQUIRE(ff_test::fillStandardConditions(inputs, model));
        inputs.set("fuel.e", 0.0);
        inputs.set("normalWind", 5.0);

        const double ros = speedOf(model, inputs);
        CHECK(std::isfinite(ros));
        CHECK(ros == doctest::Approx(0.0));
    }
}

TEST_CASE("spread is finite and non-negative over a wind and slope sweep") {
    ModelSandbox sandbox;
    const std::vector<std::string>& names = standardFuelModels();
    const double winds[] = {-10.0, -1.0, 0.0, 1.0, 5.0, 20.0, 50.0};
    const double slopes[] = {-1.0, -0.2, 0.0, 0.2, 1.0};

    for (size_t i = 0; i < names.size(); i++) {
        PropagationModel* model = sandbox.propagation(names[i]);
        REQUIRE(model != 0);
        Inputs inputs(model);
        REQUIRE(ff_test::fillStandardConditions(inputs, model));

        for (size_t w = 0; w < sizeof(winds) / sizeof(winds[0]); w++) {
            for (size_t s = 0; s < sizeof(slopes) / sizeof(slopes[0]); s++) {
                CAPTURE(names[i]);
                CAPTURE(winds[w]);
                CAPTURE(slopes[s]);
                inputs.set("normalWind", winds[w]);
                inputs.set("slope", slopes[s]);

                const double ros = speedOf(model, inputs);
                CHECK(std::isfinite(ros));
                CHECK(ros >= 0.0);
            }
        }
    }
}

TEST_CASE("Rothermel never spreads slower for more wind") {
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("Rothermel");
    REQUIRE(model != 0);

    Inputs inputs(model);
    REQUIRE(ff_test::fillStandardConditions(inputs, model));

    double previous = -1.0;
    for (double wind = 0.0; wind <= 30.0; wind += 0.5) {
        CAPTURE(wind);
        inputs.set("normalWind", wind);
        const double ros = speedOf(model, inputs);
        CHECK(ros >= previous);
        previous = ros;
    }
}

TEST_CASE("Rothermel clamps the wind at its own effective limit") {
    // The 2013 Andrews/Cruz/Rothermel limit caps the wind the model will react
    // to, so beyond it extra wind must change nothing at all.
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("Rothermel");
    REQUIRE(model != 0);

    Inputs inputs(model);
    REQUIRE(ff_test::fillStandardConditions(inputs, model));

    inputs.set("normalWind", 200.0);
    const double atLimit = speedOf(model, inputs);
    inputs.set("normalWind", 2000.0);
    const double wellPast = speedOf(model, inputs);

    CHECK(std::isfinite(atLimit));
    CHECK(wellPast == doctest::Approx(atLimit));
}

TEST_CASE("Rothermel treats a downslope as flat ground") {
    // phiP squares the slope, so without the clamp a downslope would speed the
    // fire up exactly as much as the matching upslope.
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("Rothermel");
    REQUIRE(model != 0);

    Inputs inputs(model);
    REQUIRE(ff_test::fillStandardConditions(inputs, model));

    inputs.set("slope", 0.0);
    const double flat = speedOf(model, inputs);
    inputs.set("slope", -0.5);
    const double downhill = speedOf(model, inputs);
    inputs.set("slope", 0.5);
    const double uphill = speedOf(model, inputs);

    CHECK(downhill == doctest::Approx(flat));
    CHECK(uphill > flat);
}

TEST_CASE("drier live fuel spreads faster, over the physical range") {
    // Live fuel moisture appears in BalbiNov2011 through xsi, which scales the
    // flame temperature. Raising it should slow the fire down.
    //
    // It only does so up to about Ml = 0.8 for this fuel. Past that, xsi
    // exceeds 1, the flame temperature term goes negative, and R00 raises it
    // to the fourth power — so spread starts climbing again: 1.3e-3 m/s at
    // Ml = 0.5, 1.9e-8 at Ml = 0.8, then back up to 8.2e-5 at Ml = 1.0, which
    // is the value fuel 1 of tests/runff/fuels.csv carries. The sweep below
    // deliberately stops before that inversion rather than asserting it is
    // correct; see tests/unit/README.md.
    ModelSandbox sandbox;
    PropagationModel* model = sandbox.propagation("BalbiNov2011");
    REQUIRE(model != 0);

    Inputs inputs(model);
    REQUIRE(ff_test::fillStandardConditions(inputs, model));
    inputs.set("normalWind", 5.0);

    double previous = -1.0;
    for (double liveMoisture = 0.8; liveMoisture >= 0.1; liveMoisture -= 0.05) {
        CAPTURE(liveMoisture);
        inputs.set("fuel.Ml", liveMoisture);
        const double ros = speedOf(model, inputs);
        CHECK(ros >= previous);
        previous = ros;
    }
}

TEST_CASE("recorded rates of spread for the standard fuel") {
    // Regression pins, not reference physics: fuel 1 of tests/runff/fuels.csv,
    // 5 m/s of normal wind on flat ground, with the default
    // windReductionFactor of 0.4. If one of these moves, the model changed —
    // which may well be intended, but should be a decision rather than a
    // surprise. tests/runff only ever runs Rothermel, so for every other model
    // here this is the only thing watching the arithmetic.
    ModelSandbox sandbox;

    struct Expectation {
        const char* model;
        double ros;
    };
    const Expectation expected[] = {
        {"Rothermel", 0.392218},
        {"Balbi2015", 0.069659},
        {"Balbi2020", 0.0823161},
        {"BalbiNov2011", 8.16139e-05},
        {"BalbiNov2011Curv", 0.080263},
        {"BalbiNov2011TMdMl", 0.0584482},
        {"BalbiUnsteady", 0.1},
    };

    // Loose enough to survive -march=native and FP contraction differing
    // between CI runners, tight enough that no real change to a model slips
    // through.
    const double tolerance = 1e-5;

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        CAPTURE(std::string(expected[i].model));
        PropagationModel* model = sandbox.propagation(expected[i].model);
        REQUIRE(model != 0);

        Inputs inputs(model);
        REQUIRE(ff_test::fillStandardConditions(inputs, model));
        inputs.set("normalWind", 5.0);
        inputs.set("slope", 0.0);

        CHECK(speedOf(model, inputs) == doctest::Approx(expected[i].ros).epsilon(tolerance));
    }
}

} /* TEST_SUITE */
