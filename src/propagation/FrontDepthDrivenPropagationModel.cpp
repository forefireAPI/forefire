/**
 * @file FrontDepthDrivenPropagationModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../PropagationModel.h"
#include "../FireDomain.h"
#include <math.h>

namespace libforefire {

class FrontDepthDrivenPropagationModel: public PropagationModel {
	static const string name;
	static int isInitialized;
	size_t vv_coeff;
	size_t Kdepth;
    size_t fdepth;
    size_t normalWind;
	double windReductionFactor;
	double speed_module;
	double getSpeed(double*);

public:
	FrontDepthDrivenPropagationModel(const int& = 0, DataBroker* db=0);
	virtual ~FrontDepthDrivenPropagationModel();
	string getName();
};
PropagationModel* getFrontDepthDrivenPropagationModelModel(const int& = 0, DataBroker* db=0);

} 
using namespace std;

namespace libforefire {
const string FrontDepthDrivenPropagationModel::name = "FrontDepthDriven";
PropagationModel* getFrontDepthDrivenPropagationModelModel(const int & mindex, DataBroker* db) {
	return new FrontDepthDrivenPropagationModel(mindex, db);
}

int FrontDepthDrivenPropagationModel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getFrontDepthDrivenPropagationModelModel );

FrontDepthDrivenPropagationModel::FrontDepthDrivenPropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	fdepth = registerProperty("frontDepth");
    normalWind = registerProperty("normalWind");
	speed_module = params->getDouble("speed_module");
	vv_coeff = registerProperty("fuel.vv_coeff");
	Kdepth = registerProperty("fuel.Kdepth");
    windReductionFactor = params->getDouble("windReductionFactor");
	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);

}

FrontDepthDrivenPropagationModel::~FrontDepthDrivenPropagationModel() {
}

string FrontDepthDrivenPropagationModel::getName(){
	return name;
}

double FrontDepthDrivenPropagationModel::getSpeed(double* valueOf){
	double lvv_coeff = valueOf[vv_coeff] ;
	double lfdepth  = valueOf[fdepth] ;
	double lKdepth  = valueOf[Kdepth] ;

    //double ROS=windReductionFactor*valueOf[normalWind]*speed_module*lvv_coeff*(1+lKdepth*lfdepth/(1+lfdepth));
    double ROS= windReductionFactor*valueOf[normalWind]*speed_module*lvv_coeff*(1+lKdepth*lfdepth);
	//if (lfdepth >20){
	//cout <<"lfdepth : "<< lfdepth << "   ROS : "<<ROS<<endl;
	//}
	
	return ROS;
}

}
