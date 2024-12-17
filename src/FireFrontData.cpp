/*

  Copyright (C) 2012 ForeFire Team, SPE, CNRS/Universita di Corsica.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 US

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
