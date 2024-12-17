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
