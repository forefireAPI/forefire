/**
 * @file FireFrontData.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
