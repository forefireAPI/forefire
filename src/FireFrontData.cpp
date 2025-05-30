/**
 * @file FireFrontData.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FireFrontData.h"
#include "FireDomain.h"

using namespace std;

namespace libforefire {

FireFrontData::FireFrontData() {
}

FireFrontData::FireFrontData(FireFront* ff){
	time = ff->getTime();
	numFirenodes = ff->getNumFN();
	containingFront = 0;
	domain = ff->getDomain();
	if ( numFirenodes > 0 ){
		FireNode* curfn = ff->getHead();
		for ( size_t numfn = 0; numfn < numFirenodes; numfn++ ){
			nodes.push_back(new FireNodeData(curfn));
			curfn = curfn->getNext();
		}
	}
	if ( ff->getInnerFronts().size() > 0 ){
		list<FireFront*> fronts = ff->getInnerFronts();
		list<FireFront*>::iterator front;
		FireFrontData* newfront = 0;
		for ( front = fronts.begin(); front != fronts.end(); ++front ){
			if ( (*front)->getNumFN() > 3 ){
				newfront = new FireFrontData(*front);
				addInnerFront(newfront);
				newfront->setContFront(this);
			}
		}
	}
}

FireFrontData::~FireFrontData() {
	list<FireFrontData*>::iterator front;
	for ( front = innerFronts.begin(); front != innerFronts.end(); ++front ){
		delete *front;
	}
	innerFronts.clear();
	list<FireNodeData*>::iterator node;
	for ( node = nodes.begin(); node != nodes.end(); ++node ){
		delete *node;
	}
	nodes.clear();
}

void FireFrontData::reconstructState(FireFront* ff){
	if ( nodes.size() > 0 ){
		list<FireNodeData*>::iterator node = nodes.begin();
		FireNode* curfn = domain->addFireNode(*node, ff);
		FireNode* prev;
		++node;
		while ( node != nodes.end() ){
			prev = curfn;
			curfn = domain->addFireNode(*node, ff, prev);
			++node;
		}
	}
	list<FireFrontData*>::iterator front;
	FireFront* newfront;
	for ( front = innerFronts.begin();
			front != innerFronts.end(); ++front ){
		newfront = domain->addFireFront((*front)->getTime(), ff);
		(*front)->reconstructState(newfront);
		newfront->setContFront(ff);
	}
}

void FireFrontData::setContFront(FireFrontData* ff){
	containingFront = ff;
}

void FireFrontData::addInnerFront(FireFrontData* ff){
	innerFronts.push_back(ff);
}

double FireFrontData::getTime(){
	return time;
}

}
