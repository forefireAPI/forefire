/*

Copyright (C) 2024 ForeFire Team, SPE, CNRS/Universita di Corsica.

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
