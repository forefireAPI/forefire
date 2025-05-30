/**
 * @file CraterSO2FluxModel.cpp
 * @brief  SO2 flux model for a Volcano Crater (Durand PhD thesis).
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../FluxModel.h"
#include "../FireDomain.h"
#include "../include/Futils.h"
using namespace std;
namespace libforefire {

class CraterSO2FluxModel: public FluxModel {

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
	CraterSO2FluxModel(const int& = 0, DataBroker* = 0);
	virtual ~CraterSO2FluxModel();

	string getName();
};

FluxModel* getCraterSO2FluxModel(const int& = 0, DataBroker* = 0);


/* name of the model */
const string CraterSO2FluxModel::name = "CraterSO2Flux";

/* instantiation */
FluxModel* getCraterSO2FluxModel(const int& index, DataBroker* db) {
	return new CraterSO2FluxModel(index, db);
}

/* registration */
int CraterSO2FluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getCraterSO2FluxModel );

/* constructor */
CraterSO2FluxModel::CraterSO2FluxModel(
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
	craterArea = 1600.;
	if ( params->isValued("crater.area") )
		craterArea = params->getDouble("crater.area");
	if ( !params->isValued("SO2.hours") )
		cout<<"ERROR: vector of parameters SO2.hours should be valued"<<endl;
	refHours = params->getDoubleArray("SO2.hours");
	if ( !params->isValued("SO2.flows") )
		cout<<"ERROR: vector of parameters SO2.flows should be valued"<<endl;
	refFlows = params->getDoubleArray("SO2.flows");
	emissionRatio = 1.0;
	if ( params->isValued("SO2.craterRatio") )
		emissionRatio = params->getDouble("SO2.craterRatio");

	/* local variables */
	// Error estimate correction
	convert =1.;
	// converting from kg.s-1 to molecules.s-1
	convert = 6.022e23*(convert/64.e-3);
}

/* destructor (shoudn't be modified) */
CraterSO2FluxModel::~CraterSO2FluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string CraterSO2FluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double CraterSO2FluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){

	if ( bt - eruptionTime < 0 ) return 0.;

	/* getting the hours since eruption */
	double hoursSinceEruption = (bt-eruptionTime)/3600.;
	if ( hoursSinceEruption > refHours.back() ) return 0.;

	/* getting the index of vector of fluxes */
	size_t hind = 0;
	size_t nhours = refHours.size();
	while ( hind+1 < nhours and refHours[hind+1] < hoursSinceEruption ) hind++;
	/* interpolation of the flux between the values */
	double beta = (hoursSinceEruption-refHours[hind])
			/(refHours[hind+1]-refHours[hind]);

	double flux = beta*refFlows[hind+1] + (1.-beta)*refFlows[hind];
	double craso2 = convert*emissionRatio*flux;
	//cout <<  " craso2 "  <<  craso2   << " convert "  << convert  << " flux " <<  flux  << endl;
/* CraterSO2Flux.activeArea = surface active en m² du cratere
// converting from molecule.s-1 to molecules.m².s-1*/
//		double t = bt-at;
//		cout << " bt " << bt << " at " << at << " et " << et << endl;
//		cout << "bt-at " << t << endl;
//		return craso2/params->getDouble("CraterSO2Flux.activeArea");
		return craso2/931;
}

} /* namespace libforefire */
