/**
 * @file RothermelAndrews2018.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "PropagationModel.h"
#include "FireDomain.h"
#include <math.h>
using namespace std;
namespace libforefire {

class RothermelAndrews2018: public PropagationModel {
	
	// ROS model as defined in Table 5 and Table 6 in Andrews, 2018.
    // @book{Andrews_2018, 
    //     title={The Rothermel surface fire spread model and associated developments: A comprehensive explanation}, 
    //     url={http://dx.doi.org/10.2737/RMRS-GTR-371}, 
    //     DOI={10.2737/rmrs-gtr-371}, 
    //     institution={U.S. Department of Agriculture, Forest Service, Rocky Mountain Research Station}, 
    //     author={Andrews, Patricia L.}, 
    //     year={2018}
    //     }

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;
	double windReductionFactor;
	/*! properties needed by the model */
	size_t wv_;
	size_t slope_;
	size_t wo_;
	size_t fd_;
	size_t fpsa_;
	size_t mf_;
	size_t pp_;
	size_t h_;
	size_t st_;
	size_t se_;
	size_t me_;

	/*! result of the model */
	double getSpeed(double*);

public:
	RothermelAndrews2018(const int& = 0, DataBroker* db=0);
	virtual ~RothermelAndrews2018();

	string getName();

};

PropagationModel* getRothermelAndrews2018Model(const int& = 0, DataBroker* db=0);


/* name of the model */
const string RothermelAndrews2018::name = "RothermelAndrews2018";

/* instantiation */
PropagationModel* getRothermelAndrews2018Model(const int & mindex, DataBroker* db) {
	return new RothermelAndrews2018(mindex, db);
}

/* registration */
int RothermelAndrews2018::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getRothermelAndrews2018Model );

/* constructor */
RothermelAndrews2018::RothermelAndrews2018(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	/* defining the properties needed for the model */
	windReductionFactor = params->getDouble("windReductionFactor");

	wv_ = registerProperty("normalWind");
	slope_ = registerProperty("slope");
	wo_ = registerProperty("fuel.fl1h_tac");
	fd_ = registerProperty("fuel.fd_ft");
	fpsa_ = registerProperty("fuel.SAVcar_ftinv");
	mf_ = registerProperty("fuel.mdOnDry1h_r");
	pp_ = registerProperty("fuel.fuelDens_lbft3");
	h_ = registerProperty("fuel.H_BTUlb");
	me_ = registerProperty("fuel.Dme_pc");
	st_ = registerProperty("fuel.totMineral_r");
	se_ = registerProperty("fuel.effectMineral_r");
	
	/* allocating the vector for the values of these properties */
	if ( numProperties > 0 ) properties =  new double[numProperties];

	/* registering the model in the data broker */
	dataBroker->registerPropagationModel(this);

	/* Definition of the coefficients */
}

/* destructor (shoudn't be modified) */
RothermelAndrews2018::~RothermelAndrews2018() {
}

/* accessor to the name of the model */
string RothermelAndrews2018::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double RothermelAndrews2018::getSpeed(double* valueOf){
	// Constants
	double msToftmin = 196.85039;
	double ftminToms = 0.00508;
	double lbft2Tokgm2 = 4.88243;
	double tacTokgm2 = 0.224;
	double pcTor = 0.01;

	// Fuel properties
	double tan_slope = 0;
	if (valueOf[slope_]>0){ // Only defined for null or positive slope
		tan_slope = valueOf[slope_];
	}
	double wv = msToftmin * valueOf[wv_]; // convert m/s to feat/min
	double me = valueOf[me_] * pcTor; // convert percentage to ratio
	double pp = valueOf[pp_];
	double mf = valueOf[mf_];
	double fpsa = valueOf[fpsa_];
	double fd = valueOf[fd_];
	double wo = valueOf[wo_] * tacTokgm2 / lbft2Tokgm2; // convert US short tons per acre to pounds per square foot
	double h = valueOf[h_];
	double st = valueOf[st_];
	double se = valueOf[se_];

	if (wv < 0) wv = 0;

	if(wo > 0){
		double Beta_op = 3.348 * pow(fpsa, -0.8189);  // Optimum packing ratio
		double ODBD = wo / fd; // Ovendry bulk density
        double Beta = ODBD / pp; // Packing ratio
		double WN = wo / (1 + st); // Net fuel loading
        double A =  133.0 / pow(fpsa, 0.7913); // updated A
        double T_max = pow(fpsa,1.5) * pow(495.0 + 0.0594 * pow(fpsa, 1.5),-1.0); // Maximum reaction velocity
		double T = T_max * pow((Beta / Beta_op), A) * exp(A * (1 - Beta / Beta_op));  // Optimum reaction velocity
        double NM = 1. - 2.59 * (mf / me) + 5.11 * pow(mf / me, 2.) - 3.52 * pow(mf / me,3.);  // Moisture damping coeff.
        double NS = 0.174 * pow(se, -0.19);  // Mineral damping coefficient
        double RI = T * WN * h * NM * NS;
        double PFR = pow(192.0 + 0.2595 * fpsa, -1) * exp((0.792 + 0.681 * pow(fpsa, 0.5)) * (Beta + 0.1));  // Propogating flux ratio
        // Wind Coefficien t
        double B = 0.02526 * pow(fpsa, 0.54);
        double C = 7.47 * exp(-0.1333 * pow(fpsa, 0.55));
        double E = 0.715 * exp(-3.59 * pow(10, -4) * fpsa);
        if (wv > (0.9 * RI)) wv = 0.9 * RI;
		double WC = (C * pow(wv, B)) * pow((Beta / Beta_op), (-E));
		double SC = 5.275*(pow(Beta, -0.3))*pow(tan_slope, 2);
        // Heat sink
        double EHN = exp(-138. / fpsa);  // Effective Heating Number = f(surface are volume ratio)
        double QIG = 250. + 1116. * mf;  // Heat of preignition= f(moisture content)
        // rate of spread (ft per minute) // RI = BTU/ft^2
        double numerator = (RI * PFR * (1 + WC + SC));
        double denominator = (ODBD * EHN * QIG);
        double R = numerator / denominator; // WC and SC will be zero at slope = wind = 0

		if(R <= 0.0) {
			return 0;
		}else{
			return R * ftminToms;
		}
	}else{
		return 0;
	}
} /* namespace libforefire */
}