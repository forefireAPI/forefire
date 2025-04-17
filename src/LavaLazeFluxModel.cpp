/**
 * @file LavaLazeFluxModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "FireDomain.h"
#include "include/Futils.h"

using namespace std;
namespace libforefire {

class LavaLazeFluxModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double arrivalTime;
	vector<double> refHours;
	vector<double> refFlows;
	double exchangeArea;
	
	/*! local variables */
	double convert;

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:

	LavaLazeFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~LavaLazeFluxModel();

	string getName();
};

FluxModel* getLavaLazeFluxModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string LavaLazeFluxModel::name = "LavaLazeFlux";

/* instantiation */
FluxModel* getLavaLazeFluxModel(const int& index, DataBroker* db) {
	return new LavaLazeFluxModel(index, db);
}

/* registration */
int LavaLazeFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getLavaLazeFluxModel );

/* constructor */
LavaLazeFluxModel::LavaLazeFluxModel(
		const int & mindex, DataBroker* db)
	: FluxModel(mindex, db) {
	/* defining the properties needed for the model */
	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];
	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	arrivalTime = 5600.;
	if(params->isValued("vaporFlux.activeArea"))
//		cout<<params->getDouble("vaporFlux.activeArea")<<endl;
	if ( params->isValued("laze.arrivalTime") )
		arrivalTime = params->getDouble("laze.arrivalTime");
	if ( !params->isValued("laze.hours") )
		cout<<"ERROR: vector of parameters laze.hours should be valued"<<endl;
	refHours = params->getDoubleArray("laze.hours");
	if ( !params->isValued("laze.flows") )
		cout<<"ERROR: vector of parameters laze.flows should be valued"<<endl;
	refFlows = params->getDoubleArray("laze.flows");
	exchangeArea = 80000.;
	if ( params->isValued("laze.exchangeArea") )
		exchangeArea = params->getDouble("laze.exchangeArea");
	/* coefficients */
	// Dividing by the final area of exchange to convert to kg.m-2.s-1
    convert= 1./exchangeArea;
}

/* destructor (shoudn't be modified) */
LavaLazeFluxModel::~LavaLazeFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string LavaLazeFluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double LavaLazeFluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){

//	if ( bt - arrivalTime < 0 ) return 0.;


	/* getting the hours since eruption */
	double hoursSinceArrival = (bt-arrivalTime)/3600.;
	if ( hoursSinceArrival > refHours.back() ) return 0.;

	/* getting the index in the vector of fluxes */
	size_t hind = 0;
	size_t nhours = refHours.size();
	while ( hind+1 < nhours and refHours[hind+1] < hoursSinceArrival ) hind++;
	/* interpolation of the flux between the values */
	double beta = (hoursSinceArrival-refHours[hind])
			/(refHours[hind+1]-refHours[hind]);
	double flux = beta*refFlows[hind+1] + (1.-beta)*refFlows[hind];

//	cout << "LavaLaze before " << params->getDouble("LavaLazeFlux.activeArea") << endl ;
	//if(params->getDouble("LavaLazeFlux.activeArea") < 1 ) return 1;
//	double active= params->getDouble("LavaLazeFlux.activeArea") ;
//	if (active < 1 ) cout << "LavaLaze " << active << endl ;
//	cout << "LavaLaze " << active << endl ;
//	cout << " fluxh2o " << flux << endl;
//	cout << "LavaLaze " << params->getDouble("LavaLazeFlux.activeArea") +1 << endl ;
	if((bt-at) < (35*3600)) return flux/(params->getDouble("LavaLazeFlux.activeArea") +1.);

//params->getDouble("LavaLazeFlux.activeArea")

       return 0;
//	return convert*flux/vaporFlux.activeArea;
//	return flux/params->getDouble("LavaLazeFlux.activeArea");

}

} /* namespace libforefire */
