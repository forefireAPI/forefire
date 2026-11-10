/**
 * @file farsite_reference.cpp
 * @brief Surface spread of the FARSITE reference, for comparison with ForeFire.
 * @license Public domain, as the FARSITE source it is copied from (17 USC 105),
 *          except contributions from outside the RMRS lab, under the BSD
 *          license below.
 *
 * The arithmetic below is copied from mbedward/farsite, commit
 * 4537d60ab013fa91d0b49d8efb385ffb1b3ddd13 (https://github.com/mbedward/farsite).
 * Every block names the file and lines it comes from. Only the plumbing around
 * it changed: the fuel comes from a local table instead of Farsite5, the slope
 * arrives as a tangent instead of whole degrees, and the wind always blows
 * straight upslope, so the wind/slope vector is a plain sum.
 *
 * Reads one case per line on stdin:
 *     fuel ones tens hundreds liveh livew wind_mph slope_tan
 * (moistures as fractions of dry weight, wind at midflame) and writes one line
 * per case on stdout:
 *     rateo phiw phis phiew head lb flank back
 * rateo is the no-wind no-slope spread, head/flank/back the spread rates at
 * the head, flank and back of the ellipse, all in m/min.
 *
 * Build: c++ -O2 -o farsite_reference farsite_reference.cpp
 *
 * Copyright (c) 2016 The FARSITE Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace std;

static const double PI = acos(-1.0);  // Farsite5.cpp:48

inline double pow2(double in) { return in * in; }  // Farsite5.h:40-43

// Farsite5.h:183-202
typedef struct
{
     long number;
     char code[8];
     double h1;
     double h10;
     double h100;
     double lh;
     double lw;
     long dynamic;
     long sav1;
     long savlh;
     long savlw;
     double depth;
     double xmext;
     double heatd;
     double heatl;
     char desc[256];
}NewFuel;

// Farsite5.cpp:6907-7128, Farsite5::InitializeNewFuel, the 13 standard models
static const NewFuel newfuels[14] = {
     {0,  "",     0,     0,     0,     0, 0,    0, 0,    0,    0,    0,    0,    0,    0,    ""},
     {1,  "FM1",  0.74,  0,     0,     0, 0,    0, 3500, 1800, 1500, 1.0,  0.12, 8000, 8000, "Short Grass"},
     {2,  "FM2",  2.0,   1.0,   0.5,   0, 0.5,  0, 3000, 1800, 1500, 1.0,  0.15, 8000, 8000, "Timber Grass/Understory"},
     {3,  "FM3",  3.01,  0,     0,     0, 0,    0, 1500, 1800, 1500, 2.5,  0.25, 8000, 8000, "Tall Grass"},
     {4,  "FM4",  5.01,  4.01,  2.0,   0, 5.01, 0, 2000, 1800, 1500, 6.0,  0.20, 8000, 8000, "Chaparral"},
     {5,  "FM5",  1.0,   0.5,   0,     0, 2.0,  0, 2000, 1800, 1500, 2.0,  0.20, 8000, 8000, "Short Brush"},
     {6,  "FM6",  1.5,   2.5,   2.0,   0, 0,    0, 1750, 1800, 1500, 2.5,  0.25, 8000, 8000, "Dormant Brush"},
     {7,  "FM7",  1.13,  1.87,  1.5,   0, 0.37, 0, 1550, 1800, 1500, 2.5,  0.40, 8000, 8000, "Southern Rough"},
     {8,  "FM8",  1.5,   1.0,   2.5,   0, 0,    0, 2000, 1800, 1500, 0.2,  0.30, 8000, 8000, "Closed Timber Litter"},
     {9,  "FM9",  2.92,  0.41,  0.15,  0, 0,    0, 2500, 1800, 1500, 0.2,  0.25, 8000, 8000, "Hardwood Litter"},
     {10, "FM10", 3.01,  2.0,   5.01,  0, 2.0,  0, 2000, 1800, 1500, 1.0,  0.25, 8000, 8000, "Timber Litter/Understory"},
     {11, "FM11", 1.5,   4.51,  5.51,  0, 0,    0, 1500, 1800, 1500, 1.0,  0.15, 8000, 8000, "Light Slash"},
     {12, "FM12", 4.01,  14.03, 16.53, 0, 0,    0, 1500, 1800, 1500, 2.3,  0.20, 8000, 8000, "Medium Slash"},
     {13, "FM13", 7.01,  23.04, 28.05, 0, 0,    0, 1500, 1800, 1500, 3.0,  0.25, 8000, 8000, "Heavy Slash"},
};

static bool GetNewFuel(long number, NewFuel* newfuel)
{
	if (number < 1 || number > 13)
		return false;
	memcpy(newfuel, &newfuels[number], sizeof(NewFuel));
	return true;
}

// The members of Mechanix (fsx4.hpp:483-522) that the functions below use.
struct Mechanix
{
	double lb_ratio, hb_ratio, rateo;
	double b, part1;
	double m_ones, m_tens, m_hundreds, m_livew, m_liveh;
	double phiw, phis, phiew, LocalWindDir;
	double slopespd;
	double vecspeed, ivecspeed, m_windspd;
	double fros, vecros;
	double react, savx;
	double head, back, flank;

	double spreadrate(double slope_tan, double windspd, int fuel);
	void VecSurf(double slope_tan, double aspectf);
	double vectorspd(double* ivecspeed, double aspectf, long CrownWinds);
	double headback();
	void ellipse(double iros, double wspeed);
};

// fsxwmech.cpp:784-1045, Mechanix::spreadrate (the one compiled in; the two
// versions after it in that file are commented out). Changed: the slope
// argument is already a tangent, and GetNewFuel is the local table.
double Mechanix::spreadrate(double slope_tan, double windspd, int fuel)
{// Rothermel spread equation based on BEHAVE source code,
 //  support for dynamic fuel models added 10/13/2004
	long i, j, ndead=0, nlive=0;
	double depth;
	double seff[4][2]={{0.01,0.01},{0.01,0.01},{0.01,0},{0.01,0.0}};	     //mineral content
	double wtfact, fined=0.0, finel=0.0, wmfd=0.0, fdmois=0.0, w=0.0, wo=0.0, beta;
	double rm, sigma=0.0, rhob, sum3=0.0, betaop, rat, aa, gammax=0.0, gamma=0, wind=0;
	double xir, rbqig=0, xi=0, c, e, slopex=0;
	double ewind=0, wlim, sum1=0, sum2=0;

	double mois[4][2]=		// fraction of oven-dry weight
	{	{m_ones, m_liveh},        // use Mechanix Class copies of FE data
		{m_tens, m_livew},
		{m_hundreds, 0.0},
		{m_ones, 0.0}
	};
	double gx[5]={0.0, 0.0, 0.0, 0.0, 0.0};
	double wn[4][2]={{0,0},{0,0},{0,0},{0,0}};
	double qig[4][2]={{0,0},{0,0},{0,0},{0,0}};
	double a[4][2]={{0,0},{0,0},{0,0},{0,0}};
	double f[4][2]={{0,0},{0,0},{0,0},{0,0}};
	double g[4][2]={{0,0},{0,0},{0,0},{0,0}};
	double ai[2]={0,0};
	double fi[2]={0,0};
	double hi[2]={0,0};
	double se[2]={0,0};
	double xmf[2]={0,0};
	double si[2]={0,0};
	double wni[2]={0,0};
	double etam[2]={0,0};
	double etas[2]={0,0};
	double rir[2]={0,0};

     NewFuel newfuel;

     memset(&newfuel, 0x0, sizeof(NewFuel));
     if(!GetNewFuel(fuel, &newfuel)) // will get all models, 13, 40, and custom
     {	rateo=0.0;

          return rateo;
     }

     // count number of fuels
     if(newfuel.h1) ndead++;
	if(newfuel.h10) ndead++;
	if(newfuel.h100) ndead++;
	if(newfuel.lh) nlive++;
	if(newfuel.lw) nlive++;

     if(nlive==0)
     	newfuel.dynamic=0;

     if(nlive>0)
          nlive=2;                      // boost to max number
     if(ndead>0)
          ndead=4;

     double nclas[2]={(double) ndead,(double) nlive};  // # of dead & live fuel classes
    	double xmext[2]={newfuel.xmext, 0};

    	double load[4][2]=			     // tons per acre, later converted to lb/ft2
	{	{newfuel.h1, newfuel.lh},
		{newfuel.h10, newfuel.lw},
		{newfuel.h100, 0.0},
		{0.0, 0.0},
	};

     depth=newfuel.depth;

     //-------------------------------------------------------------------------
     // do the dynamic load transfer
     //-------------------------------------------------------------------------
     if(newfuel.dynamic)
     {    if(mois[0][1]<0.30) // if live herbaceous is less than 30.0
          {    load[3][0]=load[0][1];
               load[0][1]=0.0;
          }
          else if(mois[0][1]<1.20)
          {    load[3][0]=load[0][1]*(1.20-mois[0][1])/0.9;
               load[0][1]-=load[3][0];
          }
     }
     //-------------------------------------------------------------------------

	double sav[4][2]=			     // 1/ft
	{	{(double) newfuel.sav1, (double) newfuel.savlh},
		{109.0, (double) newfuel.savlw},
		{30.0, 0.0},
		{(double) newfuel.savlh, 0.0},
	};

	double heat[4][2]=
     {    {newfuel.heatd, newfuel.heatl},
          {newfuel.heatd, newfuel.heatl},
          {newfuel.heatd, 0.0},
          {newfuel.heatd, 0.0},
     };

	wind=windspd*88.0;                      // ft/minute
	slopex=slope_tan;  	// was tan((double) slope/180.0*PI), slope in whole degrees

	// fuel weighting factors
	for(i=0; i<2; i++)
	{	for(j=0; j<nclas[i]; j++)
		{    a[j][i]=load[j][i]*sav[j][i]/32.0;
			ai[i]=ai[i]+a[j][i];
			wo=wo+0.04591*load[j][i];
		}
		if(nclas[i]!=0)
		{	for(j=0; j<nclas[i]; j++)
			{    if(ai[i]>0.0)
                    	f[j][i]=a[j][i]/ai[i];
                    else
                         f[j][i]=0.0;
               }
               memset(gx, 0x0, 5*sizeof(double));
               for(j=0; j<nclas[i]; j++)
			{    if(sav[j][i]>=1200.0)
	              		gx[0]+=f[j][i];
				else if(sav[j][i]>=192.0)
               		gx[1]+=f[j][i];
				else if(sav[j][i]>=96.0)
               		gx[2]+=f[j][i];
				else if(sav[j][i]>=48.0)
               		gx[3]+=f[j][i];
				else if(sav[j][i]>=16.0)
               		gx[4]+=f[j][i];
			}
               for(j=0; j<nclas[i]; j++)
			{    if(sav[j][i]>=1200.0)
	              		g[j][i]=gx[0];
				else if(sav[j][i]>=192.0)
	              		g[j][i]=gx[1];
				else if(sav[j][i]>=96.0)
	              		g[j][i]=gx[2];
				else if(sav[j][i]>=48.0)
	              		g[j][i]=gx[3];
				else if(sav[j][i]>=16.0)
	              		g[j][i]=gx[4];
                    else
                    	g[j][i]=0.0;
			}

		}
	}
	fi[0]=ai[0]/(ai[0]+ai[1]);
	fi[1]=1.0-fi[0];

	/* no need for this, because extinction moistures are assigned */
	/* as on last page of Burgan and Rothermel 1984 */
	/*	rhob=(wo/depth);
		beta=rhob/32;
		xmext[0]=.12+4.*beta;
	*/

	//moisture of extinction
	if(nclas[1]!=0)
	{	for(j=0; j<nclas[0]; j++)
		{    wtfact=load[j][0]*exp(-138.0/sav[j][0]);
			fined=fined+wtfact;
			wmfd=wmfd+wtfact*mois[j][0];
		}
		fdmois=wmfd/fined;
		for(j=0; j<nclas[1]; j++)
		{    if(sav[j][1]<1e-6)
              		continue;
			finel=finel+load[j][1]*exp(-500.0/sav[j][1]);
          }
		w=fined/finel;
		xmext[1]=2.9*w*(1.0-fdmois/xmext[0])-0.226;
		if(xmext[1]<xmext[0])
			xmext[1]=xmext[0];
	}

	// intermediate calculations, summing parameters by fuel component
	for(i=0; i<2; i++)
	{	if(nclas[i]!=0)
		{	for(j=0; j<nclas[i]; j++)
			{    if(sav[j][i]<1e-6)
               		continue;
               	wn[j][i]=0.04591*load[j][i]*(1.0-0.0555);
				qig[j][i]=250.0+1116.0*mois[j][i];
				hi[i]=hi[i]+f[j][i]*heat[j][i];
				se[i]=se[i]+f[j][i]*seff[j][i];
				xmf[i]=xmf[i]+f[j][i]*mois[j][i];
				si[i]=si[i]+f[j][i]*sav[j][i];
				sum1=sum1+0.04591*load[j][i];
				sum2=sum2+0.04591*load[j][i]/32.0;
				sum3=sum3+fi[i]*f[j][i]*qig[j][i]*exp(-138.0/sav[j][i]);
			}
			for(j=0; j<nclas[i]; j++)
				//wni[i]=wni[i]+f[j][i]*wn[j][i];
				wni[i]=wni[i]+g[j][i]*wn[j][i];  /* g[j][i] should be subst for f[j][i] in the wni[i] equation */
										   /* if the above g-factors are calculated */
			rm=xmf[i]/xmext[i];
			etam[i]=1.0-2.59*rm+5.11*pow2(rm)-3.52*pow(rm,3.0);
			if(xmf[i] >= xmext[i])
				etam[i]=0;
			etas[i]=0.174/(pow(se[i],0.19));
			if(etas[i]>1.0)
				etas[i]=1.0;
			sigma=sigma+fi[i]*si[i];
			rir[i]=wni[i]*hi[i]*etas[i]*etam[i];
		}
	}

	/* final calculations */
	rhob=sum1/depth;
	beta=sum2/depth;
	betaop=3.348/pow(sigma,0.8189);
	rat=beta/betaop;
	aa=133.0/pow(sigma,0.7913);
	//gammax=pow(sigma,1.5)/(495.0+0.0594*pow(sigma,1.5));
     gammax=(sigma*sqrt(sigma))/(495.0+0.0594*sigma*sqrt(sigma));
	gamma=gammax*pow(rat,aa)*exp(aa*(1.0-rat));
	xir=gamma*(rir[0]+rir[1]);
	rbqig=rhob*sum3;
	xi=exp((0.792+0.681*sqrt(sigma))*(beta+0.1))/(192.0+0.2595*sigma);
/*	flux=xi*xir;*/
	rateo=xir*xi/rbqig;   /* this is in English units */

	phis=5.275*pow(beta,-0.3)*pow2(slopex);
		c=7.47*exp(-0.133*pow(sigma,0.55));
		b=0.02526*pow(sigma,0.54);
		e=0.715*exp(-0.000359*sigma);
		part1=c*pow(rat,-e);
	phiw=pow(wind,b)*part1;

	wlim=0.9*xir;

	if(phis>0.0)
     {	if(phis>wlim)             // can't have inifinite windspeed
     		phis=wlim;
		slopespd=pow((phis/part1),1.0/b)/88.0; 	// converts phis to windspd in mph
     }
    	else
		slopespd=0.0;

/*   maximum windspeed effect on ros*/
	phiew=phiw+phis;
	ewind=pow(((phiew*pow(rat,e))/c),1.0/b);

	if(ewind>wlim)
	{	ewind=wlim;
		phiew=c*pow(wlim,b)*pow(rat,-e);
	}
	savx=sigma;					// SAVX, REACT IN MECHANIX.PUBLIC:
     react=xir*0.189275;  		    	// convert btu/f2/min to kW/m2
	rateo=rateo*0.30480060960;				// convert from f/min to m/min

     return rateo;
}

// fsxwmech.cpp:443-466, MechCalls::VecSurf. Changed: ld.slope > 0 tests the
// tangent, and TransformWindDir is skipped because the wind blows upslope
// (fsxwmech.cpp:96, a wind direction of -1 means "aligned with the aspect").
void Mechanix::VecSurf(double slope_tan, double aspectf)
{
	// calculates wind/slope vector for surface fire
	if (slope_tan > 0)
	{
		LocalWindDir = aspectf;
		vecspeed = vectorspd(&ivecspeed, aspectf, 0);
		if (vecspeed > phiew)
			vecspeed = phiew;   		 // vecspeed is still dimensionless
		vecros = (fros * (1.0 + vecspeed));   // wind/slope vectored spread rate
	}
	else
	{
		ivecspeed = m_windspd;
		if (phiw > phiew)
			phiw = phiew;
		vecspeed = phiw;
		vecros = (fros * (1 + phiw));	// winddriven spread rate in wind directn
	}
}

// fsxwmech.cpp:1726-1760, Mechanix::vectorspd, surface branch only
double Mechanix::vectorspd(double* ivecspeed, double aspectf, long CrownWinds)
{
	// determines the direction and strength of windXslope vector
	double angle, vecspeed;

	angle = fabs(LocalWindDir - aspectf);
	if (angle != PI)					// dimensionless coefs
	{
		vecspeed = sqrt(pow2(phiw) +
					pow2(phis) +
					(2.0 * phiw * phis * cos(angle)));
		*ivecspeed = sqrt(pow2(m_windspd) +
						pow2(slopespd) +
						(2.0 * m_windspd * slopespd * cos(angle)));
	}							// actual windspeed vector, not dimensionless coefs
	else
	{
		vecspeed = fabs(phis - phiw);
		*ivecspeed = fabs(slopespd - m_windspd);
	}
	(void) CrownWinds;

	return vecspeed;
}

// fsxwmech.cpp:1630-1639, Mechanix::headback
double Mechanix::headback()
{
	// calculates Alexanders head/backing ratio assuming fire origin at focus equation [16]
	double hb_ratio, part;

	part = sqrt(pow2(lb_ratio) - 1);
	hb_ratio = (lb_ratio + part) / (lb_ratio - part);

	return hb_ratio;
}

// fsxwmech.cpp:1642-1666, Mechanix::ellipse. ConstantBackingSpreadRate is
// false by default (Farsite5.cpp:291).
void Mechanix::ellipse(double iros, double wspeed)
{
	// ANDERSON 1983 FOR MIDFLAME WINDSPEED
	lb_ratio = .936 * exp(.1147 * wspeed) +
		.461 * exp(-.0692 * wspeed) -
		.397;

	if (lb_ratio > 8.0) 			 // maximum eccentricity
		lb_ratio = 8.0;
	hb_ratio = headback();		 //	subt 0.397 to make circle at 0 windspd (my own addition)
	head = iros;
	back = head / hb_ratio;		// backspread from hb_ratio from Alexander 1985
	flank = ((head + back) / lb_ratio) / 2.0;
	head = (head + back) / 2.0;
	back = head - back;			// modifies rates of spread for elliptical dimensions
}

int main()
{
	int fuel;
	double ones, tens, hundreds, liveh, livew, wind_mph, slope_tan;
	while (scanf("%d %lf %lf %lf %lf %lf %lf %lf", &fuel, &ones, &tens,
				 &hundreds, &liveh, &livew, &wind_mph, &slope_tan) == 8)
	{
		Mechanix m;
		memset(&m, 0, sizeof(m));
		m.m_ones = ones;
		m.m_tens = tens;
		m.m_hundreds = hundreds;
		m.m_liveh = liveh;
		m.m_livew = livew;
		m.m_windspd = wind_mph;

		// fsxwburn4.cpp:279-285, BurnThread::SurfaceFire
		m.fros = m.spreadrate(slope_tan, m.m_windspd, fuel);
		double phiw = m.phiw, phis = m.phis, phiew = m.phiew;
		double head = 0, lb = 1, flank = 0, back = 0;
		if (m.fros > 0.0)
		{
			m.VecSurf(slope_tan, 0.0);
			head = m.vecros;
			m.ellipse(m.vecros, m.ivecspeed);
			lb = m.lb_ratio;
			// fsxwmech.cpp:509-512, SpreadCorrect at Normal = pi/2 and pi
			flank = m.flank;
			back = m.head - m.back;
		}
		printf("%.12g %.12g %.12g %.12g %.12g %.12g %.12g %.12g\n",
			   m.fros, phiw, phis, phiew, head, lb, flank, back);
	}
	return 0;
}
