/**
 * @file CraterVaporFluxModel.cpp
 * @brief  Vapor flux model for a Volcano Crater (Durand PhD thesis).
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "FireDomain.h"
#include "include/Futils.h"
using namespace std;
namespace libforefire {

class CraterVaporFluxModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double eruptionTime;
	double craterArea;
	vector<double> refHours;
	vector<double> refFlows;
	double emissionRatio;

	/*! local variables */
	double convert;

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	CraterVaporFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~CraterVaporFluxModel();

	string getName();
};

FluxModel* getCraterVaporFluxModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string CraterVaporFluxModel::name = "CraterVaporFluxModel";

/* instantiation */
FluxModel* getCraterVaporFluxModel(const int& index, DataBroker* db) {
	return new CraterVaporFluxModel(index, db);
}

/* registration */
int CraterVaporFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getCraterVaporFluxModel );

/* constructor */
CraterVaporFluxModel::CraterVaporFluxModel(
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
	craterArea = 30.*30.;
	if ( params->isValued("crater.area") )
		craterArea = params->getDouble("crater.area");
	if ( !params->isValued("crater.hours") )
		cout<<"ERROR: vector of parameters crater.hours should be valued"<<endl;
	refHours = refHours = params->getDoubleArray("crater.hours");
	if ( !params->isValued("crater.flows") )
		cout<<"ERROR: vector of parameters crater.flows should be valued"<<endl;
	refFlows = params->getDoubleArray("crater.flows");
	emissionRatio = 0.008;
	if ( params->isValued("crater.vaporEmissionRatio") )
		emissionRatio = params->getDouble("crater.vaporEmissionRatio");

	/* local variables */
	convert = emissionRatio/craterArea;
}

/* destructor (shoudn't be modified) */
CraterVaporFluxModel::~CraterVaporFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string CraterVaporFluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double CraterVaporFluxModel::getValue(double* valueOf
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
	double vapor = convert*flux;
	return vapor;
}

} /* namespace libforefire */
