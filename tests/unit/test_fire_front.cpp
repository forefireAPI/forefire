/**
 * @file test_fire_front.cpp
 * @brief A front that merges with itself and is left with too few nodes.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "model_sandbox.h"

#include "FFPoint.h"
#include "FFVector.h"
#include "FireDomain.h"
#include "FireFront.h"
#include "FireNode.h"
#include "TimeTable.h"

#include <cmath>
#include <vector>

using libforefire::FFPoint;
using libforefire::FFVector;
using libforefire::FireDomain;
using libforefire::FireFront;
using libforefire::FireNode;
using libforefire::TimeTable;
using ff_test::ModelSandbox;

TEST_SUITE("fire front") {

TEST_CASE("a front trashed when it merges with itself takes all its nodes with it") {
    ModelSandbox sandbox;
    FireDomain* domain = sandbox.fireDomain();
    TimeTable schedule;
    domain->setTimeTable(&schedule);
    FireFront* front = domain->addFireFront(0.);

    // Eight nodes on a circle, clockwise, as an expanding front.
    std::vector<FireNode*> nodes;
    FireNode* prev = 0;
    for (int i = 0; i < 8; i++) {
        double angle = -2. * M_PI * i / 8.;
        FFPoint loc(500. + 20. * std::cos(angle), 500. + 20. * std::sin(angle), 0.);
        FFVector vel;
        prev = domain->addFireNode(loc, vel, 0., 20., 0., front, prev);
        nodes.push_back(prev);
    }
    REQUIRE(front->getNumFN() == 8);
    REQUIRE(front->getArea() > 0.);

    // Merging nodes 0 and 4 splits the front into the loop it keeps, nodes 1
    // to 4, and an inner loop, nodes 5 to 7. The kept loop has fewer than 5
    // nodes, so the front is trashed.
    front->merge(nodes[0], nodes[4]);

    for (size_t i = 0; i < nodes.size(); i++) {
        CAPTURE(i);
        CHECK(nodes[i]->getFront() != front);
    }
}

}
