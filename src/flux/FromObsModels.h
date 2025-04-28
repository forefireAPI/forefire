/**
 * @file FromObsModels.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef FROMOBSMODEL_H_
#define FROMOBSMODEL_H_

#include "../FireDomain.h"

using namespace std;

namespace libforefire {

struct SensibleheatFlux{
    double flaming;
    double smoldering; 

    SensibleheatFlux(double f, double s) : flaming(f), smoldering(s) {}
};

/*! \compute heat flux from local input */
SensibleheatFlux computeHeatFLuxFromBmap(const double&, const double&, const double&, const double&, const double&, const double&, const double&);

} /* namespace libforefire */

#endif /* FROMOBSMODEL_H_ */
