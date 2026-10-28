/**
 * @file test_quit_command.cpp
 * @brief `quit[]` must not end the process it is running inside.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 *
 * `quit` sits in the script command table, so it is reachable from the Python
 * binding and from the HTTP server, and it used to call exit(0). A host got no
 * traceback, no exception, no `finally`, no destructors, and a status of 0 —
 * a batch job recorded success having stopped early. See #160.
 *
 * The test for that is the whole file running to completion: if `quit[]` still
 * called exit(0), the cases below this one would not report, and ctest would
 * see a suite that passed nothing rather than a suite that failed. The
 * `--test-suite` entry in CMakeLists is what turns that into a visible result.
 */

#include "doctest/doctest.h"

#include "Command.h"

#include <string>

using libforefire::Command;

TEST_SUITE("quit command") {

TEST_CASE("quit[] returns instead of ending the process") {
    Command executor;
    Command::clearQuitRequest();

    std::string command = "quit[]";
    executor.ExecuteCommand(command);

    // Reaching this line at all is the assertion that matters.
    CHECK(Command::quitRequested());
}

TEST_CASE("the quit request can be cleared and the session reused") {
    // A host that decides to ignore the request has to be able to carry on,
    // which is the difference between asking and terminating.
    Command executor;
    Command::clearQuitRequest();
    REQUIRE_FALSE(Command::quitRequested());

    std::string quitCommand = "quit[]";
    executor.ExecuteCommand(quitCommand);
    REQUIRE(Command::quitRequested());

    Command::clearQuitRequest();
    CHECK_FALSE(Command::quitRequested());

    // The interpreter still works after a quit: setParameter and getParameter
    // round-trip, which they could not do if quit had freed the parameters
    // singleton as it used to.
    std::string setCommand = "setParameter[ff_test.after.quit=42]";
    executor.ExecuteCommand(setCommand);
    CHECK(libforefire::SimulationParameters::GetInstance()
              ->getParameter("ff_test.after.quit") == "42");
}

TEST_CASE("a second quit is harmless") {
    // quit() deletes the session objects and nulls them; running it twice must
    // not double free what the first call released.
    Command executor;
    Command::clearQuitRequest();

    std::string command = "quit[]";
    executor.ExecuteCommand(command);
    executor.ExecuteCommand(command);

    CHECK(Command::quitRequested());
}

} /* TEST_SUITE */
