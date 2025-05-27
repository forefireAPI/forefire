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



#include "../FluxModel.h"
#include "../FireDomain.h"

using namespace std;

namespace libforefire {

class SpottingFluxBasicModel: public FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t spot0;
	size_t sigmad;

	/*! coefficients needed by the model */
	double spottingDuration;
	double nominalSpottingFlux;
	double startSpotting;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:
	SpottingFluxBasicModel(const int& = 0, DataBroker* = 0);
	virtual ~SpottingFluxBasicModel();

	string getName();
};

FluxModel* getSpottingFluxBasicModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string SpottingFluxBasicModel::name = "SpottingFluxBasic";

/* instantiation */
FluxModel* getSpottingFluxBasicModel(const int& index, DataBroker* db) {
	return new SpottingFluxBasicModel(index, db);
}

/* registration */
int SpottingFluxBasicModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getSpottingFluxBasicModel );

/* constructor */
SpottingFluxBasicModel::SpottingFluxBasicModel(
		const int & mindex, DataBroker* db) : FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	spot0 = registerProperty("fuel.Spot");
	sigmad = registerProperty("fuel.Sigmad");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);


	/* Definition of the coefficients */
	spottingDuration = 100.0;
	if ( params->isValued("spottingDuration") )
		spottingDuration = params->getDouble("spottingDuration");
	nominalSpottingFlux = 1.0;
	if ( params->isValued("nominalSpottingFlux") )
		nominalSpottingFlux = params->getDouble("nominalSpottingFlux");
	startSpotting = 0.0;
	if ( params->isValued("startSpotting") )
		startSpotting = params->getDouble("startSpotting");
}

/* destructor (shoudn't be modified) */
SpottingFluxBasicModel::~SpottingFluxBasicModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string SpottingFluxBasicModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double SpottingFluxBasicModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){
	/* Mean heat flux released between the time interval [bt, et] */
	/* The heat flux is supposed to be constant from the arrival time (at)
	 * and for a period of time of 'burningDuration', constant of the model */
	//if (valueOf[sigmad] > 0){
	//	return nominalSpottingFlux * valueOf[spot0];
	//}
	//else {
	//	return 0;
	//}
	//cout<<"AT "<<at<<endl;
	//cout<<"BT "<<bt<<endl;
	//cout<<"StartSpotting "<<startSpotting<<endl;
	//cout<<"spotDuration "<<spottingDuration<<endl;
	//cout<<"nominalSpottingFlux "<<nominalSpottingFlux<<endl;
	
	if ( startSpotting >= bt ){
		//cout<<"ok start before at"<<endl;
		return 0.;
	}
	else {
		/* Instantaneous flux */
		/* ------------------ */
		if (bt <= startSpotting + spottingDuration) {
			//cout<<"nominalSpottingFlux "<<nominalSpottingFlux<<endl;
			return nominalSpottingFlux; //* valueOf[spot0] ;
		}
		else {
			return 0.;
		}

	}
}

} /* namespace libforefire */
