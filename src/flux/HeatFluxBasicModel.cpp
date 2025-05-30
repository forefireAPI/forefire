/**
 * @file HeatFluxBasicModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../FluxModel.h"
#include "../FireDomain.h"

using namespace std;

namespace libforefire {

class HeatFluxBasicModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double burningDuration;
	double nominalHeatFlux;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	HeatFluxBasicModel(const int& = 0, DataBroker* = 0);
	virtual ~HeatFluxBasicModel();

	string getName();
};

FluxModel* getHeatFluxBasicModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string HeatFluxBasicModel::name = "heatFluxBasic";

/* registration */
int HeatFluxBasicModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getHeatFluxBasicModel );

/* instantiation */
FluxModel* getHeatFluxBasicModel(const int& index, DataBroker* db) {
	return new HeatFluxBasicModel(index, db);
}

/* constructor */
HeatFluxBasicModel::HeatFluxBasicModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
 
	dataBroker->registerFluxModel(this);
 
	/* Definition of the coefficients */
	burningDuration = 300.;
	if ( params->isValued("burningDuration") )
		burningDuration = params->getDouble("burningDuration");
	nominalHeatFlux = 1000000.;
	if ( params->isValued("nominalHeatFlux") )
		nominalHeatFlux = params->getDouble("nominalHeatFlux");
}

/* destructor (shoudn't be modified) */
HeatFluxBasicModel::~HeatFluxBasicModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string HeatFluxBasicModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ***	*************** */
#include <iostream>

double HeatFluxBasicModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	/* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of 'burningDuration', constant of the model */

	/* Instantaneous flux */
	/* ------------------ */
    double v = 0;
	if ( bt == et ){
		if ( bt < at ) v = 0;
		else if ( bt < at + burningDuration ) v = nominalHeatFlux;
		else v = 0; 
		return v;
	}

	/* Averaged flux */
	/* ------------- */

	/* looking outside burning interval */
	if ( et < at or bt > at + burningDuration ) {
		v = 0;
		return v;
	}

	/* begin time outside interval, end time inside */
	if ( bt < at and et <= at + burningDuration ) {
		v = nominalHeatFlux * (et - at) / (et - bt);
	 
		return v;
	}

	/* begin time outside interval, end time outside */
	if ( bt < at and et > at + burningDuration ) {
		v = nominalHeatFlux * burningDuration / (et - bt);
	 
		return v;
	}

	/* begin time inside interval, end time inside */
	if ( bt >= at and et <= at + burningDuration ) {
		v = nominalHeatFlux;
		 		return v;
	}

	/* begin time inside interval, end time outside */
	v = nominalHeatFlux * (at + burningDuration - bt) / (et - bt); 
	return v;
}

} /* namespace libforefire */
