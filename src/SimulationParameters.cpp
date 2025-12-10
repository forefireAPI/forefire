/**
 * @file SimulationParameters.cpp
 * @brief Implementation of the SimulationParameters class
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "SimulationParameters.h"

#include <string>
#include <iostream>
#include <map>
#include <algorithm>

namespace libforefire {


	
SimulationParameters* SimulationParameters::instance = 0;

string SimulationParameters::undefined = "1234567890";
double SimulationParameters::doubleUndefined = 1234567890.;
int SimulationParameters::intUndefined = 1234567890;
size_t SimulationParameters::sizeUndefined = 1234567890;


SimulationParameters* SimulationParameters::GetInstance(){
	if ( instance == 0 ) instance =  new SimulationParameters;
	return instance;
}

string SimulationParameters::FormatISODate(double secs, int year, int yday){
    
    char s[22];
    year -= 1900;
    yday--;
    int tt = secs + yday*86400 + (year-70)*31536000 + ((year-69)/4)*86400 - ((year-1)/100)*86400 + ((year+299)/400)*86400;
    time_t rawtime = (time_t) tt;
    struct tm * timeInfo;
    timeInfo = gmtime(&rawtime);
    strftime(s, 22, "%Y-%m-%dT%H:%M:%SZ", timeInfo);
    return string(s);
}
/*
bool SimulationParameters::ISODateDecomposition(string date, double &secs, int &year, int &yday)
{
    // Exemple of Date : 2013-01-01T01:01:30Z
    if (date.size() != 20)
        return false;
    
    struct tm tm;

    memset(&tm, 0, sizeof tm);

    strptime(date.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);

    cout << "day of year "<<tm.tm_yday<<endl;

    secs = tm.tm_sec + tm.tm_hour * 3600 + tm.tm_min * 60;
    year = tm.tm_year + 1900;
    yday = tm.tm_yday + 1;

    return true;
}
*/

bool SimulationParameters::ISODateDecomposition(string date, double &secs, int &year, int &yday)
{
    // Example of Date : 2013-01-01T01:01:30Z
    if (date.size() != 20)
        return false;
    /*
    struct tm tm;
    strptime(date.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);

    secs = tm.tm_sec + tm.tm_hour * 3600 + tm.tm_min * 60;
    year = tm.tm_year + 1900;
    yday = tm.tm_yday + 1;
    */

	year = atoi(date.substr(0, 4).c_str());
    secs = (atoi(date.substr(11, 2).c_str()) * 3600) + (atoi(date.substr(14, 2).c_str()) * 60) + atoi(date.substr(17, 2).c_str());

	int daysPerMonth[] = {31, ((((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0))) ? 29 : 28), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int month = atoi(date.substr(5, 2).c_str()) - 1;

    yday = atoi(date.substr(8, 2).c_str());

	for (int i = 0; i < month; i++)
	{
		yday += daysPerMonth[i];
	}
    
    return true;
}

double SimulationParameters::SecsBetween(double t1, int y1, int yday1,
                                         double t2, int y2, int yday2)
{
    /* -----------------------------------------------------------
     *  Convert both (year, day‑of‑year, seconds‑of‑day) triplets
     *  to seconds since the Unix epoch (1970‑01‑01 00:00:00 UTC)
     *  using an arithmetic formula that handles years both
     *  before and after 1970.  The result keeps the sign, so
     *  the caller can get a positive or negative difference.
     * ----------------------------------------------------------- */

    auto daysFromEpoch = [](int year, int yday) -> long long
    {
        /* Number of days between 1970‑01‑01 and the first
         * day of the given ISO year.
         *
         * Days = 365 × ΔY + leapDays
         * where leapDays counts the Gregorian leap‑year rule:
         *   leap years are divisible by 4,
         *   except centuries, unless divisible by 400.
         */
        long long y = static_cast<long long>(year);
        long long delta = y - 1970;               // years offset (may be negative)

        // Count leap days from 1970 (exclusive) up to the given year (exclusive).
        long long leaps = (y - 1) / 4  - 1969 / 4
                        - (y - 1) / 100 + 1969 / 100
                        + (y - 1) / 400 - 1969 / 400;

        return delta * 365LL + leaps + static_cast<long long>(yday - 1);
    };

    const long long secsPerDay = 86400LL;

    long double secs1 = static_cast<long double>(daysFromEpoch(y1, yday1)) * secsPerDay + t1;
    long double secs2 = static_cast<long double>(daysFromEpoch(y2, yday2)) * secsPerDay + t2;

    return static_cast<double>(secs2 - secs1);
}

string SimulationParameters::GetPath(string arg)
{
	if (arg.size()==0){
		return "";
	}
    // If the first character of the path is "/", then it's an absolute path
    if (arg.at(0) == '/') {
        return arg;
    }
    
    // Else converts the relative path to an absolute path
    // Example : 
    // If  caseDirectory = /home/svn/web/apiDEV/out  AND  ForeFireDataDirectory = 2012-05-03
    // Then  ../../../layers => /home/svn/web/apiDEV/out/2012-05-03/../../../layers
    // And so understood as  /home/svn/web/layers
    return GetInstance()->getParameter("caseDirectory") + '/' + GetInstance()->getParameter("ForeFireDataDirectory") + '/' + arg;
}

SimulationParameters::SimulationParameters(){

std::string fuelsFarsiteCSV = R"(Index;code;h1;h10;h100;lh;lw;dynamic;sav1;savlh;savlw;depth;xmext;heatd;heatl;desc
0;NB0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;NO DATA MODEL
1;FM1;0.740000;0.000000;0.000000;0.000000;0.000000;0;3500;1800;1500;1.000000;0.120000;8000.000000;8000.000000;Short Grass
2;FM2;2.000000;1.000000;0.500000;0.000000;0.500000;0;3000;1800;1500;1.000000;0.150000;8000.000000;8000.000000;Timber Grass/Understory
3;FM3;3.010000;0.000000;0.000000;0.000000;0.000000;0;1500;1800;1500;2.500000;0.250000;8000.000000;8000.000000;Tall Grass
4;FM4;5.010000;4.010000;2.000000;0.000000;5.010000;0;2000;1800;1500;6.000000;0.200000;8000.000000;8000.000000;Chaparral
5;FM5;1.000000;0.500000;0.000000;0.000000;2.000000;0;2000;1800;1500;2.000000;0.200000;8000.000000;8000.000000;Short Brush
6;FM6;1.500000;2.500000;2.000000;0.000000;0.000000;0;1750;1800;1500;2.500000;0.250000;8000.000000;8000.000000;Dormant Brush
7;FM7;1.130000;1.870000;1.500000;0.000000;0.370000;0;1550;1800;1500;2.500000;0.400000;8000.000000;8000.000000;Southern Rough
8;FM8;1.500000;1.000000;2.500000;0.000000;0.000000;0;2000;1800;1500;0.200000;0.300000;8000.000000;8000.000000;Closed Timber Litter
9;FM9;2.920000;0.410000;0.150000;0.000000;0.000000;0;2500;1800;1500;0.200000;0.250000;8000.000000;8000.000000;Hardwood Litter
10;FM10;3.010000;2.000000;5.010000;0.000000;2.000000;0;2000;1800;1500;1.000000;0.250000;8000.000000;8000.000000;Timber Litter/Understory
11;FM11;1.500000;4.510000;5.510000;0.000000;0.000000;0;1500;1800;1500;1.000000;0.150000;8000.000000;8000.000000;Light Slash
12;FM12;4.010000;14.030000;16.530000;0.000000;0.000000;0;1500;1800;1500;2.300000;0.200000;8000.000000;8000.000000;Medium Slash
13;FM13;7.010000;23.040000;28.050000;0.000000;0.000000;0;1500;1800;1500;3.000000;0.250000;8000.000000;8000.000000;Heavy Slash
95;NB5;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;Urban or Developed
96;NB4;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;Agricultural or Cropland
97;NB3;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;Snow or Ice
98;NB2;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;Water
99;NB1;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;Barren
101;GR1;0.100000;0.000000;0.000000;0.300000;0.000000;1;2200;2000;1500;0.400000;0.150000;8000.000000;8000.000000;Short, sparse, dry climate grass
102;GR2;0.100000;0.000000;0.000000;1.000000;0.000000;1;2000;1800;1500;1.000000;0.150000;8000.000000;8000.000000;Low load, dry climate grass
103;GR3;0.100000;0.400000;0.000000;1.500000;0.000000;1;1500;1300;1500;2.000000;0.300000;8000.000000;8000.000000;Low load, very coarse, humid climate grass
104;GR4;0.250000;0.000000;0.000000;1.900000;0.000000;1;2000;1800;1500;2.000000;0.150000;8000.000000;8000.000000;Moderate load, dry climate grass
105;GR5;0.400000;0.000000;0.000000;2.500000;0.000000;1;1800;1600;1500;1.500000;0.400000;8000.000000;8000.000000;Low load, humid climate grass
106;GR6;0.100000;0.000000;0.000000;3.400000;0.000000;1;2200;2000;1500;1.500000;0.400000;9000.000000;9000.000000;Moderate load, humid climate grass
107;GR7;1.000000;0.000000;0.000000;5.400000;0.000000;1;2000;1800;1500;3.000000;0.150000;8000.000000;8000.000000;High load, dry climate grass
108;GR8;0.500000;1.000000;0.000000;7.300000;0.000000;1;1500;1300;1500;4.000000;0.300000;8000.000000;8000.000000;High load, very coarse, humid climate grass
109;GR9;1.000000;1.000000;0.000000;9.000000;0.000000;1;1800;1600;1500;5.000000;0.400000;8000.000000;8000.000000;Very high load, humid climate grass
121;GS1;0.200000;0.000000;0.000000;0.500000;0.650000;1;2000;1800;1800;0.900000;0.150000;8000.000000;8000.000000;Low load, dry climate grass-shrub
122;GS2;0.500000;0.500000;0.000000;0.600000;1.000000;1;2000;1800;1800;1.500000;0.150000;8000.000000;8000.000000;Moderate load, dry climate grass-shrub
123;GS3;0.300000;0.250000;0.000000;1.450000;1.250000;1;1800;1600;1600;1.800000;0.400000;8000.000000;8000.000000;Moderate load, humid climate grass-shrub
124;GS4;1.900000;0.300000;0.100000;3.400000;7.100000;1;1800;1600;1600;2.100000;0.400000;8000.000000;8000.000000;High load, humid climate grass-shrub
141;SH1;0.250000;0.250000;0.000000;0.150000;1.300000;1;2000;1800;1600;1.000000;0.150000;8000.000000;8000.000000;Low load, dry climate shrub
142;SH2;1.350000;2.400000;0.750000;0.000000;3.850000;0;2000;1800;1600;1.000000;0.150000;8000.000000;8000.000000;Moderate load, dry climate shrub
143;SH3;0.450000;3.000000;0.000000;0.000000;6.200000;0;1600;1800;1400;2.400000;0.400000;8000.000000;8000.000000;Moderate load, humid climate shrub
144;SH4;0.850000;1.150000;0.200000;0.000000;2.550000;0;2000;1800;1600;3.000000;0.300000;8000.000000;8000.000000;Low load, humid climate timber-shrub
145;SH5;3.600000;2.100000;0.000000;0.000000;2.900000;0;750;1800;1600;6.000000;0.150000;8000.000000;8000.000000;High load, dry climate shrub 
146;SH6;2.900000;1.450000;0.000000;0.000000;1.400000;0;750;1800;1600;2.000000;0.300000;8000.000000;8000.000000;Low load, humid climate shrub
147;SH7;3.500000;5.300000;2.200000;0.000000;3.400000;0;750;1800;1600;6.000000;0.150000;8000.000000;8000.000000;Very high load, dry climate shrub
148;SH8;2.050000;3.400000;0.850000;0.000000;4.350000;0;750;1800;1600;3.000000;0.400000;8000.000000;8000.000000;High load, humid climate shrub
149;SH9;4.500000;2.450000;0.000000;1.550000;7.000000;1;750;1800;1500;4.400000;0.400000;8000.000000;8000.000000;Very high load, humid climate shrub
161;TU1;0.200000;0.900000;1.500000;0.200000;0.900000;1;2000;1800;1600;0.600000;0.200000;8000.000000;8000.000000;Light load, dry climate timber-grass-shrub
162;TU2;0.950000;1.800000;1.250000;0.000000;0.200000;0;2000;1800;1600;1.000000;0.300000;8000.000000;8000.000000;Moderate load, humid climate timber-shrub
163;TU3;1.100000;0.150000;0.250000;0.650000;1.100000;1;1800;1600;1400;1.300000;0.300000;8000.000000;8000.000000;Moderate load, humid climate timber-grass-shrub
164;TU4;4.500000;0.000000;0.000000;0.000000;2.000000;0;2300;1800;2000;0.500000;0.120000;8000.000000;8000.000000;Dwarf conifer with understory
165;TU5;4.000000;4.000000;3.000000;0.000000;3.000000;0;1500;1800;750;1.000000;0.250000;8000.000000;8000.000000;Very high load, dry climate timber-shrub
181;TL1;1.000000;2.200000;3.600000;0.000000;0.000000;0;2000;1800;1600;0.200000;0.300000;8000.000000;8000.000000;Low load, compact conifer litter
182;TL2;1.400000;2.300000;2.200000;0.000000;0.000000;0;2000;1800;1600;0.200000;0.250000;8000.000000;8000.000000;Low load broadleaf litter
183;TL3;0.500000;2.200000;2.800000;0.000000;0.000000;0;2000;1800;1600;0.300000;0.200000;8000.000000;8000.000000;Moderate load confier litter
184;TL4;0.500000;1.500000;4.200000;0.000000;0.000000;0;2000;1800;1600;0.400000;0.250000;8000.000000;8000.000000;Small downed logs
185;TL5;1.150000;2.500000;4.400000;0.000000;0.000000;0;2000;1800;1600;0.600000;0.250000;8000.000000;8000.000000;High load conifer litter
186;TL6;2.400000;1.200000;1.200000;0.000000;0.000000;0;2000;1800;1600;0.300000;0.250000;8000.000000;8000.000000;High load broadleaf litter
187;TL7;0.300000;1.400000;8.100000;0.000000;0.000000;0;2000;1800;1600;0.400000;0.250000;8000.000000;8000.000000;Large downed logs
188;TL8;5.800000;1.400000;1.100000;0.000000;0.000000;0;1800;1800;1600;0.300000;0.350000;8000.000000;8000.000000;Long-needle litter
189;TL9;6.650000;3.300000;4.150000;0.000000;0.000000;0;1800;1800;1600;0.600000;0.350000;8000.000000;8000.000000;Very high load broadleaf litter
201;SB1;1.500000;3.000000;11.000000;0.000000;0.000000;0;2000;1800;1600;1.000000;0.250000;8000.000000;8000.000000;Low load activity fuel
202;SB2;4.500000;4.250000;4.000000;0.000000;0.000000;0;2000;1800;1600;1.000000;0.250000;8000.000000;8000.000000;Moderate load activity or low load blowdown
203;SB3;5.500000;2.750000;3.000000;0.000000;0.000000;0;2000;1800;1600;1.200000;0.250000;8000.000000;8000.000000;High load activity fuel or moderate load blowdown
204;SB4;5.250000;3.500000;5.250000;0.000000;0.000000;0;2000;1800;1600;2.700000;0.250000;8000.000000;8000.000000;High load blowdown)";




std::string landCoverIndexedCSV = R"(Index;Rhod;Rhol;Md;Ml;sd;sl;e;Sigmad;Sigmal;stoch;RhoA;Ta;Tau0;Deltah;DeltaH;Cp;Cpa;Ti;X0;r00;Blai;me
0;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
1;563.0;522.0;0.1;1.0;6099.0;7273.0;0.24;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
2;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
3;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
4;613.0;538.0;0.1;1.0;4357.0;6524.0;0.19;1.286;0.085;8.3;1.0;300;70000;18677000.0;18677000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
5;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
6;562.0;474.0;0.1;1.0;6740.0;8195.0;0.57;1.326;0.166;8.3;1.0;300;70000;18941000.0;18941000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
7;658.0;651.0;0.1;1.0;4734.0;5733.0;0.15;1.415;0.541;8.3;1.0;300;70000;18472000.0;18466000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
8;446.0;513.0;0.1;1.0;7792.0;9072.0;0.78;0.492;0.023;8.3;1.0;300;70000;18587000.0;18587000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
9;467.0;543.0;0.1;1.0;6115.0;7224.0;0.285;0.855;0.174;8.3;1.0;300;70000;18474000.0;18474000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
10;674.0;612.0;0.1;1.0;4801.0;5928.0;0.45;1.525;0.709;8.3;1.0;300;70000;18280000.0;18277000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
11;653.0;582.0;0.1;1.0;4753.0;6569.0;0.75;1.096;1.105;8.3;1.0;300;70000;18226000.0;18221000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
12;596.0;586.0;0.1;1.0;3688.0;5551.0;0.475;1.346;0.077;8.3;1.0;300;70000;19050000.0;19050000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
13;438.0;488.0;0.1;1.0;7274.0;8453.0;0.38;1.053;0.321;8.3;1.0;300;70000;17842000.0;17842000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
14;634.0;570.0;0.1;1.0;5518.0;7704.0;0.2;1.293;0.402;8.3;1.0;300;70000;18507000.0;18507000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
15;667.0;636.0;0.1;1.0;5162.0;7145.0;0.3;1.457;0.519;8.3;1.0;300;70000;18851000.0;18851000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
16;563.0;531.0;0.1;1.0;4694.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19235000.0;19235000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
17;599.0;531.0;0.1;1.0;5813.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19041000.0;19041000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
18;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
19;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
20;678.0;573.0;0.1;1.0;6064.0;5456.0;0.475;1.604;0.241;8.3;1.0;300;70000;18992000.0;18992000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
21;597.0;442.0;0.1;1.0;7975.0;10000.0;0.09;0.566;0.008;8.3;1.0;300;70000;18887000.0;18887000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
22;551.0;545.0;0.1;1.0;5360.0;7065.0;1.0;1.283;0.261;8.3;1.0;300;70000;19021000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
23;565.0;531.0;0.1;1.0;4644.0;7370.0;0.285;1.18;0.143;8.3;1.0;300;70000;19169000.0;19169000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
24;659.0;609.0;0.1;1.0;6691.0;6974.0;0.2;0.923;0.469;8.3;1.0;300;70000;18920000.0;18920000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
25;491.0;482.0;0.1;1.0;5732.0;7599.0;0.77;1.287;0.247;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
26;597.0;442.0;0.1;1.0;7975.0;10000.0;0.09;0.566;0.008;8.3;1.0;300;70000;18887000.0;18887000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
27;590.0;471.0;0.1;1.0;7687.0;8345.0;0.475;0.259;0.139;8.3;1.0;300;70000;18715000.0;18715000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
28;565.0;491.0;0.1;1.0;4522.0;6577.0;0.8;1.343;0.125;8.3;1.0;300;70000;18866000.0;18864000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
29;559.0;481.0;0.1;1.0;4488.0;7793.0;0.38;1.139;0.106;8.3;1.0;300;70000;19207000.0;19207000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
30;533.0;527.0;0.1;1.0;3886.0;4358.0;0.42;1.0;0.251;8.3;1.0;300;70000;18508000.0;18499000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
31;625.0;601.0;0.1;1.0;5210.0;5747.0;0.475;1.188;1.006;8.3;1.0;300;70000;18922000.0;18919000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
32;625.0;601.0;0.1;1.0;5210.0;5747.0;0.475;1.188;1.006;8.3;1.0;300;70000;18922000.0;18919000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
33;500.0;442.0;0.1;1.0;8359.0;10000.0;0.27;0.469;0.0;8.3;1.0;300;70000;17129000.0;17129000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
34;621.0;600.0;0.1;1.0;5314.0;5947.0;0.665;1.103;0.811;8.3;1.0;300;70000;18853000.0;18847000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
35;502.0;442.0;0.1;1.0;8135.0;10000.0;0.3;0.482;0.0;8.3;1.0;300;70000;16931000.0;16931000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
36;592.0;547.0;0.1;1.0;5819.0;7229.0;0.57;1.209;0.606;8.3;1.0;300;70000;18426000.0;18422000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
37;502.0;442.0;0.1;1.0;8888.0;10000.0;0.3;0.482;0.0;8.3;1.0;300;70000;17103000.0;17103000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
38;624.0;603.0;0.1;1.0;5341.0;5776.0;1.2;1.533;1.387;8.3;1.0;300;70000;18912000.0;18907000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
39;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
40;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
41;618.0;584.0;0.1;1.0;6104.0;6996.0;0.76;0.549;0.499;8.3;1.0;300;70000;19075000.0;19029000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
42;762.0;761.0;0.1;1.0;6216.0;6985.0;0.728;0.809;1.069;8.3;1.0;300;70000;19130000.0;18581000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
43;644.0;595.0;0.1;1.0;6667.0;6781.0;0.95;0.426;0.624;8.3;1.0;300;70000;19036000.0;18986000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
44;577.0;550.0;0.1;1.0;5307.0;6592.0;0.665;0.836;0.344;8.3;1.0;300;70000;19046000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
45;577.0;550.0;0.1;1.0;5307.0;6592.0;0.665;0.217;0.344;8.3;1.0;300;70000;19046000.0;19021000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
46;544.0;486.0;0.1;1.0;5101.0;7518.0;0.57;0.109;0.159;8.3;1.0;300;70000;19295000.0;19295000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
47;623.0;651.0;0.1;1.0;4102.0;5421.0;0.2;1.349;0.12;8.3;1.0;300;70000;18632000.0;18632000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
48;615.0;606.0;0.1;1.0;4545.0;4814.0;0.6;1.009;1.126;8.3;1.0;300;70000;18771000.0;18760000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
49;591.0;584.0;0.1;1.0;4441.0;4857.0;1.05;1.576;0.818;8.3;1.0;300;70000;19322000.0;19322000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
50;661.0;617.0;0.1;1.0;6252.0;7700.0;0.18;0.321;0.647;8.3;1.0;300;70000;17630000.0;17529000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
51;831.0;792.0;0.1;1.0;6758.0;7320.0;0.2;0.407;1.134;8.3;1.0;300;70000;19144000.0;18486000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
52;442.0;442.0;0.1;1.0;10000.0;10000.0;0.3;0.04;0.092;8.3;1.0;300;70000;16298000.0;16298000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
53;554.0;442.0;0.1;1.0;7236.0;10000.0;0.21;0.128;0.065;8.3;1.0;300;70000;17428000.0;17428000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
54;436.0;464.0;0.1;1.0;6435.0;8759.0;0.285;0.78;0.102;8.3;1.0;300;70000;18396000.0;18396000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
55;554.0;442.0;0.1;1.0;7236.0;10000.0;0.14;0.118;0.043;8.3;1.0;300;70000;17428000.0;17428000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
62;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
75;614.0;613.0;0.25;1.0;4287.0;5738.0;0.1;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
73;613.0;538.0;0.1;1.0;4357.0;6524.0;0.19;1.286;0.085;8.3;1.0;300;70000;18677000.0;18677000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
82;614.0;613.0;0.1;1.0;4287.0;5738.0;0.4;1.378;0.174;8.3;1.0;300;70000;18727000.0;18727000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
83;563.0;522.0;0.05;1.0;6099.0;7273.0;0.4;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
102;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
103;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
104;626.0;600.0;0.1;1.0;4325.0;5844.0;0.6;1.393;0.201;8.3;1.0;300;70000;18802000.0;18802000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
105;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
106;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
111;500.0;500.0;0.15;0.5;2400.0;5700.0;0;0.0;0;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
112;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
121;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
123;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
124;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
131;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
132;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
133;720.0;720.0;0.08;1;5544.0;4766.0;0;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
141;720.0;720.0;0.08;1;5544.0;4766.0;0.5;0.89;1.79;8.3;1.0;300.0;75590.0;2300000.0;1.6E7;2000.0;1100.0;600.0;0.3;2.5E-5;4.0;0.3
142;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
162;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
211;500.0;500.0;0.13;0.5;2400.0;5700.0;1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
212;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
213;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
221;500.0;500.0;0.13;0.5;2400.0;5700.0;0.5;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
222;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
223;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
231;500.0;500.0;0.13;0.5;2400.0;5700.0;2;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
241;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
242;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
243;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
244;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
255;563.0;522.0;0.1;1.0;6099.0;7273.0;0;0.764;0.352;8.3;1.0;300;70000;18169000.0;18167000.0;1800;1000;600;0.3;2.5e-05;4.0;0.3
311;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
312;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
313;500.0;500.0;0.13;0.5;2400.0;5700.0;1.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
321;500.0;500.0;0.13;0.5;2400.0;5700.0;0.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
322;500.0;500.0;0.13;0.5;2400.0;5700.0;2.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
323;500.0;500.0;0.08;0.4;6000.0;5000.0;2;0.4;1.8;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
324;500.0;500.0;0.14;0.5;2400.0;5700.0;1.6;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
331;500.0;500.0;1.6;2;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
332;500.0;500.0;10;10;2400.0;5700.0;0;10;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
333;500.0;500.0;2.13;2.5;2400.0;5700.0;0.6;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
334;500.0;500.0;2.13;2.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
335;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
411;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
412;500.0;500.0;0.13;0.5;2400.0;5700.0;0.1;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
421;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
422;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
423;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
511;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
512;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
521;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
522;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3
523;500.0;500.0;0.13;0.5;2400.0;5700.0;0;0.6;1.28;8.3;1.0;300.0;70000.0;2300000.0;1.5E7;1800.0;1000.0;600.0;0.3;2.5E-5;4.0;0.3)";
	


	parameters.insert(make_pair("caseDirectory", (getenv("PWD")==NULL?".":getenv("PWD"))));
	parameters.insert(make_pair("ForeFireDataDirectory", "ForeFire"));
	parameters.insert(make_pair("experiment", "ForeFire"));
	parameters.insert(make_pair("PPath", "parallel"));
	parameters.insert(make_pair("STDfarsiteFuelsTable", fuelsFarsiteCSV));
	parameters.insert(make_pair("STDlandIndexedFuelsTable", landCoverIndexedCSV));

	parameters.insert(make_pair("NetCDFfile", ""));
	parameters.insert(make_pair("fuelsTableFile", "fuels.ff"));
	parameters.insert(make_pair("paramsFile", "Params.ff"));
	parameters.insert(make_pair("parallelInit", "0"));
	parameters.insert(make_pair("InitFile", "Init.ff"));
	parameters.insert(make_pair("InitFiles", "output"));
	parameters.insert(make_pair("InitTime", "99999999999999"));

	parameters.insert(make_pair("LookAheadDistanceForeTimeGradientDataLayer", "40"));
	parameters.insert(make_pair("BMapsFiles", "1234567890"));

    parameters.insert(make_pair("ISOdate", "2012-01-01T00:00:00Z"));
    parameters.insert(make_pair("count", "0"));                      
    parameters.insert(make_pair("dumpMode", "ff")); 

	parameters.insert(make_pair("runmode", "standalone")); 
	parameters.insert(make_pair("MNHalt", "0")); 


	parameters.insert(make_pair("propagationModel", "Iso"));
	parameters.insert(make_pair("frontScanDistance", "1000"));
	parameters.insert(make_pair("burningTresholdFlux", "10"));
	parameters.insert(make_pair("normalScheme","medians"));
	parameters.insert(make_pair("curvatureComputation", "1"));
	parameters.insert(make_pair("curvatureScheme","circumradius"));
	parameters.insert(make_pair("frontDepthComputation", "0"));
	parameters.insert(make_pair("frontDepthScheme","normalDir"));
	parameters.insert(make_pair("minimalPropagativeFrontDepth","10."));
	parameters.insert(make_pair("maxFrontDepth","200."));
	parameters.insert(make_pair("initialFrontDepth", "20"));
	parameters.insert(make_pair("initialBurningDuration", "30"));
	parameters.insert(make_pair("bmapLayer","0"));
	parameters.insert(make_pair("minSpeed","0.005"));
	parameters.insert(make_pair("maxSpeed","10"));
	parameters.insert(make_pair("relax","0.5"));
	parameters.insert(make_pair("smoothing","1"));
	parameters.insert(make_pair("windReductionFactor","0.4"));
	parameters.insert(make_pair("defaultColormap","turbo"));
	parameters.insert(make_pair("atmoNX", "100"));
	parameters.insert(make_pair("atmoNY", "100"));
	parameters.insert(make_pair("atmoNZ", "20"));
	parameters.insert(make_pair("refLongitude", "0"));
	parameters.insert(make_pair("refLatitude", "0"));
    parameters.insert(make_pair("refYear", "0"));
    parameters.insert(make_pair("refDay", "0"));
    parameters.insert(make_pair("refTime", "0"));
	parameters.insert(make_pair("year", "2012"));
	parameters.insert(make_pair("month", "1"));
	parameters.insert(make_pair("day", "1"));
	parameters.insert(make_pair("DefaultSWLngLat","8.6192,41.765"));
	parameters.insert(make_pair("perimeterResolution", "40"));
	parameters.insert(make_pair("spatialIncrement", "2"));
	parameters.insert(make_pair("watchedProc", "-2"));
	parameters.insert(make_pair("CommandOutputs", "0"));
	parameters.insert(make_pair("FireDomainOutputs", "0"));
	parameters.insert(make_pair("FireFrontOutputs", "1"));
	parameters.insert(make_pair("FireNodeOutputs", "1"));
	parameters.insert(make_pair("FDCellsOutputs", "1"));
	parameters.insert(make_pair("HaloOutputs", "1"));
	parameters.insert(make_pair("propagationSpeedAdjustmentFactor", "1"));
	parameters.insert(make_pair("fireOutputDirectory", "."));
	parameters.insert(make_pair("atmoOutputDirectories", "."));
	parameters.insert(make_pair("outputFiles","output"));
	parameters.insert(make_pair("outputsUpdate","0"));
	parameters.insert(make_pair("debugFronts", "0"));
	parameters.insert(make_pair("surfaceOutputs","0"));
	parameters.insert(make_pair("bmapOutputUpdate","0"));
	parameters.insert(make_pair("numAtmoIterations","1000000"));
	parameters.insert(make_pair("numberOfAtmoStepPerParallelCom","1"));
	parameters.insert(make_pair("MNHExchangeScalarLayersNames","plumeTopHeight,plumeBottomHeight,smokeAtGround,tke"));
	
	parameters.insert(make_pair("plumeTopHeightRange","0,6000"));
	parameters.insert(make_pair("plumeBottomHeightange","0,6000"));
	parameters.insert(make_pair("smokeAtGroundRange","0,10"));
	parameters.insert(make_pair("tkeRange","0,10"));
	parameters.insert(make_pair("windURange","-10,10"));
	parameters.insert(make_pair("windVRange","-10,10"));
	

	parameters.insert(make_pair("updateBinStreamFrequency","10"));
	parameters.insert(make_pair("max_inner_front_nodes_filter","50"));
	parameters.insert(make_pair("heatFluxDefaultModel","heatFluxBasic"));
	parameters.insert(make_pair("vaporFluxDefaultModel","vaporFluxBasic"));
	parameters.insert(make_pair("SPOTF1DefaultModel","SpottingFluxBasic"));

}

SimulationParameters::~SimulationParameters() {
}
vector<string> SimulationParameters::getAllKeys() {
    vector<string> keys;
    for (map<string, string>::iterator it = parameters.begin(); it != parameters.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}
void SimulationParameters::setParameter(string key, string value, bool protect){
	//cout<<"setting "<<key<<" to "<<value<<endl;
	list<string>::iterator protection
		= find(GetInstance()->protectedParameters.begin(), GetInstance()->protectedParameters.end(), key);
	if ( protection == protectedParameters.end() ){
		map<string, string>::iterator param = GetInstance()->parameters.find(key);
		if ( param != GetInstance()->parameters.end() ) GetInstance()->parameters.erase(key);
		GetInstance()->parameters.insert(make_pair(key, value));
	}
	if ( protect ) GetInstance()->protectedParameters.push_back(key);
}

void SimulationParameters::setDouble(string key, double value){
	ostringstream oss;
	oss<<value;
	setParameter(key, oss.str());
}

void SimulationParameters::setInt(string key, int value){
	ostringstream oss;
	oss<<value;
	setParameter(key, oss.str());
}

void SimulationParameters::setSize(string key, size_t value){
	ostringstream oss;
	oss<<value;
	setParameter(key, oss.str());
}

bool SimulationParameters::isValued(string key){
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() and param->second != undefined ) return true;
	return false;
}

string SimulationParameters::getParameter(string key){

	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) return param->second;
	return undefined;
}

double SimulationParameters::getDouble(string key){
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) {
		double val;
		istringstream iss(param->second);
		if ( iss >> val ) return val;
	}
	return doubleUndefined;
}

int SimulationParameters::getInt(string key){
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) {
		istringstream iss(param->second);
		int val;
		if ( iss >> val ) return val;
	}
	return intUndefined;
}

size_t SimulationParameters::getSize(string key){
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) {
		istringstream iss(param->second);
		size_t val;
		if ( iss >> val ) return val;
	}
	return sizeUndefined;
}

vector<string> SimulationParameters::getParameterArray(string key){
	vector<string> vals;
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) tokenizeToString(param->second, vals, ",");
	return vals;
}

vector<double> SimulationParameters::getDoubleArray(string key){
	vector<double> vals;
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) tokenizeToDouble(param->second, vals, ",");
	return vals;
}

vector<int> SimulationParameters::getIntArray(string key){
	vector<int> vals;
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) tokenizeToInt(param->second, vals, ",");
	return vals;
}

vector<size_t> SimulationParameters::getSizeArray(string key){
	vector<size_t> vals;
	map<string, string>::iterator param = GetInstance()->parameters.find(key);
	if ( param != GetInstance()->parameters.end() ) tokenizeToSize(param->second, vals, ",");
	return vals;
}

void SimulationParameters::tokenizeToString(const string& str
		, vector<string>& vals, const string& delimiter = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiter, 0);
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of(delimiter, lastPos);

	while (string::npos != pos || string::npos != lastPos){
		// Found a token, add it to the vector.
		vals.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiter, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiter, lastPos);
	}
}

void SimulationParameters::tokenizeToDouble(const string& str
		, vector<double>& vals, const string& delimiter = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiter, 0);
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of(delimiter, lastPos);

	double val;
	while (string::npos != pos || string::npos != lastPos){
		// Found a token, add it to the vector.
		istringstream iss(str.substr(lastPos, pos - lastPos));
		if ( iss >> val ) vals.push_back(val);
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiter, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiter, lastPos);
	}
}

void SimulationParameters::tokenizeToInt(const string& str
		, vector<int>& vals, const string& delimiter = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiter, 0);
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of(delimiter, lastPos);

	int val;
	while (string::npos != pos || string::npos != lastPos){
		// Found a token, add it to the vector.
		istringstream iss(str.substr(lastPos, pos - lastPos));
		if ( iss >> val ) vals.push_back(val);
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiter, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiter, lastPos);
	}
}

void SimulationParameters::tokenizeToSize(const string& str
		, vector<size_t>& vals, const string& delimiter = " ") {
	// Skip delimiters at beginning.
	string::size_type lastPos = str.find_first_not_of(delimiter, 0);
	// Find first "non-delimiter".
	string::size_type pos     = str.find_first_of(delimiter, lastPos);

	size_t val;
	while (string::npos != pos || string::npos != lastPos){
		// Found a token, add it to the vector.
		istringstream iss(str.substr(lastPos, pos - lastPos));
		if ( iss >> val ) vals.push_back(val);
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiter, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiter, lastPos);
	}
}



// Helper: convert degrees to radians.
inline double deg2rad(double deg) {
    return deg * M_PI / 180.0;
}

// Helper: convert radians to degrees.
inline double rad2deg(double rad) {
    return rad * 180.0 / M_PI;
}

// Converts longitude/latitude (in degrees) to UTM coordinates.
// Returns a vector: {UTM_Easting, UTM_Northing}
std::vector<double> SimulationParameters::lonlat2UTM(double lon, double lat, int utmzone, bool isNorth) {
    // Convert input angles to radians.
	// Constants for WGS84
	const double a = 6378137.0;             // semi-major axis
	const double f = 1 / 298.257223563;       // flattening
	const double b = a * (1 - f);             // semi-minor axis
	const double e2 = (a*a - b*b) / (a*a);    // square of eccentricity
	const double k0 = 0.9996;                 // scale factor

    double lat_rad = deg2rad(lat);
    double lon_rad = deg2rad(lon);
    
    // Central meridian for the UTM zone.
    double lambda0 = deg2rad((utmzone - 1) * 6 - 180 + 3);
    
    // Precalculate trigonometric functions.
    double sinLat = sin(lat_rad);
    double cosLat = cos(lat_rad);
    
    // Radius of curvature in the prime vertical.
    double N = a / sqrt(1 - e2 * sinLat * sinLat);
    
    double T = tan(lat_rad) * tan(lat_rad);
    double C = ((a*a - b*b) / (b*b)) * cosLat * cosLat; // second eccentricity squared times cos²(lat)
    double A = cosLat * (lon_rad - lambda0);
    
    // Compute the meridional arc.
    double M = a * ((1 - e2/4 - 3*e2*e2/64 - 5*e2*e2*e2/256) * lat_rad
             - (3*e2/8 + 3*e2*e2/32 + 45*e2*e2*e2/1024) * sin(2 * lat_rad)
             + (15*e2*e2/256 + 45*e2*e2*e2/1024) * sin(4 * lat_rad)
             - (35*e2*e2*e2/3072) * sin(6 * lat_rad));
    
    // Calculate UTM Easting.
    double x = k0 * N * (A + (1 - T + C) * A*A*A / 6 +
                (5 - 18*T + T*T + 72*C - 58 * ((a*a - b*b) / (b*b))) * A*A*A*A*A / 120)
                + 500000.0;
    
    // Calculate UTM Northing.
    double y = k0 * (M + N * tan(lat_rad) * (A*A/2 + (5 - T + 9*C + 4*C*C) * A*A*A*A / 24 +
                (61 - 58*T + T*T + 600*C - 330 * ((a*a - b*b) / (b*b))) * A*A*A*A*A*A / 720));
    
    // Adjust for southern hemisphere.
    if (!isNorth) {
        y += 10000000.0;
    }
    
    return { x, y };
}

// Converts UTM coordinates to longitude/latitude (in degrees).
// Returns a vector: {longitude, latitude}
std::vector<double> SimulationParameters::UTM2lonlat(double x, double y, int utmzone, bool isNorth) {
    // Remove false easting.
	// Constants for WGS84
	const double a = 6378137.0;             // semi-major axis
	const double f = 1 / 298.257223563;       // flattening
	const double b = a * (1 - f);             // semi-minor axis
	const double e2 = (a*a - b*b) / (a*a);    // square of eccentricity
	const double k0 = 0.9996;                 // scale factor

    double x_adj = x - 500000.0;
    
    // Remove false northing if in southern hemisphere.
    if (!isNorth) {
        y -= 10000000.0;
    }
    
    // Central meridian for the UTM zone.
    double lambda0 = deg2rad((utmzone - 1) * 6 - 180 + 3);
    
    // Calculate the footpoint latitude.
    double M = y / k0;
    double mu = M / (a * (1 - e2/4 - 3*e2*e2/64 - 5*e2*e2*e2/256));
    
    // Calculate e1 (first eccentricity coefficient for the footpoint latitude).
    double e1 = (1 - sqrt(1 - e2)) / (1 + sqrt(1 - e2));
    
    // Compute the footprint latitude (phi1) using the series expansion.
    double phi1 = mu + (3*e1/2 - 27*e1*e1*e1/32) * sin(2 * mu)
                  + (21*e1*e1/16 - 55*e1*e1*e1*e1/32) * sin(4 * mu)
                  + (151*e1*e1*e1/96) * sin(6 * mu)
                  + (1097*e1*e1*e1*e1/512) * sin(8 * mu);
    
    double sinPhi1 = sin(phi1);
    double cosPhi1 = cos(phi1);
    
    // Radius of curvature in the prime vertical at phi1.
    double N1 = a / sqrt(1 - e2 * sinPhi1 * sinPhi1);
    double T1 = tan(phi1) * tan(phi1);
    double C1 = ((a*a - b*b) / (b*b)) * cosPhi1 * cosPhi1;
    double D = x_adj / (N1 * k0);
    
    // Calculate latitude (in radians).
    double lat_rad = phi1 - (N1 * tan(phi1) / a) *
            (D*D/2 - (5 + 3*T1 + 10*C1 - 4*C1*C1 - 9 * ((a*a - b*b)/(b*b))) * D*D*D*D/24
             + (61 + 90*T1 + 298*C1 + 45*T1*T1 - 252 * ((a*a - b*b)/(b*b)) - 3*C1*C1) * D*D*D*D*D*D/720);
    
    // Calculate longitude (in radians).
    double lon_rad = lambda0 + (D - (1 + 2*T1 + C1) * D*D*D/6
                      + (5 - 2*C1 + 28*T1 - 3*C1*C1 + 8 * ((a*a - b*b)/(b*b)) + 24*T1*T1) * D*D*D*D*D/120) / cosPhi1;
    
    // Convert the results to degrees.
    double lat = rad2deg(lat_rad);
    double lon = rad2deg(lon_rad);
    
    return { lon, lat };
	/*    double lon = 12.4924; // longitude in degrees
    double lat = 41.8902; // latitude in degrees (approx. location of the Colosseum in Rome)
    int utmzone = 33;     // appropriate UTM zone for Rome
    bool isNorth = true;  // northern hemisphere
    
    std::vector<double> utm = lonlat2UTM(lon, lat, utmzone, isNorth);
    std::cout << "UTM Easting: " << utm[0] << "\nUTM Northing: " << utm[1] << "\n";
    
    std::vector<double> lonlat = UTM2lonlat(utm[0], utm[1], utmzone, isNorth);
    std::cout << "Longitude: " << lonlat[0] << "\nLatitude: " << lonlat[1] << "\n";
    
    */
}



}
