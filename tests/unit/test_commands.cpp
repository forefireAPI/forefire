/**
 * @file test_commands.cpp
 * @brief Commands that need a domain, run without one.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * Commands that need a domain used to dereference a null one, so a script
 * whose loadData had failed segfaulted on the next startFire. They must print
 * an error and return instead. As in test_quit_command.cpp, reaching the end
 * of the case is the assertion that matters.
 */

#include "doctest/doctest.h"

#include "Command.h"

#include <string>

using libforefire::Command;

namespace {

void run(Command& executor, std::string command) {
    executor.ExecuteCommand(command);
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

} /* TEST_SUITE */
