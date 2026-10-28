/**
 * @file test_topology.cpp
 * @brief FireDomain::checkTopology on a front reduced to two nodes.
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

using libforefire::FFPoint;
using libforefire::FFVector;
using libforefire::FireDomain;
using libforefire::FireFront;
using libforefire::FireNode;
using libforefire::TimeTable;
using ff_test::ModelSandbox;

TEST_SUITE("topology") {

TEST_CASE("a front reduced to two nodes does not keep a trashed head") {
    ModelSandbox sandbox;
    FireDomain* domain = sandbox.fireDomain();
    TimeTable schedule;
    domain->setTimeTable(&schedule);
    FireFront* front = domain->addFireFront(0.);

    FFVector vel;
    FFPoint a(500., 500., 0.), b(510., 500., 0.);
    FireNode* first = domain->addFireNode(a, vel, 0., 20., 0., front);
    domain->addFireNode(b, vel, 0., 20., 0., front, first);
    REQUIRE(front->getNumFN() == 2);
    REQUIRE(front->getHead() == first);

    // Previous and next are the same node: both nodes are trashed.
    domain->checkTopology(first);

    CHECK(front->getHead() == nullptr);
}

}
