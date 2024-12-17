/*

Copyright (C) 2024 ForeFire Team, SPE, CNRS Universita di Corsica.

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

class CurvatureDrivenPropagationModel: public PropagationModel {
    /*! name the model */
	static const string name;
    /*! boolean for initialization */
	static int isInitialized;
    /*! properties needed by the model */
	size_t curvature;
	size_t vv_coeff;
	size_t Kcurv;
	size_t beta;
    /*! coefficients needed by the model */
	double speed_module;

	double getSpeed(double*);

public:
	CurvatureDrivenPropagationModel(const int& = 0, DataBroker* db=0);
	virtual ~CurvatureDrivenPropagationModel();
	string getName();
};
PropagationModel* getCurvatureDrivenPropagationModelModel(const int& = 0, DataBroker* db=0);

} 
using namespace std;

namespace libforefire {
    /* name of the model */
const string CurvatureDrivenPropagationModel::name = "CurvatureDriven";
PropagationModel* getCurvatureDrivenPropagationModelModel(const int & mindex, DataBroker* db) {
	return new CurvatureDrivenPropagationModel(mindex, db);
}
/* registration */
int CurvatureDrivenPropagationModel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getCurvatureDrivenPropagationModelModel );
/* constructor */
CurvatureDrivenPropagationModel::CurvatureDrivenPropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	curvature = registerProperty("frontCurvature");
	vv_coeff = registerProperty("fuel.vv_coeff");
	Kcurv = registerProperty("fuel.Kcurv");
	beta = registerProperty("fuel.beta");
	speed_module = params->getDouble("speed_module");
	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);

}

CurvatureDrivenPropagationModel::~CurvatureDrivenPropagationModel() {
}

string CurvatureDrivenPropagationModel::getName(){
	return name;
}
/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */
double CurvatureDrivenPropagationModel::getSpeed(double* valueOf){
	double lvv_coeff = valueOf[vv_coeff];
	double lKcurv = valueOf[Kcurv];
	double lbeta = valueOf[beta];
	double curv= valueOf[curvature];
	//double ROS=Idealizedvv*lvv_coeff*(1-lKcurv*atan(lbeta*curv));
	double ROS=speed_module*lvv_coeff*(1-lKcurv*curv/sqrt(1+lbeta*pow(curv,2)));

	ROS=max(0.0,ROS);

	return ROS;
}

}
