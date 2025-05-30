/**
 * @file FireNodeData.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FireNodeData.h"

namespace libforefire {

FireNodeData::FireNodeData() {
}

FireNodeData::FireNodeData(
		const double& tid, const double& px, const double& py
		, const double vx, const double& vy, const double& t
		, const double& tpid, const double& tnid
		, const double& fdepth, const double& kappa, string curstate)
: id(tid), posX(px), posY(py), velX(vx), velY(vy)
, time(t), pid(tpid), nid(tnid)
, fdepth(fdepth), curvature(kappa), state(curstate) {
}

FireNodeData::FireNodeData(FireNode* fn){
	id = fn->getIDtoDouble();
	posX = fn->getLoc().getX();
	posY = fn->getLoc().getY();
	velX = fn->getVel().getVx();
	velY = fn->getVel().getVy();
	time = fn->getTime();
	fn->getPrev() != 0 ? pid = fn->getPrev()->getIDtoDouble() : pid = 0;
	fn->getNext() != 0 ? nid = fn->getNext()->getIDtoDouble() : nid = 0;
	fdepth = fn->getFrontDepth();
	curvature = fn->getCurvature();
	state = fn->getStateString(fn->getState());
}

FireNodeData::~FireNodeData() {
}

double FireNodeData::distance(FireNodeData* fn){
	return sqrt((fn->posX-posX)*(fn->posX-posX)
			+ (fn->posY-posY)*(fn->posY-posY));
}

double FireNodeData::distance(FireNode* fn){
	return sqrt((fn->getX()-posX)*(fn->getX()-posX)
			+ (fn->getY()-posY)*(fn->getY()-posY));
}

double FireNodeData::distance(FFPoint& loc){
	return distance(loc.getX(), loc.getY());
}

double FireNodeData::distance(const double& px, const double& py){
	return sqrt((px-posX)*(px-posX) + (py-posY)*(py-posY));
}

string FireNodeData::toString(){
	ostringstream oss;
	oss << "FireNode[loc=("<<posX<<","<<posY<<");vel=("
			<<velX<<','<<velY<<");t="<<time<<"] for id "
			<<((long)id)%10000000<<", "<<((long)id)/10000000
			<<", state="<<state;
	return  oss.str();
}

}
