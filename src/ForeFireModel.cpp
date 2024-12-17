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
