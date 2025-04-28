/**
 * @file HeatFluxNominalModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../FluxModel.h"
#include "../FireDomain.h"

namespace libforefire {

class HeatFluxNominalModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t tau0;
	size_t sd;

	/*! coefficients of the model */
	double nominalHeatFlux;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	HeatFluxNominalModel(const int& = 0, DataBroker* = 0);
	virtual ~HeatFluxNominalModel();

	string getName();
};

FluxModel* getHeatFluxNominalModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string HeatFluxNominalModel::name = "heatFluxNominal";

/* registration */
int HeatFluxNominalModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getHeatFluxNominalModel );

/* instantiation */
FluxModel* getHeatFluxNominalModel(const int& index, DataBroker* db) {
	return new HeatFluxNominalModel(index, db);
}

/* constructor */
HeatFluxNominalModel::HeatFluxNominalModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	tau0 = registerProperty("fuel.Tau0");
	sd = registerProperty("fuel.sd");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	nominalHeatFlux = 150000.;
	if ( params->isValued("nominalHeatFlux") )
		nominalHeatFlux = params->getDouble("nominalHeatFlux");
}

/* destructor (shoudn't be modified) */
HeatFluxNominalModel::~HeatFluxNominalModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string HeatFluxNominalModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double HeatFluxNominalModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	/* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of given by fuel properties tau0/sd */

	double burningDuration = valueOf[tau0]/valueOf[sd];

	/* Instantaneous flux */
	/* ------------------ */
	if ( bt == et ){
		if ( bt < at ) return 0;
		if ( bt < at + burningDuration ) return nominalHeatFlux;
		return 0;
	}

	/* Averaged flux */
	/* ------------- */
	/* looking outside burning interval */
	if ( et < at or bt > at + burningDuration ) return 0;
	/* begin time outside interval, end time inside */
	if ( bt < at and et <= at + burningDuration ) return nominalHeatFlux*(et-at)/(et-bt);
	/* begin time outside interval, end time outside */
	if ( bt < at and et > at + burningDuration ) return nominalHeatFlux*burningDuration/(et-bt);
	/* begin time inside interval, end time inside */
	if ( bt >= at and et <= at + burningDuration ) return nominalHeatFlux;
	/* begin time inside interval, end time outside */
	return nominalHeatFlux*(at+burningDuration-bt)/(et-bt);

}

} /* namespace libforefire */
