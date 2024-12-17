/*

Copyright (C) 2012 ForeFire Team, SPE, CNRS/Universita di Corsica.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 US

 */
#include "PropagationModel.h"
#include "FireDomain.h"
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