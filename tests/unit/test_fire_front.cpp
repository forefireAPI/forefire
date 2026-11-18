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

#include "NCXYZTDataLayer.h"
#include "DataBroker.h"
#include "PropagationModel.h"

TEST_SUITE("fire front") {
TEST_CASE("terrain-corrected weighted medians are perpendicular and preserve slope input units") {
    struct RestoreScheme { FireNode::NormalScheme scheme=FireNode::nmlScheme; ~RestoreScheme(){FireNode::nmlScheme=scheme;} } restore;
    for (auto gradient : {std::pair<double,double>{0,0}, {1,1}, {.4,-.2}}) {
        ModelSandbox sandbox;
        auto* domain=sandbox.fireDomain(); TimeTable schedule; domain->setTimeTable(&schedule);
        const double gx=gradient.first,gy=gradient.second;
        std::vector<double> heights;
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)heights.push_back(1000+gx*(i+.5)*1000/3+gy*(j+.5)*1000/3);
        FFPoint origin(0,0,0),extent(1000,1000,0);double t0=0,span=0;size_t nx=3,ny=3,nz=1,nt=1;
        auto* altitude=new libforefire::XYZTDataLayer<double>("altitude",origin,t0,extent,span,nx,ny,nz,nt,heights.data());
        auto* broker=domain->getDataBroker();broker->registerLayer("altitude",altitude);
        auto* front=domain->addFireFront(0.); FireNode* prev=nullptr;FireNode* node=nullptr;
        for(double y:{600.,500.,400.}){FFPoint p(500,y,1000+gx*500+gy*y);FFVector v;prev=domain->addFireNode(p,v,0.,20.,0.,front,prev);if(y==500)node=prev;}
        REQUIRE(node);
        FireNode::setNormalScheme("weightedMedians");node->computeLocalFrontProperties();auto legacy=node->getNormal();
        CHECK(legacy.getVx()==doctest::Approx(1/std::sqrt(1+gx*gx)));
        CHECK(legacy.getVy()==doctest::Approx(0));CHECK(legacy.getVz()==doctest::Approx(gx/std::sqrt(1+gx*gx)));
        FireNode::setNormalScheme("slopeCorrectedWeightedMedians");node->computeLocalFrontProperties();auto n=node->getNormal();
        CHECK(n.norm()==doctest::Approx(1));
        CHECK(n.getVy()+gy*n.getVz()==doctest::Approx(0).epsilon(1e-10));
        CHECK(-gx*n.getVx()-gy*n.getVy()+n.getVz()==doctest::Approx(0).epsilon(1e-10));
        auto* model=sandbox.propagation("Rothermel");broker->getPropagationData(model,node);
        bool checked=false;
        for(size_t i=0;i<model->wantedProperties.size();++i)if(model->wantedProperties[i]=="slope"){
            const double expected=(gx*n.getVx()+gy*n.getVy())/std::hypot(n.getVx(),n.getVy());
            CHECK(model->properties[i]==doctest::Approx(expected));checked=true;
        }
        CHECK(checked);
    }
}
}
