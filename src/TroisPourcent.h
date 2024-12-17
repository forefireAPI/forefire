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

#ifndef TROISPOURCENT_H_
#define TROISPOURCENT_H_

#include "PropagationModel.h"
#include "FireDomain.h"

namespace libforefire {

class TroisPourcent: public libforefire::PropagationModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t effectiveSlope;
	size_t normalWind;

	/*! coefficients needed by the model */
	double R0;
	double windFactor;
	double slopeFactor;

	/*! local variables */

	/*! result of the model */
	double getSpeed(double*);

public:

	TroisPourcent(const int& = 0, DataBroker* db=0);
	virtual ~TroisPourcent();

	string getName();

};

PropagationModel* getTroisPourcentModel(const int& = 0, DataBroker* = 0);

}

#endif /* TROISPOURCENT_H_ */
