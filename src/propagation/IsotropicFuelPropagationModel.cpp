/**
 * @file IsotropicFuelPropagationModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../PropagationModel.h"
#include "../FireDomain.h"
#include <math.h>

namespace libforefire {

class IsotropicFuelPropagationModel: public PropagationModel {
	static const string name;
	static int isInitialized;
    /*! properties needed by the model */
	size_t vv_coeff;
	/*! coefficients needed by the model */
	double speed_module;

	double getSpeed(double*);

public:
	IsotropicFuelPropagationModel(const int& = 0, DataBroker* db=0);
	virtual ~IsotropicFuelPropagationModel();
	string getName();
};
PropagationModel* getIsotropicFuelPropagationModelModel(const int& = 0, DataBroker* db=0);

} 
using namespace std;

namespace libforefire {
/* name of the model */
const string IsotropicFuelPropagationModel::name = "IsotropicFuel";
PropagationModel* getIsotropicFuelPropagationModelModel(const int & mindex, DataBroker* db) {
	return new IsotropicFuelPropagationModel(mindex, db);
}
/* registration */
int IsotropicFuelPropagationModel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getIsotropicFuelPropagationModelModel );
/* constructor */
IsotropicFuelPropagationModel::IsotropicFuelPropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	speed_module = params->getDouble("speed_module");
	vv_coeff = registerProperty("fuel.vv_coeff");
	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);

}

IsotropicFuelPropagationModel::~IsotropicFuelPropagationModel() {
}

string IsotropicFuelPropagationModel::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double IsotropicFuelPropagationModel::getSpeed(double* valueOf){

	double lvv_coeff = valueOf[vv_coeff];
 
	double ROS=speed_module*lvv_coeff;
	return ROS;
}
}