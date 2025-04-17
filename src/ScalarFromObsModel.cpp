/**
 * @file ScalarFromObsModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FluxModel.h"
#include "FireDomain.h"
#include "FromObsModels.h"

using namespace std;

namespace libforefire {

class ScalarFromObsModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
    size_t nominalHeatFlux_f_data;
    size_t nominalHeatFlux_s_data;
    
    size_t evaporationTime_data;
    size_t residenceTime_data;
    size_t burningTime_data;

    size_t EFScalar_f_data;
    size_t EFScalar_s_data;

    size_t radiation_fraction_f_data;
    size_t radiation_fraction_s_data;
	
    size_t conversion_factor_data;
    
	/*! coefficients needed by the model */

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	ScalarFromObsModel(const int& = 0, DataBroker* = 0);
	virtual ~ScalarFromObsModel();

	string getName();
};


FluxModel* getScalarFromObsModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string ScalarFromObsModel::name = "SFObs";

/* registration */
int ScalarFromObsModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getScalarFromObsModel );

/* instantiation */
FluxModel* getScalarFromObsModel(const int& index, DataBroker* db) {
	return new ScalarFromObsModel(index, db);
}

/* constructor */
ScalarFromObsModel::ScalarFromObsModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	nominalHeatFlux_f_data = registerProperty("FromObs_NominalHeatFlux_flaming");
	nominalHeatFlux_s_data = registerProperty("FromObs_NominalHeatFlux_smoldering");
	
	evaporationTime_data = registerProperty("FromObs_evaporationTime");
    residenceTime_data = registerProperty("FromObs_residenceTime");
    burningTime_data = registerProperty("FromObs_burningTime");

	EFScalar_f_data = registerProperty("FromObs_EFScalar_flaming");
	EFScalar_s_data = registerProperty("FromObs_EFScalar_smoldering");

	radiation_fraction_f_data = registerProperty("FromObs_radiationFraction_flaming");
	radiation_fraction_s_data = registerProperty("FromObs_radiationFraction_smoldering");
	
    conversion_factor_data = registerProperty("FromObs_conversionFactor");

    /* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
}

/* destructor (shoudn't be modified) */
ScalarFromObsModel::~ScalarFromObsModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string ScalarFromObsModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */
double ScalarFromObsModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	/* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of 'burningDuration', constant of the model */
    
    double nominalHeatFlux_f = valueOf[nominalHeatFlux_f_data];
    double nominalHeatFlux_s = valueOf[nominalHeatFlux_s_data];
    
    double evaporationTime   = valueOf[evaporationTime_data];
    double residenceTime     = valueOf[residenceTime_data];
    double burningTime       = valueOf[burningTime_data];
    
    double EFScalar_f = valueOf[EFScalar_f_data];
    double EFScalar_s = valueOf[EFScalar_s_data];

    double radiation_fraction_f = valueOf[radiation_fraction_f_data];
    double radiation_fraction_s = valueOf[radiation_fraction_s_data];

    double conversion_factor = valueOf[conversion_factor_data];
    
    SensibleheatFlux sensibleheatFlux = computeHeatFLuxFromBmap(burningTime,residenceTime, nominalHeatFlux_f,nominalHeatFlux_s, bt, et, evaporationTime+at);

    /*double ScalarFlux = EFScalar * (HeatFlux/(1.-radiation_fraction));      */
    double ScalarFlux =   EFScalar_f * conversion_factor * radiation_fraction_f * 1.e-6 * sensibleheatFlux.flaming    / (1.-radiation_fraction_f) 
                        + EFScalar_s * conversion_factor * radiation_fraction_s * 1.e-6 * sensibleheatFlux.smoldering / (1.-radiation_fraction_s);

	return ScalarFlux;


}

} /* namespace libforefire */
