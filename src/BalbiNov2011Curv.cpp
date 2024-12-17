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

namespace libforefire {

class BalbiNov2011Curv : public PropagationModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t slope;
	size_t normalWind;
	size_t curvature;
	size_t Rhod;
	size_t Rhol;
	size_t Md;
	size_t Ml;
	size_t sd;
	size_t sl;
	size_t e;
	size_t Sigmad;
	size_t Sigmal;
	size_t stoch;
	size_t RhoA;
	size_t Ta;
	size_t Tau0;
	size_t Deltah;
	size_t DeltaH;
	size_t Cp;
	size_t Ti;
	size_t X0;
	size_t r00;
	size_t Blai;

	/*! coefficients needed by the model */
	double Cpa;
	double cooling;
	double gravity;

	/*! local variables */

	/*! result of the model */
	double getSpeed(double*);

public:

	BalbiNov2011Curv(const int& = 0, DataBroker* db=0);
	virtual ~BalbiNov2011Curv();

	string getName();

};

PropagationModel* getBalbiNov2011CurvModel(const int& = 0, DataBroker* db=0);


/* name of the model */
const string BalbiNov2011Curv::name = "BalbiNov2011Curv";

/* instantiation */
PropagationModel* getBalbiNov2011CurvModel(const int & mindex, DataBroker* db) {
	return new BalbiNov2011Curv(mindex, db);
}

/* registration */
int BalbiNov2011Curv::isInitialized =
        FireDomain::registerPropagationModelInstantiator(name, getBalbiNov2011CurvModel );

/* constructor */
BalbiNov2011Curv::BalbiNov2011Curv(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {

	/* defining the properties needed for the model */
	slope = registerProperty("slope");
	normalWind = registerProperty("normalWind");
	curvature = registerProperty("frontCurvature");
	Rhod = registerProperty("fuel.Rhod");
	Rhol = registerProperty("fuel.Rhol");
	Md = registerProperty("fuel.Md");
	Ml = registerProperty("fuel.Ml");
	sd = registerProperty("fuel.sd");
	sl = registerProperty("fuel.sl");
	e = registerProperty("fuel.e");
	Sigmad = registerProperty("fuel.Sigmad");
	Sigmal = registerProperty("fuel.Sigmal");
	stoch = registerProperty("fuel.stoch");
	RhoA = registerProperty("fuel.RhoA");
	Ta = registerProperty("fuel.Ta");
	Tau0 = registerProperty("fuel.Tau0");
	Deltah = registerProperty("fuel.Deltah");
	DeltaH = registerProperty("fuel.DeltaH");
	Cp = registerProperty("fuel.Cp");
	Ti = registerProperty("fuel.Ti");
	X0 = registerProperty("fuel.X0");
	r00 = registerProperty("fuel.r00");
	Blai = registerProperty("fuel.Blai");

	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerPropagationModel(this);

	/* Definition of the coefficients */
	cooling = 0.;
	if ( params->isValued("BalbiNov2011Curv.cooling") )
		cooling = params->getDouble("BalbiNov2011.cooling");
	Cpa = 1004.;
	if ( params->isValued("BalbiNov2011Curv.Cpa") )
		Cpa = params->getDouble("BalbiNov2011Curv.Cpa");
	gravity = 9.81;
	if ( params->isValued("BalbiNov2011Curv.gravity") )
		gravity = params->getDouble("BalbiNov2011Curv.gravity");
}

/* destructor (shoudn't be modified) */
BalbiNov2011Curv::~BalbiNov2011Curv() {
}

/* accessor to the name of the model */
string BalbiNov2011Curv::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double BalbiNov2011Curv::getSpeed(double* valueOf){

	double lRhod = valueOf[Rhod];
	double lRhol = valueOf[Rhol];
	double lMd  = valueOf[Md];
	double lMl  = valueOf[Ml];
	double lsd  = valueOf[sd];
	double lsl  = valueOf[sl];
	double le   = valueOf[e];
	double lSigmad = valueOf[Sigmad];
	double lSigmal = valueOf[Sigmal];
	double lstoch = valueOf[stoch];
	double lRhoA  = valueOf[RhoA];
	double lTa  = valueOf[Ta];
	double lTau0  = valueOf[Tau0];
	double lDeltah   = valueOf[Deltah];
	double lDeltaH = valueOf[DeltaH];
	double lCp = valueOf[Cp];
	double lTi  = valueOf[Ti];
	double lX0  = valueOf[X0];
	double lr00  = valueOf[r00];
	double lai = valueOf[Blai];

	if(le <= 0 ) return 0;

	double Betad =   lSigmad /(le* lRhod);
	double Betal =   lSigmal /(le* lRhol);
	double Sd = lsd * le * Betad;
	double Sl = lsl * le * Betal;
	double nu = min((Sd) / lai, 1.);
	double normal_wind = valueOf[normalWind];
	double B = 5.6E-8;
	double a   = lDeltah/ ((lCp*(lTi-lTa)));
	double r0 = lsd * lr00;
	double A0 = (lX0*lDeltaH)/(4*lCp*(lTi-lTa));
	double xsi = ((lMl-lMd)*((lSigmal/lSigmad)*(lDeltah/lDeltaH)));
	double A  = (nu*A0 / (1 + a * lMd)) * (1-xsi);
	double T = lTa + ( lDeltaH*(1-lX0)*(1-xsi) )     / ((lstoch+1)*Cpa);
	double R00 = (B*T*T*T*T)   / (lCp*(lTi-lTa));
	double R = 0;
	double R0 =0;
	double u00 = (2*lai*(lstoch+1)*T*lRhod)/(lRhoA*lTa*lTau0);
	double u0 = nu * u00;

	double tanGamma =  valueOf[slope] + (normal_wind/u0);
	double gamma = atan(tanGamma);

	R0 = (le / lSigmad)   * (R00) / (1 + a * lMd) * Sd/(Sd+Sl) * Sd/(Sd+Sl);

	if ( gamma > 0. ) {
		double curvAdim = valueOf[curvature]*sin(gamma)*u00*u00
				*(1.+valueOf[slope]*valueOf[slope])/(gravity*(T/lTa-1.));
		double curvCor = 1. - curvAdim;

		double geomFactor = curvCor*r0*((1+sin(gamma)-cos(gamma))/(1.+cos(gamma)));
		double Rt = R0 + A*geomFactor;
		R = 0.5*( Rt + sqrt( Rt*Rt + 4.*r0*R0/cos(gamma) ) );
	} else {
		R = R0;
	}

	return R ;

}

}
