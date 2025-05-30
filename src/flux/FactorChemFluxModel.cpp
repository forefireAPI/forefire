/**
 * @file FactorChemFluxModel.cpp
 * @brief Smoke ratio flux model for a parametrerizable industrial  burning area (FirePlume ).
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../FluxModel.h"
#include "../FireDomain.h"
#include "../include/Futils.h"

using namespace std;

namespace libforefire {

class FactorChemFluxModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double eruptionTime;
	double lavaArea;
	vector<double> refHours;
	vector<double> refFlows;
	double emissionRatio;

	/*! local variables */
	double convert;

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:

	FactorChemFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~FactorChemFluxModel();

	string getName();
};

FluxModel* getFactorChemFluxModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string FactorChemFluxModel::name = "factorChemFlux";

FluxModel* getFactorChemFluxModel(const int& index, DataBroker* db) {
	return new FactorChemFluxModel(index, db);
}

/* registration */
int FactorChemFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getFactorChemFluxModel );
/* instantiation */


/* constructor */
FactorChemFluxModel::FactorChemFluxModel(
		const int & mindex, DataBroker* db)
	: FluxModel(mindex, db) {
	/* defining the properties needed for the model */
	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];
	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	eruptionTime = 0.;
	if ( params->isValued("lava.eruptionTime") )
		eruptionTime = params->getDouble("lava.eruptionTime");
	lavaArea = 5.5e6;
	if ( params->isValued("lava.area") )
		lavaArea = params->getDouble("lava.area");
	if ( !params->isValued("SO2.hours") )
		cout<<"ERROR: vector of parameters SO2.hours should be valued"<<endl;
	refHours = params->getDoubleArray("SO2.hours");
	if ( !params->isValued("SO2.flows") )
		cout<<"ERROR: vector of parameters SO2.flows should be valued"<<endl;
	refFlows = params->getDoubleArray("SO2.flows");
	emissionRatio = 0.;
	if ( params->isValued("SO2.craterRatio") )
		emissionRatio = 1. - params->getDouble("SO2.craterRatio");

	/* local variables */
	// Error estimate correction
	convert = 392./231.;
	// Dividing by the final area of the lava to convert to kg.m-2.s-1
	convert = convert/lavaArea;
	// converting from kg.m-2.s-1 to molecules.m-2.s-1
//	convert = convert/64.e-3;
//	convert = convert/64.e-3*6.022e23;
}

/* destructor (shoudn't be modified) */
FactorChemFluxModel::~FactorChemFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string FactorChemFluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double FactorChemFluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	if ( bt - eruptionTime < 0 ) return 0.;

	/* getting the hours since eruption */
	double hoursSinceEruption = (bt-eruptionTime)/3600.;
	if ( hoursSinceEruption > refHours.back() ) return 0.;

	/* getting the index in the vector of fluxes */
	size_t hind = 0;
	size_t nhours = refHours.size();
	while ( hind+1 < nhours and refHours[hind+1] < hoursSinceEruption ) hind++;
	/* interpolation of the flux between the values */
	double beta = (hoursSinceEruption-refHours[hind])
			/(refHours[hind+1]-refHours[hind]);
	double flux = beta*refFlows[hind+1] + (1.-beta)*refFlows[hind];
	double lavaso2 = convert*emissionRatio*flux;
//	cout << "lavaso2" <<  lavaso2  << endl;
	return lavaso2;

}

} /* namespace libforefire */
