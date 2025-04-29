/**
 * @file HeatFluxFromObsModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../FluxModel.h"
#include "../FireDomain.h"
#include "FromObsModels.h"
using namespace std;

namespace libforefire {

class HeatFluxFromObsModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
    size_t evaporationTime_data;
    size_t residenceTime_data;
    size_t burningTime_data;
    size_t nominalHeatFlux_f_data;
    size_t nominalHeatFlux_s_data;

	/*! coefficients needed by the model */
	//double burningTime;
	//double residenceTime;

	/*double nominalHeatFlux;*/

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	HeatFluxFromObsModel(const int& = 0, DataBroker* = 0);
	virtual ~HeatFluxFromObsModel();

	string getName();
    
};


SensibleheatFlux  computeHeatFLuxFromBmap(const double& burningTime, const double& residenceTime, const double&  nominalHeatFlux_f, const double&  nominalHeatFlux_s, 
		 const double& bt, const double& et, const double& at){
	
    /* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of 'burningDuration', constant of the model */

	/* Instantaneous flux */
	/* ------------------ */

	if ( bt == et ){
		if ( bt < at ) return SensibleheatFlux(0.,0.);
		if ( bt < at + residenceTime ) return SensibleheatFlux(nominalHeatFlux_f, 0.);
		if ( bt < at + burningTime )   return SensibleheatFlux(0., nominalHeatFlux_s);
		return SensibleheatFlux(0.,0.);
	}

	/* Averaged flux */
	/* ------------- */

	/* looking outside burning interval */
	if ( et < at or bt > at + burningTime ) return SensibleheatFlux( 0.,0.);
	

    
    /* begin time outside interval, end time inside flaming time*/
	if ( bt < at and et <= at + residenceTime ) return SensibleheatFlux(nominalHeatFlux_f*(et-at)/(et-bt), 0.);
    
    /* begin time outside interval, end time inside smoldering time*/
	if ( bt < at and et <= at + burningTime ) return SensibleheatFlux(nominalHeatFlux_f*residenceTime/ (et-bt),  nominalHeatFlux_s*(et-(at+residenceTime)) / (et-bt) );
	
    /* begin time outside interval, end time outside */
	if ( bt < at and et > at + burningTime ) return  SensibleheatFlux(nominalHeatFlux_f*residenceTime / (et-bt),  nominalHeatFlux_s*(burningTime-residenceTime) / (et-bt) );
	


    /* begin time inside flaming, end time inside flaming */
	if ( bt >= at and bt <= at + residenceTime and et <= at + residenceTime ) return SensibleheatFlux(nominalHeatFlux_f, 0. );
    
    /* begin time inside flaming, end time inside smoldering*/
	if ( bt >= at and bt <= at + residenceTime and et <= at + burningTime   ) return SensibleheatFlux(nominalHeatFlux_f*(at+residenceTime-bt) / (et-bt) , nominalHeatFlux_s*(et-(at+residenceTime)) / (et-bt) );
	
    /* begin time insObsModel.cpp:129: warning: coide flaming, end time outside*/
	if ( bt >= at and bt <= at + residenceTime and et > at + burningTime   ) return SensibleheatFlux(nominalHeatFlux_f*(at+residenceTime-bt) / (et-bt) , nominalHeatFlux_s*(burningTime-residenceTime) / (et-bt));



    /* begin time inside smoldering, end time inside smoldering */
	if ( bt >= at+residenceTime and bt <= at+burningTime and et <= at+burningTime ) return SensibleheatFlux(0., nominalHeatFlux_s);
    
    /* begin time inside smoldering, end time outside*/
	if ( bt >= at+residenceTime and bt <= at+burningTime and et >  at+burningTime   ) return SensibleheatFlux(0., nominalHeatFlux_s*((burningTime+at)-bt) / (et-bt)) ;

    return  SensibleheatFlux( -999.,-999.);

    //cout << "merde" << endl;

}


FluxModel* getHeatFluxFromObsModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string HeatFluxFromObsModel::name = "heatFluxFromObs";

/* registration */
int HeatFluxFromObsModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getHeatFluxFromObsModel );

/* instantiation */
FluxModel* getHeatFluxFromObsModel(const int& index, DataBroker* db) {
	return new HeatFluxFromObsModel(index, db);
}

/* constructor */
HeatFluxFromObsModel::HeatFluxFromObsModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	evaporationTime_data = registerProperty("FromObs_evaporationTime");
    residenceTime_data = registerProperty("FromObs_residenceTime");
	burningTime_data = registerProperty("FromObs_burningTime");
	nominalHeatFlux_f_data = registerProperty("FromObs_NominalHeatFlux_flaming");
	nominalHeatFlux_s_data = registerProperty("FromObs_NominalHeatFlux_smoldering");

    /* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	/*nominalHeatFlux = 150000.;
	if ( params->isValued("nominalHeatFlux") )
		nominalHeatFlux = params->getDouble("nominalHeatFlux");*/

}




/* destructor (shoudn't be modified) */
HeatFluxFromObsModel::~HeatFluxFromObsModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string HeatFluxFromObsModel::getName(){
	return name;
}



double HeatFluxFromObsModel::getValue(double* valueOf
, const double& bt, const double& et, const double& at){
    /* Mean heat flux released between the time interval [bt, et] */
/* The heat flux is supposed to be constant from the arrival time (at)
* and for a period of time of 'burningDuration', constant of the model */


    double burningTime       = valueOf[burningTime_data];
    double residenceTime     = valueOf[residenceTime_data];
    double evaporationTime     = valueOf[evaporationTime_data];
    double nominalHeatFlux_f = valueOf[nominalHeatFlux_f_data];
    double nominalHeatFlux_s = valueOf[nominalHeatFlux_s_data];
    SensibleheatFlux sensibleheatFlux = computeHeatFLuxFromBmap(burningTime,residenceTime,nominalHeatFlux_f,nominalHeatFlux_s,bt,et,at+evaporationTime);
if (at >= 0)
cout << "formObs " << et << ' ' << bt << ' ' << at << ' ' << "  -  "
             <<                     burningTime/500 << '|' << residenceTime
             << ' ' << nominalHeatFlux_f/1.8e6        << '|' <<nominalHeatFlux_s << ' '
             << ' ' << sensibleheatFlux.flaming << '|' <<  sensibleheatFlux.smoldering << endl;
return sensibleheatFlux.flaming + sensibleheatFlux.smoldering ;
}

} /* namespace libforefire */
