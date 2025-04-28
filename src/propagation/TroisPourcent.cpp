/**
 * @file TroisPourcent.cpp
 * @brief Implementation of the TroisPourcent class
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "TroisPourcent.h"

namespace libforefire {

/* name of the model */
const string TroisPourcent::name = "TroisPourcent";

/* instantiation */
PropagationModel* getTroisPourcentModel(const int & mindex, DataBroker* db) {
	return new TroisPourcent(mindex, db);
}

/* registration */
int TroisPourcent::isInitialized =
        FireDomain::registerPropagationModelInstantiator(name, getTroisPourcentModel );

/* constructor */
TroisPourcent::TroisPourcent(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	/* defining the properties needed for the model */
	effectiveSlope = registerProperty("slope");
	normalWind = registerProperty("normalWind");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerPropagationModel(this);

	/* Definition of the coefficients */
	R0 = 0.01;
	if ( params->isValued("TroisPourcent.R0") )
		R0 = params->getDouble("TroisPourcent.R0");
	windFactor = 0.03;
	if ( params->isValued("TroisPourcent.windFactor") )
		windFactor = params->getDouble("TroisPourcent.windFactor");
	slopeFactor = 0.00;
	if ( params->isValued("TroisPourcent.slopeFactor") )
		slopeFactor = params->getDouble("TroisPourcent.slopeFactor");
}

/* destructor (shoudn't be modified) */
TroisPourcent::~TroisPourcent() {
}

/* accessor to the name of the model */
string TroisPourcent::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double TroisPourcent::getSpeed(double* valueOf){
	double overspeed = windFactor*valueOf[normalWind] + slopeFactor*(1.+valueOf[effectiveSlope]);
	if ( overspeed > 0. ) {
		return R0 + overspeed;
	} else {
		return R0;
	}
}

}
