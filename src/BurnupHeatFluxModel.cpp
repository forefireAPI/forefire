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


#include "FluxModel.h"
#include "FireDomain.h"
#include <math.h>

using namespace std;

namespace libforefire {

class BurnupHeatFluxModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t burnTime;
	size_t fuelLoad;
	size_t moisture;
	size_t dryFuelHeatContent;

	/*! coefficients needed by the model */
	double timeCoeff;
	double waterLatentHeat;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	BurnupHeatFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~BurnupHeatFluxModel();

	string getName();
};

FluxModel* getBurnupHeatFluxModel(const int& = 0, DataBroker* = 0);


/* name of the model */
const string BurnupHeatFluxModel::name = "BurnUpHeatFlux";

/* registration */
int BurnupHeatFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getBurnupHeatFluxModel );

/* instantiation */
FluxModel* getBurnupHeatFluxModel(const int& index, DataBroker* db) {
	return new BurnupHeatFluxModel(index, db);
}

/* constructor */
BurnupHeatFluxModel::BurnupHeatFluxModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	burnTime = registerProperty("fuel.w");
	fuelLoad = registerProperty("fuel.Sigmad");
	dryFuelHeatContent = registerProperty("fuel.DeltaH");
	moisture = registerProperty("moisture");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	timeCoeff = 0.8514;
	if ( params->isValued("burnup.timeCoeff") )
		timeCoeff = params->getDouble("burnup.timeCoeff");
	waterLatentHeat = 2.5e6;
	if ( params->isValued("burnup.waterLatentHeat") )
		waterLatentHeat = params->getDouble("burnup.waterLatentHeat");

}

/* destructor (shoudn't be modified) */
BurnupHeatFluxModel::~BurnupHeatFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string BurnupHeatFluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double BurnupHeatFluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){

	/* see Mandel et al., Geosci. Model Dev., 4, 2011 */

	double Tf = valueOf[burnTime]/timeCoeff;
	double dF = (exp(-(bt-at)/Tf) - exp(-(et-at)/Tf));
	double dM = dF/(et-bt)*valueOf[fuelLoad]/(1.+valueOf[moisture]);
	double sensibleHeatFlux = dM*valueOf[dryFuelHeatContent];
	double latentHeatFlux = dM*(valueOf[moisture]+0.56)*waterLatentHeat;
	return sensibleHeatFlux + latentHeatFlux;

	// TODO instantaneous and averaged fluxes
}

} /* namespace libforefire */
