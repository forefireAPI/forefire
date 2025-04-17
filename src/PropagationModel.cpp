/**
 * @file PropagationModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "PropagationModel.h"
#include "DataBroker.h"

using namespace std;

namespace libforefire{

PropagationModel* getDefaultPropagationModel(const int & mindex, DataBroker* db) {
	return new PropagationModel(mindex, db);
}

PropagationModel::PropagationModel(const int & mindex, DataBroker* db)
: ForeFireModel(mindex, db) {
}

PropagationModel::~PropagationModel() {
}

double PropagationModel::getSpeedForNode(FireNode* fn){
	dataBroker->getPropagationData(this, fn);
	return getSpeed(properties);
}

}
