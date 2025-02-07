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

#ifndef CLIBFOREFIRE_H_
#define CLIBFOREFIRE_H_

#include "Command.h"
#include "FFArrays.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire {



#ifdef __cplusplus
extern "C" {
#endif

// Function for control of the Fire Simulation

void MNHInit(const double);
void MNHCreateDomain(const int
		, const int, const int
		, const int, const double
		, const double, const double
		, const int, const double*
		, const int, const double*
		, const int, const double*
		, const double);
void CheckLayer(const char*);
void MNHStep(double);
void MNHGoTo(double);
void executeMNHCommand(const char*);

// Functions for communicating with MNH
void FFPutString(const char*, char*);
void FFGetString(const char*, const char*);

void FFPutInt(const char*, int*);
void FFGetInt(const char*, int*);

void FFPutIntArray(const char*, int*, size_t, size_t);
void FFGetIntArray(const char*, double, int*, size_t, size_t);

void FFPutDouble(const char*, double*);
void FFGetDouble(const char*, double*);

void FFPutDoubleArray(const char*, double*, size_t, size_t);
void FFGetDoubleArray(const char*, double, double*, size_t, size_t);

void FFDumpDoubleArray(size_t, size_t
		, const char* , double , double*
		, size_t ,size_t ,size_t ,size_t , size_t );



Command* getLauncher();

#ifdef __cplusplus
}
#endif

}

#endif /* CLIBFOREFIRE_H_ */
