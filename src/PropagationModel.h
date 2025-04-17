/**
 * @file PropagationModel.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
