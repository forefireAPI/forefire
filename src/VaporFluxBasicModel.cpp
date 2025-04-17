/**
 * @file VaporFluxBasicModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "FireDomain.h"

using namespace std;

namespace libforefire {

class VaporFluxBasicModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double burningDuration;
	double nominalVaporFlux;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	VaporFluxBasicModel(const int& = 0, DataBroker* = 0);
	virtual ~VaporFluxBasicModel();

	string getName();
};

FluxModel* getVaporFluxBasicModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string VaporFluxBasicModel::name = "vaporFluxBasic";

/* instantiation */
FluxModel* getVaporFluxBasicModel(const int& index, DataBroker* db) {
	return new VaporFluxBasicModel(index, db);
}

/* registration */
int VaporFluxBasicModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getVaporFluxBasicModel );

/* constructor */
VaporFluxBasicModel::VaporFluxBasicModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	burningDuration = 30.;
	if ( params->isValued("burningDuration") )
		burningDuration = params->getDouble("burningDuration");
	nominalVaporFlux = 1.;
	if ( params->isValued("nominalVaporFlux") )
		nominalVaporFlux = params->getDouble("nominalVaporFlux");
}

/* destructor (shoudn't be modified) */
VaporFluxBasicModel::~VaporFluxBasicModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string VaporFluxBasicModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double VaporFluxBasicModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	/* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of 'burningDuration', constant of the model */

	/* Instantaneous flux */
	/* ------------------ */
	if ( bt == et ){
		if ( bt < at ) return 0;
		if ( bt < at + burningDuration ) return nominalVaporFlux;
		return 0;
	}

	/* Averaged flux */
	/* ------------- */

	/* looking outside burning interval */
	if ( et < at or bt > at + burningDuration ) return 0;
	/* begin time outside interval, end time inside */
	if ( bt < at and et <= at + burningDuration ) return nominalVaporFlux*(et-at)/(et-bt);
	/* begin time outside interval, end time outside */
	if ( bt < at and et > at + burningDuration ) return nominalVaporFlux*burningDuration/(et-bt);
	/* begin time inside interval, end time inside */
	if ( bt >= at and et <= at + burningDuration ) return nominalVaporFlux;
	/* begin time inside interval, end time outside */
	return nominalVaporFlux*(at+burningDuration-bt)/(et-bt);

}

} /* namespace libforefire */
