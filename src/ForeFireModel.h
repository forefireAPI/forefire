/**
 * @file ForeFireModel.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
