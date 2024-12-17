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

class Balbi2015 : public PropagationModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;

	/*! properties needed by the model */
	size_t slope;
	size_t fdepth;
	size_t curvature;
	size_t normalWind;
	size_t moisture;
	size_t temperature;
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
	double cooling;
	double Cpa;

	/*! local variables */

	/*! result of the model */
	double getSpeed(double*);

public:

	Balbi2015(const int& = 0, DataBroker* db=0);
	virtual ~Balbi2015();

	string getName();

};

PropagationModel* getBalbi2015Model(const int& = 0, DataBroker* db=0);
 

/* name of the model */
const string Balbi2015::name = "Balbi2015";

/* instantiation */
PropagationModel* getBalbi2015Model(const int & mindex, DataBroker* db) {
	return new Balbi2015(mindex, db);
}

/* registration */
int Balbi2015::isInitialized =
        FireDomain::registerPropagationModelInstantiator(name, getBalbi2015Model );

/* constructor */
Balbi2015::Balbi2015(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {

	/* defining the properties needed for the model */
	slope = registerProperty("slope");
	fdepth = registerProperty("frontDepth");
	curvature = registerProperty("frontCurvature");
	normalWind = registerProperty("normalWind");
	moisture = registerProperty("moisture");
	temperature = registerProperty("temperature");
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
	if ( params->isValued("Balbi2015.cooling") )
		cooling = params->getDouble("Balbi2015.cooling");
	Cpa = 1004.;
	if ( params->isValued("Balbi2015.Cpa") )
		Cpa = params->getDouble("Balbi2015.Cpa");
}

/* destructor (shoudn't be modified) */
Balbi2015::~Balbi2015() {
}

/* accessor to the name of the model */
string Balbi2015::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */


double Balbi2015::getSpeed(double* valueOf){

	double lRhod = valueOf[Rhod];
	double lRhol = valueOf[Rhol];
	double lMd  = valueOf[Md];

	double lMl  = valueOf[Ml];
	double lsd  = valueOf[sd];
	double lsl  = valueOf[sl];
	double le   = valueOf[e];

	if( le <= 0 ) return 0;

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

	double tau = lTau0/lsd;
	double Betad = lSigmad/(le*lRhod);
	double Betal = lSigmal/(le*lRhol);
	double Sd = lsd*le*Betad;
	double Sl = lsl*le*Betal;
	double nu = min(Sd/lai, 1.);
	double B = 5.6E-8;
	double a = lDeltah/((lCp*(lTi-lTa)));
	double r0 = lsd*lr00;
	double A0 = (lX0*lDeltaH)/(4*lCp*(lTi-lTa));
	double xsi = ((lMl-lMd)*((lSigmal/lSigmad)*(lDeltah/lDeltaH)));
	double A  = (nu*A0/(1+a*lMd))*(1-xsi);
	double T = lTa + (lDeltaH*(1-lX0)*(1-xsi))/((lstoch+1)*Cpa);
	double R00 = (B*T*T*T*T)/(lCp*(lTi-lTa));
	double u00 = (2*lai*(lstoch+1)*T*lRhod)/(lRhoA*lTa*lTau0);
	double u0 = nu*u00;
	double opticalDepth = 4./(Betad*lsd);
	double epsd = 1.-exp(-valueOf[fdepth]/opticalDepth);

	double tanGamma =  valueOf[slope] + valueOf[normalWind]/u0;
	double gamma = atan(tanGamma);
	double R0 = (le /lSigmad)*(R00*epsd)/(1+a*lMd)*(Sd/(Sd+Sl))*(Sd/(Sd+Sl));

	double R = 0;

	if ( gamma > 0 ){
		double curvCor = 1. - valueOf[curvature]/sqrt(1.+valueOf[curvature]*valueOf[curvature]);
		double geomFactor = curvCor*(valueOf[fdepth]/tau)*((1+sin(gamma)-cos(gamma))
					/(1.+valueOf[fdepth]*cos(gamma)/(tau*r0)));
		double Rt = R0 + A*geomFactor;

		R = 0.5*( Rt + sqrt( Rt*Rt + 4.*r0*R0/cos(gamma) ) );
		//cout << "R "<< R << " R0 "<< R0 << " curv "<<valueOf[curvature]<< " curvCor "<<curvCor <<" geomFactor "<< geomFactor <<" depth "<< valueOf[fdepth] <<endl;
	} else {
		R = 0;
	}
	return R + R0;
}

} /* namespace libforefire */
