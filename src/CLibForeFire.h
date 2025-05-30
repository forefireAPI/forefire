/**
 * @file CLibForeFire.h
 * @brief Definitions for Bindings to C and Fortran as well as MPI processing if activated (requires a coupler model)
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
