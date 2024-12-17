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

#ifndef FROMOBSMODEL_H_
#define FROMOBSMODEL_H_

#include "FireDomain.h"

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
