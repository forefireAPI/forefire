/**
 * @file ForeFireModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "ForeFireModel.h"
#include "DataBroker.h"

namespace libforefire {

ForeFireModel::ForeFireModel(const int & mindex, DataBroker* db)
: dataBroker(db), index(mindex) {
	params = SimulationParameters::GetInstance();
	numProperties = 0;
	numFuelProperties = 0;
	fuelPropertiesTable = 0;
}

ForeFireModel::~ForeFireModel() {
	if ( properties != 0 ) delete [] properties;
	if ( fuelPropertiesTable != 0 ) delete [] fuelPropertiesTable;
}

void ForeFireModel::setDataBroker(DataBroker* db){
	dataBroker = db;
}

size_t ForeFireModel::registerProperty(string property){
	wantedProperties.push_back(property);
	if ( property.substr(0,4) == "fuel" ){
		fuelPropertiesNames.push_back(property.substr(5,string::npos));
		numFuelProperties++;
	}
	return numProperties++;
}

} /* namespace libforefire */
