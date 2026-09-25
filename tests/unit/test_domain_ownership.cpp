/**
 * @file test_domain_ownership.cpp
 * @brief A FireDomain owns what it registers, and only what it registers.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * propModelsTable and fluxModelsTable are static, shared by every domain in
 * the process. Two things used to go wrong with that, both covered here (#159):
 *
 *   - Nothing freed a domain's models or its propagative layer, so every
 *     domain built and thrown away cost about 183 kB, linearly, forever.
 *   - The constructor wiped the whole table, so building a second domain
 *     dropped the first one's registrations. The first domain's layer then
 *     held an index that was empty, and became the *second* domain's model
 *     once that one registered.
 *
 * The second is the one worth keeping a test on: it is silent, it produces
 * wrong spread rates rather than a crash, and Command.cpp builds exactly that
 * second domain for coupled runs.
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "FireDomain.h"

#include <string>

using libforefire::FireDomain;
using libforefire::FFPoint;
using ff_test::ModelSandbox;

namespace {

size_t occupiedPropSlots() {
    size_t used = 0;
    for (size_t i = 0; i < FireDomain::NUM_MAX_PROPMODELS; i++)
        if (FireDomain::propModelsTable[i] != 0) used++;
    return used;
}

FireDomain* makeDomain() {
    FFPoint sw(0., 0., 0.);
    FFPoint ne(1000., 1000., 0.);
    return new FireDomain(0., sw, ne);
}

} /* anonymous namespace */

TEST_SUITE("domain ownership") {

TEST_CASE("a destroyed domain releases its propagation model") {
    ModelSandbox sandbox; // holds the fuel table and restores parameters
    const size_t before = occupiedPropSlots();

    FireDomain* domain = makeDomain();
    REQUIRE(domain->addPropagativeLayer("Rothermel"));
    CHECK(occupiedPropSlots() == before + 1);

    delete domain;
    // Back to where we started: the slot is free and the model is gone. This
    // failing means the table fills up and the memory is never returned.
    CHECK(occupiedPropSlots() == before);
}

TEST_CASE("building a domain leaves another domain's models alone") {
    ModelSandbox sandbox;

    FireDomain* first = makeDomain();
    REQUIRE(first->addPropagativeLayer("Rothermel"));

    size_t firstIndex = FireDomain::NUM_MAX_PROPMODELS;
    for (size_t i = 0; i < FireDomain::NUM_MAX_PROPMODELS; i++)
        if (FireDomain::propModelsTable[i] != 0) firstIndex = i;
    REQUIRE(firstIndex < FireDomain::NUM_MAX_PROPMODELS);
    const libforefire::PropagationModel* firstModel =
        FireDomain::propModelsTable[firstIndex];

    // The constructor used to clear the table here.
    FireDomain* second = makeDomain();
    CHECK(FireDomain::propModelsTable[firstIndex] == firstModel);

    // And the second domain must not be handed the slot the first is using.
    REQUIRE(second->addPropagativeLayer("Rothermel"));
    CHECK(FireDomain::propModelsTable[firstIndex] == firstModel);

    delete second;
    // Destroying the second domain must not take the first one's model with
    // it: each releases only what it registered.
    CHECK(FireDomain::propModelsTable[firstIndex] == firstModel);

    delete first;
}

TEST_CASE("many domains do not accumulate table entries") {
    // The leak was linear and unbounded, so a loop is the shape of the check.
    // Slot occupancy standing still is the observable part of it.
    ModelSandbox sandbox;
    const size_t before = occupiedPropSlots();

    for (int i = 0; i < 40; i++) {
        FireDomain* domain = makeDomain();
        REQUIRE(domain->addPropagativeLayer("Rothermel"));
        delete domain;
    }

    CHECK(occupiedPropSlots() == before);
}

} /* TEST_SUITE */
