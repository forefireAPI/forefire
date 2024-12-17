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

#ifndef PROPAGATIONMODEL_H_
#define PROPAGATIONMODEL_H_

#include "ForeFireModel.h"
#include "FireNode.h"

using namespace std;

namespace libforefire{

class PropagationModel: public ForeFireModel {

public:

	PropagationModel(const int& = 0, DataBroker* = 0);
	virtual ~PropagationModel();

	virtual string getName(){return "stub propagation model";}

	double getSpeedForNode(FireNode*);
	virtual double getSpeed(double*){return 0.;}

};

PropagationModel* getDefaultPropagationModel(const int& = 0, DataBroker* = 0);

}

#endif /* PROPAGATIONMODEL_H_ */
