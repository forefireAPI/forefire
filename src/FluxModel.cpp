/**
 * @file FluxModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "DataBroker.h"

using namespace std;

namespace libforefire {

FluxModel* getDefaultFluxModel(const int & mindex, DataBroker* db) {
	return new FluxModel(mindex, db);
}

FluxModel::FluxModel(const int & mindex, DataBroker* db)
	: ForeFireModel(mindex, db) {
}

FluxModel::~FluxModel() {
}

double FluxModel::getValueAt(FFPoint& loc
		, const double& bt, const double& et, const double& at){
	dataBroker->getFluxData(this, loc, bt);
	return getValue(properties, bt, et, at);
}

} /* namespace libforefire */
