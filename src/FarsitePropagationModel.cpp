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

this File is made to replicate the Farsite model in the ForeFire framework

*/

#include "PropagationModel.h"
#include "FireDomain.h"
#include <cstring>
#include <cmath>

using namespace std;

namespace libforefire
{

	class Farsite : public PropagationModel
	{

		/*! name the model */
		static const string name;

		/*! boolean for initialization */
		static int isInitialized;


		size_t idx_slope;
		size_t idx_wind;

		size_t idx_m_ones;
		size_t idx_m_liveh;
		size_t idx_m_tens;
		size_t idx_m_livew;
		size_t idx_m_hundreds;

		/*! fuel properties needed by the model */
	//	size_t idx_code;
		size_t idx_h1;
		size_t idx_h10;
		size_t idx_h100;
		size_t idx_lh;
		size_t idx_lw;
		size_t idx_dynamic;
		size_t idx_sav1;
		size_t idx_savlh;
		size_t idx_savlw;
		size_t idx_depth;
		size_t idx_xmext;
		size_t idx_heatd;
		size_t idx_heatl;

		/*! local variables */
		std::ofstream csvfile;
		/*! result of the model */
		double getSpeed(double *);

	public:
		Farsite(const int & = 0, DataBroker *db = 0);
		virtual ~Farsite();

		string getName();
	};

	PropagationModel *getFarsiteModel(const int & = 0, DataBroker *db = 0);

	/* name of the model */
	const string Farsite::name = "Farsite";

	/* instantiation */
	PropagationModel *getFarsiteModel(const int &mindex, DataBroker *db)
	{
		return new Farsite(mindex, db);
	}

	/* registration */
	int Farsite::isInitialized =
		FireDomain::registerPropagationModelInstantiator(name, getFarsiteModel);

	/* constructor */
	Farsite::Farsite(const int &mindex, DataBroker *db)
		: PropagationModel(mindex, db)
	{
		
		/* defining the properties needed for the model */
		idx_slope = registerProperty("slope");
		idx_wind = registerProperty("normalWind");

		idx_m_ones = registerProperty("moist.ones");
		idx_m_liveh = registerProperty("moist.liveh");
		idx_m_tens = registerProperty("moist.tens");
		idx_m_livew = registerProperty("moist.livew");
		idx_m_hundreds = registerProperty("moist.hundreds");
		
		//idx_code= registerProperty("fuel.code");
		idx_h1 = registerProperty("fuel.h1");
		idx_h10 = registerProperty("fuel.h10");
		idx_h100 = registerProperty("fuel.h100");
		idx_lh = registerProperty("fuel.lh");
		idx_lw = registerProperty("fuel.lw");
		idx_dynamic = registerProperty("fuel.dynamic");
		idx_sav1 = registerProperty("fuel.sav1");
		idx_savlh = registerProperty("fuel.savlh");
		idx_savlw = registerProperty("fuel.savlw");
		idx_depth = registerProperty("fuel.depth");
		idx_xmext = registerProperty("fuel.xmext");
		idx_heatd = registerProperty("fuel.heatd");
		idx_heatl = registerProperty("fuel.heatl");

		/* allocating the vector for the values of these properties */
		if (numProperties > 0)
			properties = new double[numProperties];

		/* registering the model in the data broker */
		dataBroker->registerPropagationModel(this);
		double adjustementWind = 1.0;
		if (params->isValued("Farsite.adjustementWind"))
			adjustementWind = params->getDouble("Farsite.adjustementWind");


		if (params->isValued("FarsiteLoggerCSVPath")) {
			csvfile.open(params->getParameter("FarsiteLoggerCSVPath"));
			std::cout<< "logging Farsite data in "<<params->getParameter("FarsiteLoggerCSVPath")<<std::endl;
		}
		
		if (csvfile.is_open()) {
			
			csvfile << "ROS;rateo;phiew";
			std::cout << "Props: ";
			for (const auto& inputName : wantedProperties) {
				csvfile << ";" << inputName;
				std::cout << inputName << " ";
			}
			std::cout << std::endl;
	
			
			csvfile << std::endl;
	
			
			
		} 




	}

	/* destructor (shoudn't be modified) */
	Farsite::~Farsite()
	{
	}

	/* accessor to the name of the model */
	string Farsite::getName()
	{
		return name;
	}

	/* *********************************************** */
	/* Model for the propagation velovity of the front */
	/* *********************************************** */



	double Farsite::getSpeed(double *valueOf)
	{
		//-----------------------------------------------------------------------------
		// 1) Define indices to read from valueOf input vector
		//   
		//-----------------------------------------------------------------------------


		// If you need "dynamic" as a boolean, you can either:
		//   - interpret an integer or double from valueOf, or
		//   - define a fixed bool, etc.
		// For example:
		// size_t idxNewfuelDynamic = 15;
		// bool fuel_dynamic = (valueOf[idxNewfuelDynamic] != 0.0);

		//-----------------------------------------------------------------------------
		// 2) Read actual values from valueOf
		//-----------------------------------------------------------------------------
		double windspd = valueOf[idx_wind]; // wind speed in miles per hout
		double slope = valueOf[idx_slope]; // degrees with 0 as flat
		// moinstures variables
		double m_ones = valueOf[idx_m_ones];  
		double m_liveh = valueOf[idx_m_liveh]; 
		double m_tens = valueOf[idx_m_tens];  
		double m_livew = valueOf[idx_m_livew];  
		double m_hundreds = valueOf[idx_m_hundreds]; 

		// Fuel variables
		double fuel_h1 = valueOf[idx_h1];
		double fuel_h10 = valueOf[idx_h10];
		double fuel_h100 = valueOf[idx_h100];
		double fuel_lh = valueOf[idx_lh];
		double fuel_lw = valueOf[idx_lw];
		double fuel_xmext = valueOf[idx_xmext];
		double fuel_depth = valueOf[idx_depth];
		double fuel_sav1 = valueOf[idx_sav1];
		double fuel_savlh = valueOf[idx_savlh];
		double fuel_savlw = valueOf[idx_savlw];
		double fuel_heatd = valueOf[idx_heatd];
		double fuel_heatl = valueOf[idx_heatl];

		// Optional dynamic flag if desired
		bool fuel_dynamic = false;
		// e.g. if you want it from the array:
		fuel_dynamic = (valueOf[idx_dynamic] != 0.0);

	
		double mois[4][2]=		// fraction of oven-dry weight
		{	{m_ones, m_liveh},        
			{m_tens, m_livew},
			{m_hundreds, 0.0},
			{m_ones, 0.0}
		};

		double rateo = 0.0;
		double slopespd = 0.0;
		double react = 0.0;
		double savx = 0.0;

		long ndead = 0, nlive = 0;

		// Count how many of these are nonzero to mimic "if (newfuel.h1) ndead++;"
		if (fuel_h1 > 0.0)
			ndead++;
		if (fuel_h10 > 0.0)
			ndead++;
		if (fuel_h100 > 0.0)
			ndead++;
		if (fuel_lh > 0.0)
			nlive++;
		if (fuel_lw > 0.0)
			nlive++;

		if (nlive == 0)
			fuel_dynamic = false;

		if (nlive > 0)
			nlive = 2; // boost to max
		if (ndead > 0)
			ndead = 4;

		double nclas[2] = {static_cast<double>(ndead), static_cast<double>(nlive)};
		double xmext[2] = {fuel_xmext, 0.0};

		// Load array
		double load[4][2] = {
			{fuel_h1, fuel_lh},
			{fuel_h10, fuel_lw},
			{fuel_h100, 0.0},
			{0.0, 0.0}};

		// Possibly do dynamic load transfer if fuel_dynamic is true:
		if (fuel_dynamic)
		{
			// Example: "if(mois[0][1]<0.30) { ... }" etc.
			if (mois[0][1] < 0.30)
			{
				load[3][0] = load[0][1];
				load[0][1] = 0.0;
			}
			else if (mois[0][1] < 1.20)
			{
				load[3][0] = load[0][1] * (1.20 - mois[0][1]) / 0.9;
				load[0][1] -= load[3][0];
			}
		}

		// SAV array
		double sav[4][2] = {
			{fuel_sav1, fuel_savlh},
			{109.0, fuel_savlw},
			{30.0, 0.0},
			{fuel_savlh, 0.0}};

		// heat array
		double heat[4][2] = {
			{fuel_heatd, fuel_heatl},
			{fuel_heatd, fuel_heatl},
			{fuel_heatd, 0.0},
			{fuel_heatd, 0.0}};

		// Convert wind m/s → ft/min, slope → tangent
		double wind = windspd * 88.0;
		double slopex = slope*2;//std::tan(slope * PI / 180.0);

		// Now replicate the rest of your rate-of-spread logic:
		double seff[4][2] = {
			{0.01, 0.01},
			{0.01, 0.01},
			{0.01, 0.00},
			{0.01, 0.00}};
		double fined = 0.0, finel = 0.0, wmfd = 0.0, fdmois = 0.0, w = 0.0, wo = 0.0, sum3 = 0.0;
		double sum1 = 0.0, sum2 = 0.0, rm, sigma = 0.0, rhob;
		double xir, rbqig = 0, xi = 0, wlim, rat, betaop, aa, gammax = 0.0, gamma = 0;
		double a[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		double f[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		double g[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		double gx[5] = {0, 0, 0, 0, 0};
		double wn[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		double qig[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
		double ai[2] = {0, 0}, fi[2] = {0, 0}, hi[2] = {0, 0}, se[2] = {0, 0};
		double xmf[2] = {0, 0}, si[2] = {0, 0}, wni[2] = {0, 0}, etam[2] = {0, 0}, etas[2] = {0, 0}, rir[2] = {0, 0};

		// Fuel weighting factors:
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < (int)nclas[i]; j++)
			{
				a[j][i] = load[j][i] * sav[j][i] / 32.0;
				ai[i] += a[j][i];
				wo += 0.04591 * load[j][i];
			}
			if (nclas[i] != 0)
			{
				for (int j = 0; j < (int)nclas[i]; j++)
				{
					f[j][i] = (ai[i] > 0.0 ? a[j][i] / ai[i] : 0.0);
				}
				memset(gx, 0, 5 * sizeof(double));
				for (int j = 0; j < (int)nclas[i]; j++)
				{
					if (sav[j][i] >= 1200.0)
						gx[0] += f[j][i];
					else if (sav[j][i] >= 192.0)
						gx[1] += f[j][i];
					else if (sav[j][i] >= 96.0)
						gx[2] += f[j][i];
					else if (sav[j][i] >= 48.0)
						gx[3] += f[j][i];
					else if (sav[j][i] >= 16.0)
						gx[4] += f[j][i];
				}
				for (int j = 0; j < (int)nclas[i]; j++)
				{
					if (sav[j][i] >= 1200.0)
						g[j][i] = gx[0];
					else if (sav[j][i] >= 192.0)
						g[j][i] = gx[1];
					else if (sav[j][i] >= 96.0)
						g[j][i] = gx[2];
					else if (sav[j][i] >= 48.0)
						g[j][i] = gx[3];
					else if (sav[j][i] >= 16.0)
						g[j][i] = gx[4];
					else
						g[j][i] = 0.0;
				}
			}
		}

		fi[0] = ai[0] / (ai[0] + ai[1]);
		fi[1] = 1.0 - fi[0];

		// Dead & live fuel moisture of extinction
		if (nclas[1] != 0)
		{
			for (int j = 0; j < (int)nclas[0]; j++)
			{
				double wtfact = load[j][0] * std::exp(-138.0 / sav[j][0]);
				fined += wtfact;
				wmfd += wtfact * mois[j][0];
			}
			fdmois = wmfd / fined;

			for (int j = 0; j < (int)nclas[1]; j++)
			{
				if (sav[j][1] < 1e-6)
					continue;
				finel += load[j][1] * std::exp(-500.0 / sav[j][1]);
			}
			w = fined / finel;
			xmext[1] = 2.9 * w * (1.0 - fdmois / xmext[0]) - 0.226;
			if (xmext[1] < xmext[0])
				xmext[1] = xmext[0];
		}

		// Summation by fuel component
		for (int i = 0; i < 2; i++)
		{
			if (nclas[i] != 0)
			{
				for (int j = 0; j < (int)nclas[i]; j++)
				{
					if (sav[j][i] < 1e-6)
						continue;
					wn[j][i] = 0.04591 * load[j][i] * (1.0 - 0.0555);
					qig[j][i] = 250.0 + 1116.0 * mois[j][i];
					hi[i] += f[j][i] * heat[j][i];
					se[i] += f[j][i] * seff[j][i];
					xmf[i] += f[j][i] * mois[j][i];
					si[i] += f[j][i] * sav[j][i];
					sum1 += 0.04591 * load[j][i];
					sum2 += (0.04591 * load[j][i] / 32.0);
					sum3 += fi[i] * f[j][i] * qig[j][i] * std::exp(-138.0 / sav[j][i]);
				}
				// Weighted wni using g[] factors
				for (int j = 0; j < (int)nclas[i]; j++)
				{
					wni[i] += g[j][i] * wn[j][i];
				}
				double rmi = xmf[i] / xmext[i];
				etam[i] = 1.0 - 2.59 * rmi + 5.11 * std::pow(rmi,2.0) - 3.52 * std::pow(rmi, 3.0);
				if (xmf[i] >= xmext[i])
					etam[i] = 0.0;
				etas[i] = 0.174 / (std::pow(se[i], 0.19));
				if (etas[i] > 1.0)
					etas[i] = 1.0;
				sigma += fi[i] * si[i];
				rir[i] = wni[i] * hi[i] * etas[i] * etam[i];
			}
		}

		rhob = sum1 / fuel_depth;
		double beta = sum2 / fuel_depth;
		betaop = 3.348 / std::pow(sigma, 0.8189);
		rat = beta / betaop;
		aa = 133.0 / std::pow(sigma, 0.7913);
		gammax = sigma * std::sqrt(sigma) / (495.0 + 0.0594 * sigma * std::sqrt(sigma));
		gamma = gammax * std::pow(rat, aa) * std::exp(aa * (1.0 - rat));
		xir = gamma * (rir[0] + rir[1]);
		rbqig = rhob * sum3;
		xi = std::exp((0.792 + 0.681 * std::sqrt(sigma)) * (beta + 0.1)) / (192.0 + 0.2595 * sigma);
		rateo = (xir * xi) / rbqig; // in ft/min

		// Slope factor
		double phis = 5.275 * std::pow(beta, -0.3) * std::pow(slopex,2);
		double cVal = 7.47 * std::exp(-0.133 * std::pow(sigma, 0.55));
		double bVal = 0.02526 * std::pow(sigma, 0.54);
		double eVal = 0.715 * std::exp(-0.000359 * sigma);
		double part1 = cVal * std::pow(rat, -eVal);
		double phiw = std::pow(wind, bVal) * part1;

		wlim = 0.9 * xir;
		if (phis > 0.0)
		{
			if (phis > wlim)
			{
				phis = wlim;
			}
			slopespd = std::pow((phis / part1), (1.0 / bVal)) / 88.0; // mph
		}
		else
		{
			slopespd = 0.0;
		}

		double phiew = phiw + phis;
		double ewind = std::pow((phiew * std::pow(rat, eVal)) / cVal, 1.0 / bVal);
		if (ewind > wlim)
		{
			ewind = wlim;
			phiew = cVal * std::pow(wlim, bVal) * std::pow(rat, -eVal);
		}
		

			
		//rate_of_spread = rateo;
		savx = sigma;
		react = xir * 0.189275;		   // btu/ft2/min → kW/m2
		rateo = rateo * 0.30480060960; // ft/min → m/min
		
		double rate_of_spread = 0.6  ;

		if((std::isnan(phiew)) || (phiew<0)){
			rate_of_spread = rateo ;
		}else{
			rate_of_spread = rateo * (1 + phiew);
		}


		rate_of_spread = rate_of_spread/60.0;

		if (csvfile.is_open()) {
			csvfile << rate_of_spread<<";"<<rateo/60.0 <<";"<<phiew;   
			for (size_t i = 0; i < wantedProperties.size(); ++i) {
				csvfile << ";" << valueOf[i] ;
			}
			csvfile << std::endl;
		}
		return rate_of_spread; // eventually in m/s
	}

}
