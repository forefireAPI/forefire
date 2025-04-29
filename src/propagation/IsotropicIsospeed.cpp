/**
 * @file IsotropicIsospeed.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../PropagationModel.h"
#include "../FireDomain.h"

using namespace std;

namespace libforefire{

class IsotropicIsospeed: public PropagationModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */

	/*! coefficients needed by the model */
	double speed;

	/*! local variables */

	/*! result of the model */
	double getSpeed(double*);

public:
	IsotropicIsospeed(const int& = 0, DataBroker* db=0);
	virtual ~IsotropicIsospeed();

	string getName();

};

PropagationModel* getIsotropicModel(const int& = 0, DataBroker* db=0);

/* name of the model */
const string IsotropicIsospeed::name = "Iso";

/* instantiation */
PropagationModel* getIsotropicModel(const int & mindex, DataBroker* db) {
	return new IsotropicIsospeed(mindex, db);
}

/* registration */
int IsotropicIsospeed::isInitialized =
        FireDomain::registerPropagationModelInstantiator(name, getIsotropicModel );

/* constructor */
IsotropicIsospeed::IsotropicIsospeed(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {

	/* defining the properties needed for the model */

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerPropagationModel(this);

	/* Definition of the coefficients */
	speed = 1.;
	if ( params->isValued("Iso.speed") )
		speed = params->getDouble("Iso.speed");
}

/* destructor (shoudn't be modified) */
IsotropicIsospeed::~IsotropicIsospeed() {
}

/* accessor to the name of the model */
string IsotropicIsospeed::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double IsotropicIsospeed::getSpeed(double* valueOf){
	return speed;
}

}
