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
 * The arrival times can be read two ways, and both used to be wrong: the BMap
 * layer was never filled, and plot[...;filename=*.asc] wrote one row per x,
 * at six significant digits, instead of an ASCII grid.
 *
 * A front reaching the domain edge over a gridded altitude used to fill the
 * whole arrival map at one time: the slope past the edge was NaN, and so
 * were the node positions it moved.
 */

#include "doctest/doctest.h"

#include "Command.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

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

struct AsciiGrid {
    size_t ncols = 0, nrows = 0;
    double xll = 0, yll = 0, cellsize = 0;
    std::vector<std::vector<std::string>> rows; /* as written, north first */
};

AsciiGrid readAsciiGrid(const std::string& path) {
    AsciiGrid grid;
    std::ifstream in(path);
    std::string key;
    double nodata;
    in >> key >> grid.ncols >> key >> grid.nrows >> key >> grid.xll >> key >> grid.yll
       >> key >> grid.cellsize >> key >> nodata;
    grid.rows.assign(grid.nrows, std::vector<std::string>(grid.ncols));
    for (auto& row : grid.rows)
        for (auto& value : row)
            in >> value;
    return grid;
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

TEST_CASE("plot writes arrival times as an ASCII grid, north row first") {
    Command executor;
    burnOffCentre(executor);
    const std::string path = "ff_test_arrival_time.asc";
    run(executor, "plot[parameter=arrival_time;filename=" + path + "]");
    AsciiGrid grid = readAsciiGrid(path);
    std::remove(path.c_str());

    CHECK(grid.ncols == 200);
    CHECK(grid.nrows == 100);
    CHECK(grid.cellsize == doctest::Approx(10.));
    REQUIRE(grid.rows.size() == grid.nrows);

    /* The earliest cell is where the fire started. */
    size_t bestRow = 0, bestCol = 0;
    double earliest = std::numeric_limits<double>::infinity();
    for (size_t r = 0; r < grid.nrows; r++) {
        for (size_t c = 0; c < grid.ncols; c++) {
            double t = std::stod(grid.rows[r][c]);
            if (t < earliest) {
                earliest = t;
                bestRow = r;
                bestCol = c;
            }
        }
    }
    double x = grid.xll + (bestCol + 0.5) * grid.cellsize;
    double y = grid.yll + (grid.nrows - bestRow - 0.5) * grid.cellsize;
    CHECK(x == doctest::Approx(300.).epsilon(0.1));
    CHECK(y == doctest::Approx(800.).epsilon(0.1));

    /* Values round-trip exactly, so no arrival time is rounded. */
    auto matrix = Command::getDomain()->getDataMatrix("arrival_time");
    size_t inexact = 0;
    for (size_t r = 0; r < grid.nrows; r++)
        for (size_t c = 0; c < grid.ncols; c++)
            if (std::stod(grid.rows[r][c]) != matrix[c][grid.nrows - 1 - r]) inexact++;
    CHECK(inexact == 0);
}

TEST_CASE("plot warns when the ASCII cells are not square") {
    // An ASCII grid carries one cellsize, but plot[...;size=] can ask for cells
    // that are not square. writeASCII used to compute it from the x extent
    // alone, so the y spacing the file declares was wrong with no sign of it.
    Command executor;
    burnOffCentre(executor);
    const std::string path = "ff_test_anisotropic.asc";

    std::ostringstream captured;
    std::streambuf* previous = std::cout.rdbuf(captured.rdbuf());
    run(executor, "plot[parameter=arrival_time;filename=" + path + ";size=20,20]");
    std::cout.rdbuf(previous);

    AsciiGrid grid = readAsciiGrid(path);
    std::remove(path.c_str());

    /* 2000 x 1000 m in 20 x 20 cells: 100 m in x, 50 m in y. */
    CHECK(grid.ncols == 20);
    CHECK(grid.nrows == 20);
    CHECK(grid.cellsize == doctest::Approx(100.));
    CHECK(captured.str().find("not square") != std::string::npos);
}

TEST_CASE("a domain built without a propagation layer spreads with the propagationModel parameter") {
    // loadData picks the model from the propagationModel parameter, but a
    // bare FireDomain[...] had no propagation layer and goTo segfaulted.
    Command executor;
    run(executor, "clear[]");
    run(executor, "setParameter[propagationModel=Iso]");
    run(executor, "FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]");
    REQUIRE(Command::getDomain() != nullptr);
    run(executor, "startFire[loc=(500,500,0)]");
    run(executor, "goTo[t=100]");

    size_t burnt = 0;
    for (const auto& column : Command::getDomain()->getDataMatrix("arrival_time"))
        for (double t : column)
            if (std::isfinite(t)) burnt++;
    CHECK(burnt > 0);
}
  
TEST_CASE("a front that reaches the edge on sloping ground leaves the far side unburnt") {
    Command executor;
    run(executor, "clear[]");
    run(executor, "FireDomain[sw=(0,0,0);ne=(1000,1000,0);t=0]");
    auto* domain = Command::getDomain();
    REQUIRE(domain != nullptr);
    REQUIRE(domain->addPropagativeLayer("Iso"));

    /* 2 x 2 altitude rising 100 m to the east, cell centres at 250 and 750 */
    std::vector<double> altitude = {0., 0., 100., 100.};
    double x0 = 0, y0 = 0, t0 = 0, width = 1000, height = 1000, timespan = 0;
    size_t nx = 2, ny = 2, nz = 1, nt = 1;
    REQUIRE(domain->addScalarLayer("data", "altitude", x0, y0, t0, width, height, timespan,
                                   nx, ny, nz, nt, altitude.data()));

    run(executor, "startFire[loc=(900,500,0)]");
    run(executor, "goTo[t=200]");

    /* Iso spreads at 1 m/s: the front is past the east edge, 300 m short of
     * the west third. */
    auto matrix = domain->getDataMatrix("arrival_time");
    size_t burnt = 0, burntInWestThird = 0;
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            if (!std::isfinite(matrix[i][j])) continue;
            burnt++;
            if (i < matrix.size() / 3) burntInWestThird++;
        }
    }
    CHECK(burnt > 0);
    CHECK(burntInWestThird == 0);
}

} /* TEST_SUITE */
