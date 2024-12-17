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
