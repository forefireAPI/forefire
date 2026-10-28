/**
 * @file test_commands.cpp
 * @brief Commands without a domain, a domain built after clear[], and the
 *        arrival-time exports.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * Commands that need a domain used to dereference a null one, so a script
 * whose loadData had failed segfaulted on the next startFire. They must print
 * an error and return instead. As in test_quit_command.cpp, reaching the end
 * of the case is the assertion that matters.
 *
 * A domain built after clear[] used to crash on its first startFire.
 *
 * The BMap layer, which holds the arrival times, was never filled.
 */

#include "doctest/doctest.h"

#include "Command.h"

#include <cmath>
#include <string>

using libforefire::Command;
using libforefire::FFArray;

namespace {

void run(Command& executor, std::string command) {
    executor.ExecuteCommand(command);
}

/* A 2000 x 1000 m domain, 200 x 100 cells at the default 10 m, with a fire
 * started off-centre at (300, 800) and run to t = 300. */
void burnOffCentre(Command& executor) {
    run(executor, "clear[]");
    run(executor, "setParameter[bmapLayer=1]");
    run(executor, "FireDomain[sw=(0,0,0);ne=(2000,1000,0);t=0]");
    REQUIRE(Command::getDomain() != nullptr);
    REQUIRE(Command::getDomain()->addPropagativeLayer("Iso"));
    run(executor, "startFire[loc=(300,800,0)]");
    run(executor, "goTo[t=300]");
}

} /* anonymous namespace */

TEST_SUITE("commands") {

TEST_CASE("commands that need a domain return without one") {
    Command executor;
    const char* commands[] = {
        "FireNode[loc=(10,10,0);vel=(0,0,0);t=0]",
        "FireFront[t=0]",
        "startFire[loc=(10,10,0)]",
        "goTo[t=10]",
        "save[]",
        "trigger[fuel;vv_coeff=1]",
        "trigger[fuelIndice;loc=(1,1,0);fuelType=1]",
        "step[dt=1]@t=10",
        "step[dt=1]@nowplus=10",
        "FireDomain[sw=(0,0,0)]",
        "clear[]",
    };
    run(executor, "clear[]");
    for (const char* command : commands) {
        CAPTURE(command);
        run(executor, command);
        CHECK(Command::getDomain() == nullptr);
    }
}

TEST_CASE("a domain built after clear[] burns like the first one") {
    // The first domain's deleted layers used to outlive it in the DataBroker
    // shortcuts, so the new domain never created its own altitude layer and
    // its first startFire dereferenced a null one.
    Command executor;
    size_t burnt[2] = {0, 0};
    for (size_t& count : burnt) {
        burnOffCentre(executor);
        auto matrix = Command::getDomain()->getDataMatrix("arrival_time");
        for (const auto& column : matrix)
            for (double t : column)
                if (std::isfinite(t)) count++;
    }
    CHECK(burnt[0] > 0);
    CHECK(burnt[1] == burnt[0]);
}

TEST_CASE("the BMap layer holds the domain's arrival times") {
    Command executor;
    burnOffCentre(executor);
    auto* domain = Command::getDomain();
    auto* layer = domain->getDataLayer("BMap");
    REQUIRE(layer != nullptr);

    FFArray<double>* bmap = nullptr;
    layer->getMatrix(&bmap, domain->getSimulationTime());
    REQUIRE(bmap != nullptr);
    size_t burnt = 0, mismatched = 0;
    for (size_t i = 0; i < bmap->getDim("x"); i++) {
        for (size_t j = 0; j < bmap->getDim("y"); j++) {
            double expected = domain->getArrivalTime(i, j);
            if (std::isfinite(expected)) burnt++;
            if ((*bmap)(i, j) != expected) mismatched++;
        }
    }
    CHECK(burnt > 0);
    CHECK(mismatched == 0);
}

} /* TEST_SUITE */
