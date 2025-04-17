/**
 * @file ForeFireV1VaporFluxModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "FireDomain.h"
#include <math.h>

using namespace std;

namespace libforefire {

class ForeFireV1VaporFluxModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t sd;
	size_t md;
	size_t sigmad;
	size_t ml;
	size_t sigmal;
	size_t lai;
	size_t m_e;

	/*! coefficients needed by the model */
	double BurrowsCst;
	double liveTimeScaleRatio;
	double timeScalesRatio;
	vector<double> LAICoeffs;
	vector<double> mCoeffs;
	double chi_b;

	/*! local variables */

	/*! local functions */
	double texp(double&, double&);
	double tauexp(double&, double&);

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	ForeFireV1VaporFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~ForeFireV1VaporFluxModel();

	string getName();
};

FluxModel* getForeFireV1VaporFluxModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string ForeFireV1VaporFluxModel::name = "ForeFireV1VaporFlux";

/* registration */
int ForeFireV1VaporFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getForeFireV1VaporFluxModel );

/* instantiation */
FluxModel* getForeFireV1VaporFluxModel(const int& index, DataBroker* db) {
	return new ForeFireV1VaporFluxModel(index, db);
}

/* constructor */
ForeFireV1VaporFluxModel::ForeFireV1VaporFluxModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	sd = registerProperty("fuel.sd");
	md = registerProperty("fuel.Md");
	sigmad = registerProperty("fuel.Sigmad");
	ml = registerProperty("fuel.Ml");
	sigmal = registerProperty("fuel.Sigmal");
	lai = registerProperty("fuel.Blai");
	m_e = registerProperty("fuel.me");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	BurrowsCst = 208487;
	if ( params->isValued("FFfluxes.BurrowsCst") )
		BurrowsCst = params->getDouble("FFfluxes.BurrowsCst");
	liveTimeScaleRatio = 5.;
	if ( params->isValued("FFfluxes.liveTimeScaleRatio") )
		liveTimeScaleRatio = params->getDouble("FFfluxes.liveTimeScaleRatio");
	timeScalesRatio = 0.5;
	if ( params->isValued("FFfluxes.timeScalesRatio") )
		timeScalesRatio = params->getDouble("FF.timeScalesRatio");
	if ( !params->isValued("FFfluxes.LAICoeffs") )
		cout<<"ERROR: vector of parameters FFfluxes.LAICoeffs should be valued"<<endl;
	LAICoeffs = params->getDoubleArray("FFfluxes.LAICoeffs");
	if ( !params->isValued("FFfluxes.mCoeffs") )
		cout<<"ERROR: vector of parameters FFfluxes.mCoeffs should be valued"<<endl;
	mCoeffs = params->getDoubleArray("FFfluxes.mCoeffs");
	chi_b = 1.;
	if ( params->isValued("FFfluxes.chi_b") )
		chi_b = params->getDouble("FFfluxes.chi_b");
}

/* destructor (shoudn't be modified) */
ForeFireV1VaporFluxModel::~ForeFireV1VaporFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string ForeFireV1VaporFluxModel::getName(){
	return name;
}

double ForeFireV1VaporFluxModel::texp(double& t, double& tau){
	return t/tau/tau*exp(-t/tau);
}

double ForeFireV1VaporFluxModel::tauexp(double& t, double& tau){
	return (t+tau)/tau*exp(-t/tau);
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double ForeFireV1VaporFluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){

	/* see ForeFire manuals for flux models */

	double T0 = BurrowsCst/pow(valueOf[sd],1.236)*timeScalesRatio;
	double LAICorr = 1. + LAICoeffs[0]*(valueOf[lai]-2)*(valueOf[lai]-2)
			/(valueOf[lai]+LAICoeffs[1])/(valueOf[lai]+LAICoeffs[1]);
	double moistCorr = mCoeffs[0] + mCoeffs[1]*exp(mCoeffs[2]*valueOf[md])
		 + mCoeffs[3]*exp(-mCoeffs[4]*valueOf[md]);
	double taud = LAICorr*moistCorr*T0;
	double taul = liveTimeScaleRatio*taud;
	double deadPotVaporMass = valueOf[md]*valueOf[sigmad];
	double livePotVaporMass = chi_b*exp(-4.*valueOf[ml]/m_e)*valueOf[ml]*valueOf[sigmal];

	/* Instantaneous flux */
	/* ------------------ */
	if ( bt == et ){
		if ( bt < at ) return 0;
		double dtb = bt - at;
		return deadPotVaporMass/taud/taud*texp(dtb,taud)
				+ livePotVaporMass/taul/taul*texp(dtb,taul);
	}

	/* Averaged flux */
	/* ------------- */
	if ( et < at ) return 0;
	double dtb = bt - at;
	if ( dtb < 0 ) dtb = 0.;
	double dte = et - at;
	return ( deadPotVaporMass*(tauexp(dtb,taud)-tauexp(dte,taud))
		   + livePotVaporMass*(tauexp(dtb,taul)-tauexp(dte,taul)) )/(et-bt);

}

} /* namespace libforefire */
