/**
 * @file Rothermel.cpp
 * @brief Rothermel propagation model
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "../PropagationModel.h"
#include "../FireDomain.h"
#include <math.h>
using namespace std;
namespace libforefire {

class Rothermel: public PropagationModel {

	/*! name the model */
	static const string name;

	/*! boolean for initialization */
	static int isInitialized;
	double windReductionFactor;
	/*! properties needed by the model */
	size_t slope;
	size_t normalWind;
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
	size_t Cpa;
	size_t Ti;
	size_t X0;
	size_t r00;
	size_t Blai;
	size_t Me;
	
	/*! coefficients needed by the model */

	/*! local variables */
	std::ofstream csvfile;
	/*! result of the model */
	double getSpeed(double*);

public:
	Rothermel(const int& = 0, DataBroker* db=0);
	virtual ~Rothermel();

	string getName();

};

PropagationModel* getRothermelModel(const int& = 0, DataBroker* db=0);


/* name of the model */
const string Rothermel::name = "Rothermel";

/* instantiation */
PropagationModel* getRothermelModel(const int & mindex, DataBroker* db) {
	return new Rothermel(mindex, db);
}

/* registration */
int Rothermel::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getRothermelModel );

/* constructor */
Rothermel::Rothermel(const int & mindex, DataBroker* db)
: PropagationModel(mindex, db) {
	/* defining the properties needed for the model */
	windReductionFactor = params->getDouble("windReductionFactor");

	slope = registerProperty("slope");
	normalWind = registerProperty("normalWind");
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
	Cpa = registerProperty("fuel.Cpa");
	Ti = registerProperty("fuel.Ti");
	X0 = registerProperty("fuel.X0");
	r00 = registerProperty("fuel.r00");
	Blai = registerProperty("fuel.Blai");
	Me = registerProperty("fuel.me");

	if ( numProperties > 0 ) properties =  new double[numProperties];
	dataBroker->registerPropagationModel(this);
	/* registering the model in the data broker */
    if (params->isValued("RothermelLoggerCSVPath")) {
        csvfile.open(params->getParameter("RothermelLoggerCSVPath"));
		std::cout<< "logging Rothermel data in "<<params->getParameter("RothermelLoggerCSVPath")<<std::endl;
    }
    
    if (csvfile.is_open()) {
     
        csvfile << "ROS";
        std::cout << "Props: ";
        for (const auto& inputName : wantedProperties) {
            csvfile << ";" << inputName;
            std::cout << inputName << " ";
        }
        std::cout << std::endl;

        
        csvfile << std::endl;

       
      
    } 
	/* Definition of the coefficients */
}

/* destructor (shoudn't be modified) */
Rothermel::~Rothermel() {
}

/* accessor to the name of the model */
string Rothermel::getName(){
	return name;
}

/* *********************************************** */
/* Model for the propagation velovity of the front */
/* *********************************************** */

double Rothermel::getSpeed(double* valueOf){

	double lRhod = valueOf[Rhod] * 0.06; // conversion kg/m^3 -> lb/ft^3
	double lMd  = valueOf[Md];
	double lsd  = valueOf[sd] / 3.2808399; // conversion 1/m -> 1/ft
	double le   = valueOf[e] * 3.2808399; // conversion m -> ft
	
	if (le==0) return 0;
	double lSigmad = valueOf[Sigmad] * 0.2048; // conversion kg/m^2 -> lb/ft^2
	double lDeltaH = valueOf[DeltaH] / 2326.0;// conversion J/kg -> BTU/lb
	double normal_wind  = valueOf[normalWind] * 196.850394 ; //conversion m/s -> ft/min
	double localngle =  valueOf[slope];
	//	if (normal_wind  > 0) cout<<"wind is"<<valueOf[normalWind]<<endl;

/*	lRhod = 30.0;
	 lMd =0.1;
	  lsd =1523.99999768352;
	  le =1.0;
	  lSigmad =1044.27171; */

	normal_wind *= windReductionFactor; // factor in the data seen in 2013

	if (normal_wind < 0) normal_wind = 0;


	double tanangle = localngle;
	if (tanangle<0) tanangle=0;


	double Mchi = valueOf[Me]; // Moisture of extinction

	double Etas = 1; // no mineral damping

	double Wn = lSigmad;

	double Mratio = lMd / Mchi;

	double Etam = 1  + Mratio * (-2.59 + Mratio * (5.11 - 3.52 * Mratio));

	double A = 1 / (4.774 * pow(lsd, 0.1) - 7.27);
        
	double lRhobulk = Wn / le; // Dead bulk density = Dead fuel load / Fuel height
        
	double Beta = lRhobulk / lRhod;  // Packing ratio = Bulk density / Particle density

	double Betaop = 3.348 * pow(lsd, -0.8189);

	double RprimeMax = pow(lsd, 1.5) * (1 / (495 + 0.0594 * pow(lsd, 1.5)));

	double Rprime = RprimeMax * pow((Beta/Betaop), A) *  exp(A*(1-(Beta/Betaop))) ;

	double chi = pow(192 + 0.259*lsd, -1) * exp((0.792 + 0.681*pow(lsd, 0.5)) * (Beta+0.1));

	double epsilon  = exp(-138 / lsd);

	double Qig = 250 + 1116 * lMd; // "1,116 in Rothermel" != 1.116

	double C = 7.47 * exp(-0.133*pow(lsd, 0.55));

	double B = 0.02526*pow(lsd, 0.54);

	double E = 0.715* exp(-3.59*(10E-4 * lsd));

	double Ir = Rprime*Wn*lDeltaH*Etam*Etas;
	double Uf = 0.9*Ir;
	
	// wind limit 2013 10.1071/WF12122 andrews/cruz/rothermel
	if(windReductionFactor < 1.0){
		Uf = 96.81*pow(Ir, 1./3);
	}
	

	if (normal_wind>Uf) {
		normal_wind = Uf;
	}

	double phiV = C * pow((Beta/Betaop), -E) * pow(normal_wind,B) ;

	double phiP = 5.275 * pow(Beta, -0.3) * pow(tanangle, 2);

	double R0 = (Ir * chi) / (lRhobulk * epsilon * Qig);

	double R = R0 * (1 + phiV + phiP);

	if(R < R0)  R = R0;

	if(R > 0.0) {
		R=  R * 0.00508 ; // ft/min -> m/s
	}else{
		R=0;
	}

   


    if (csvfile.is_open()) {
        csvfile << R ;   
        for (size_t i = 0; i < wantedProperties.size(); ++i) {
            csvfile << ";" << valueOf[i] ;
        }
        csvfile << std::endl;
    }

	return R;
}

} /* namespace libforefire */
