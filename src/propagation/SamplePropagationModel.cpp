/**
 * @file SamplePropagationModel.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../PropagationModel.h"
#include "../FireDomain.h"
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
