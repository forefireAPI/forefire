/**
 * @file WindDrivenPropagationModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "PropagationModel.h"
#include "FireDomain.h"
#include <math.h>

namespace libforefire {

class WindDrivenPropagationModel: public PropagationModel {
    /*! name the model */
	static const string name;
    /*! boolean for initialization */
	static int isInitialized;
    /*! properties needed by the model */
	size_t vv_coeff;
	size_t normalWind;
	double windReductionFactor;
	double getSpeed(double*);

public:
	WindDrivenPropagationModel(const int& = 0, DataBroker* db=0);
	virtual ~WindDrivenPropagationModel();
	string getName();
};
/* instantiation */
PropagationModel* getWindDrivenPropagationModelModel(const int& = 0, DataBroker* db=0);

} 
using namespace std;

namespace libforefire {
const string WindDrivenPropagationModel::name = "WindDriven";
PropagationModel* getWindDrivenPropagationModelModel(const int & mindex, DataBroker* db) {
	return new WindDrivenPropagationModel(mindex, db);
}

int WindDrivenPropagationModel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getWindDrivenPropagationModelModel );
/* constructor */
WindDrivenPropagationModel::WindDrivenPropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	windReductionFactor = params->getDouble("windReductionFactor");
	normalWind = registerProperty("normalWind");
	vv_coeff = registerProperty("fuel.vv_coeff");
	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);

}
/* destructor (shoudn't be modified) */
WindDrivenPropagationModel::~WindDrivenPropagationModel() {
}
/* accessor to the name of the model */
string WindDrivenPropagationModel::getName(){
	return name;
}
/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */
double WindDrivenPropagationModel::getSpeed(double* valueOf){

	double lvv_coeff = valueOf[vv_coeff];
	double WROS = valueOf[normalWind]; 
	double ROS = lvv_coeff*WROS;
	// cout<<" ROS :  "<< ROS <<"   WROS:  "<<WROS<<endl;

	return ROS;
}

}
