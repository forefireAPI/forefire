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
};

}

#endif /* SIMULATIONPARAMETERS_H_ */
