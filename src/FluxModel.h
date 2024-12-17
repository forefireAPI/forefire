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

#ifndef FLUXMODEL_H_
#define FLUXMODEL_H_

#include "ForeFireModel.h"

using namespace std;

namespace libforefire {

class FluxModel: public ForeFireModel {

public:

	FluxModel(const int& = 0, DataBroker* = 0);
	virtual ~FluxModel();

	virtual string getName(){return "stub flux model";}

	double getValueAt(FFPoint&, const double&
			, const double&, const double&);
	virtual double getValue(double*, const double&
			, const double&, const double&){return 1.;}

};

FluxModel* getDefaultFluxModel(const int& = 0, DataBroker* = 0);

} /* namespace libforefire */
#endif /* FLUXMODEL_H_ */
