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
