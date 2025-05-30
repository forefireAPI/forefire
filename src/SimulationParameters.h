/**
 * @file SimulationParameters.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef SIMULATIONPARAMETERS_H_
#define SIMULATIONPARAMETERS_H_

#include "FFPoint.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire {

class SimulationParameters {

    static SimulationParameters* instance; /*!< Singleton-type class */

    /*! values for undefined parameters */
    static string undefined;
	static double doubleUndefined;
	static int intUndefined;
	static size_t sizeUndefined;

	/* key-value hash table */
    /*----------------------*/
	map<string, string> parameters;

	/*! list of parameters which value should not be modified */
	list<string> protectedParameters;

	SimulationParameters();

	SimulationParameters( const SimulationParameters & );
	SimulationParameters & operator =( const SimulationParameters & );

	void tokenizeToString(const string&, vector<string>&, const string&);
	void tokenizeToDouble(const string&, vector<double>&, const string&);
	void tokenizeToInt(const string&, vector<int>&, const string&);
	void tokenizeToSize(const string&, vector<size_t>&, const string&);

public:

	virtual ~SimulationParameters();

	void setParameter(string, string, bool = false);
	void setDouble(string, double);
	void setInt(string, int);
	void setSize(string, size_t);
	vector<string> getAllKeys();
	bool isValued(string);

	string getParameter(string);
	double getDouble(string);
	int getInt(string);
	size_t getSize(string);
	vector<string> getParameterArray(string);
	vector<double> getDoubleArray(string);
	vector<int> getIntArray(string);
	vector<size_t> getSizeArray(string);

	static SimulationParameters* GetInstance();
    
    /*! returns an ISO date string from secs, year and day of the year */
    static string FormatISODate(double secs, int year, int yday);
    
    /*! store into secs, year and yday respectively secs, year and day of the year from an ISO date string */
    static bool ISODateDecomposition(string date, double &secs, int &year, int &yday);
    
    /*! returns the number of seconds between two dates */
    static double SecsBetween(double t1, int y1, int yday1, double t2, int y2, int yday2);
    
    /*! returns the correct absolute path from a relative or absolute path */
    static string GetPath(string arg);

	static std::vector<double> UTM2lonlat(double x, double y, int utmzone, bool isNorth) ;
	static std::vector<double> lonlat2UTM(double lon, double lat, int utmzone, bool isNorth) ;
};

}

#endif /* SIMULATIONPARAMETERS_H_ */
