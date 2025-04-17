/**
 * @file CurvatureDrivenPropagationModel.cpp
 * @brief A ROS model with only driven by curvature
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
