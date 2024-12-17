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

#ifndef FIREFRONTDATA_H_
#define FIREFRONTDATA_H_

#include "ForeFireAtom.h"
#include "FireFront.h"
#include "FireNodeData.h"
#include "include/Futils.h"

namespace libforefire {

class FireDomain;

using namespace std;

//class FireNode;
class FireDomain;

class FireFrontData {

	double time; /*!< current time of the front */
	size_t numFirenodes; /*!< number of firenodes in the front */
	FireFrontData* containingFront; /*!< upper front */
	list<FireNodeData*> nodes; /*!< data of the nodes composing the front */
	list<FireFrontData*> innerFronts; /*!< inner fire fronts */
	FireDomain* domain;

public:
	FireFrontData();
	FireFrontData(FireFront*);
	virtual ~FireFrontData();

	void setContFront(FireFrontData*);
	void addInnerFront(FireFrontData*);

	double getTime();

	void reconstructState(FireFront*);

};

}

#endif /* FIREFRONTDATA_H_ */
