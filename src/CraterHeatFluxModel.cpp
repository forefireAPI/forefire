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
#include "include/Futils.h"
using namespace std;
namespace libforefire {

class CraterHeatFluxModel: public libforefire::FluxModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t windU;
	size_t windV;

	/*! coefficients needed by the model */
	double eruptionTime;
	double crustTemperature;
	double lavaTemperature;
	vector<double> refHours;
	vector<double> crustFractions;
	vector<double> windValues;
	vector<double> A;
	vector<double> B;
	vector<double> heatCorrections;

	/*! local variables */

	/*! result of the model */
	double getValue(double*, const double&
			, const double&, const double&);

public:

	CraterHeatFluxModel(const int& = 0, DataBroker* = 0);
	virtual ~CraterHeatFluxModel();

	string getName();
};

FluxModel* getCraterHeatFluxModel(const int& = 0, DataBroker* = 0);

/* name of the model */
const string CraterHeatFluxModel::name = "CraterHeatFluxModel";

/* instantiation */
FluxModel* getCraterHeatFluxModel(const int& index, DataBroker* db) {
	return new CraterHeatFluxModel(index, db);
}

/* registration */
int CraterHeatFluxModel::isInitialized =
        FireDomain::registerFluxModelInstantiator(name, getCraterHeatFluxModel );

/* constructor */
CraterHeatFluxModel::CraterHeatFluxModel(
		const int & mindex, DataBroker* db)
	: FluxModel(mindex, db) {

	/* defining the properties needed for the model */
	windU = registerProperty("windU");
	windV = registerProperty("windV");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerFluxModel(this);

	/* Definition of the coefficients */
	eruptionTime = 0.;
	if ( params->isValued("lava.eruptionTime") )
		eruptionTime = params->getDouble("lava.eruptionTime");
	if ( !params->isValued("lava.hours") )
		cout<<"ERROR: vector of parameters lava..hours should be valued"<<endl;
	refHours = params->getDoubleArray("lava.hours");
	if ( !params->isValued("lava.crustFractions") )
		cout<<"ERROR: vector of parameters lava.crustFractions should be valued"<<endl;
	crustFractions = params->getDoubleArray("lava.crustFractions");
	if ( !params->isValued("lava.windTresholds") )
		cout<<"ERROR: vector of parameters lava.windTresholds should be valued"<<endl;
	windValues = params->getDoubleArray("lava.windTresholds");
	if ( !params->isValued("lava.A") )
		cout<<"ERROR: vector of parameters lava.A should be valued"<<endl;
	A = params->getDoubleArray("lava.A");
	if ( !params->isValued("lava.B") )
		cout<<"ERROR: vector of parameters lava.B should be valued"<<endl;
	B = params->getDoubleArray("lava.B");
	crustTemperature = 400.;
	if ( params->isValued("lava.crustTemperature") )
		crustTemperature = params->getDouble("lava.crustTemperature");
	lavaTemperature = 800.;
	if ( params->isValued("lava.temperature") )
		lavaTemperature = params->getDouble("lava.temperature");
	if ( !params->isValued("crater.heatCorrections") )
		cout<<"ERROR: vector of parameters crater.heatCorrections should be valued"<<endl;
	heatCorrections = params->getDoubleArray("crater.heatCorrections");

	/* local variables */
}

/* destructor (shoudn't be modified) */
CraterHeatFluxModel::~CraterHeatFluxModel() {
	if ( properties != 0 ) delete properties;
}

/* accessor to the name of the model */
string CraterHeatFluxModel::getName(){
	return name;
}

/* ****************** */
/* Model for the flux */
/* ****************** */

double CraterHeatFluxModel::getValue(double* valueOf
		, const double& bt, const double& et, const double& at){

	if ( bt - eruptionTime < 0 ) return 0.;

	/* getting the hours since eruption */
	double hoursSinceEruption = (bt-eruptionTime)/3600.;
	if ( hoursSinceEruption > refHours.back() ) return 0.;

	/* getting the fraction of crust */
	size_t hind = 0;
	size_t nhours = refHours.size();
	while ( hind+1 < nhours and refHours[hind+1] < hoursSinceEruption ) hind++;
	double beta = (hoursSinceEruption-refHours[hind])
					/(refHours[hind+1]-refHours[hind]);
	double crustFraction = beta*crustFractions[hind+1]
	        + (1.-beta)*crustFractions[hind];
	double heatCorrection = beta*heatCorrections[hind+1]
	        + (1.-beta)*heatCorrections[hind];

	/* getting the mean temperature */
	double mean_temp = crustFraction*crustTemperature
			+ (1.-crustFraction)*lavaTemperature;

	/* getting the wind module */
	double windModule = sqrt(valueOf[windU]*valueOf[windU]+valueOf[windV]*valueOf[windV]);

	/* if the wind module is larger than 10 */
	if ( windModule > 10. ) return A[4]*mean_temp - B[4];
	/* else interpolating between two known values */
	/* getting the index for the coefficients */
	size_t wind = 0;
	size_t nwind = windValues.size();
	while ( wind+1 < nwind and windValues[wind+1] < windModule ) wind++;

	/* computing the values for the interval boundaries */
	double leftval = A[wind]*mean_temp - B[wind];
	double rightval = A[wind+1]*mean_temp - B[wind+1];

	/* linear interpolation between the boundaries */
	beta = (windModule-windValues[wind])/(windValues[wind+1]-windValues[wind]);
//	return heatCorrection*(beta*rightval + (1.-beta)*leftval);
	double coef = 0.25; // TODO fluxes are divided by 4 arbitrarily
	return coef*heatCorrection*(beta*rightval + (1.-beta)*leftval);
}

} /* namespace libforefire */
