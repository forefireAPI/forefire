/**
 * @file FluxModel.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
