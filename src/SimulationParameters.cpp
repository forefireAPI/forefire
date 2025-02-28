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

double SimulationParameters::SecsBetween(double t1, int y1, int yday1, double t2, int y2, int yday2)
{
    y1 -= 1900;
    yday1--;
    int tt1 = t1 + yday1*86400 + (y1-70)*31536000 + ((y1-69)/4)*86400 - ((y1-1)/100)*86400 + ((y1+299)/400)*86400;
    time_t rawtime1 = (time_t) tt1;
    
    y2 -= 1900;
    yday2--;
    int tt2 = t2 + yday2*86400 + (y2-70)*31536000 + ((y2-69)/4)*86400 - ((y2-1)/100)*86400 + ((y2+299)/400)*86400;
    time_t rawtime2 = (time_t) tt2;
   // cout << "t1 "<< t1<< "t2  "<< t2<< " y1 "<< y1<< " y2 "<< y2 << " yday 1 "<<  yday1<< " yday 2 "<<  yday2 << rawtime1 << " yo "<<rawtime2 <<endl;
    return fabs(difftime(rawtime1, rawtime2));
}

string SimulationParameters::GetPath(string arg)
{
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

	parameters.insert(make_pair("caseDirectory", (getenv("PWD")==NULL?".":getenv("PWD"))));
	parameters.insert(make_pair("ForeFireDataDirectory", "ForeFire"));
	parameters.insert(make_pair("experiment", "ForeFire"));
	parameters.insert(make_pair("PPath", "parallel"));

	parameters.insert(make_pair("NetCDFfile", "data.nc"));
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
	parameters.insert(make_pair("spatialCFLmax", "0.3"));
	parameters.insert(make_pair("spatialIncrement", "2"));
	parameters.insert(make_pair("spatialCFLmax", "0.3"));
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
	parameters.insert(make_pair("max_inner_front_nodes_filter","50"));
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

}
