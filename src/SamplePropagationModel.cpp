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

class SamplePropagationModel: public PropagationModel {
	static const string name;
	static int isInitialized;
	size_t slope;
	size_t normalWind;
	size_t Sigmad;
	double windReductionFactor;

	double getSpeed(double*);

public:
	SamplePropagationModel(const int& = 0, DataBroker* db=0);
	virtual ~SamplePropagationModel();
	string getName();
};
PropagationModel* getSamplePropagationModelModel(const int& = 0, DataBroker* db=0);

} 
using namespace std;

namespace libforefire {
const string SamplePropagationModel::name = "SamplePropagationModel";
PropagationModel* getSamplePropagationModelModel(const int & mindex, DataBroker* db) {
	return new SamplePropagationModel(mindex, db);
}

int SamplePropagationModel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getSamplePropagationModelModel );

SamplePropagationModel::SamplePropagationModel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	windReductionFactor = params->getDouble("windReductionFactor");
	slope = registerProperty("slope");
	normalWind = registerProperty("normalWind");
	Sigmad = registerProperty("fuel.Sigmad");
	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);

}

SamplePropagationModel::~SamplePropagationModel() {
}

string SamplePropagationModel::getName(){
	return name;
}

double SamplePropagationModel::getSpeed(double* valueOf){
	double lSigmad = valueOf[Sigmad] ;
	double normal_wind  = valueOf[normalWind] ;
	double localngle =  valueOf[slope];
	normal_wind *= windReductionFactor*localngle;
	if (normal_wind < 0) normal_wind = 0;
	return normal_wind*lSigmad;
}

}
