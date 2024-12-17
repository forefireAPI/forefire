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

#ifndef FOREFIREMODEL_H_
#define FOREFIREMODEL_H_

#include "FFPoint.h"
#include "FFVector.h"
#include "FFArrays.h"
#include "SimulationParameters.h"

using namespace std;

namespace libforefire {

class DataBroker;

class ForeFireModel {

protected:

	/*! Link to data handler */
	DataBroker* dataBroker;

	/*! Link to the parameters */
	SimulationParameters* params;

	/*! registering a needed property */
	size_t registerProperty(string);

public:

	int index; /*!< Index of the model in the data broker storage */
	size_t numProperties; /*!< number of properties needed by the model */
	vector<string> wantedProperties; /*!< names of the properties needed by the model */
	size_t numFuelProperties; /*!< number of fuel properties needed by the model */
	vector<string> fuelPropertiesNames; /*!< names of desired fuel properties */
	FFArray<double>* fuelPropertiesTable; /*!< table of values for the desired fuel properties */

	/*! vector containing the data relative to the needed properties */
	double* properties;

	ForeFireModel(const int& = 0, DataBroker* = 0);
	virtual ~ForeFireModel();

	virtual string getName(){return "stub model";}

	void setDataBroker(DataBroker*);
};

} /* namespace libforefire */
#endif /* FOREFIREMODEL_H_ */
