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

#ifndef ATMOSPHERICDATA_H_
#define ATMOSPHERICDATA_H_

#include "FFArrays.h"

namespace libforefire {

/*! \class AtmosphericData
 * \brief Storage for all atmospheric data in fire/atmospheric coupling
 *
 *  Atmospheric data is a container for all the atmospheric variables
 *  needed in a coupled fire/atmosphere simulation
 */
class AtmosphericData {
public:
	/*! \brief Default constructor */
	AtmosphericData();
	/*! \brief Destructor */
	virtual ~AtmosphericData();

	// Information to be provided by the atmospheric model
	double currentTime; /*!< time at the start of the atmospheric time-step */
	FFArray<double>* windU; /*!< Surface wind field, U component (start of the time step) */
	FFArray<double>* windV; /*!< Surface wind field, V component (start of the time step) */

	double oldTime; /*!< time at the end of the atmospheric time-step */
	FFArray<double>* oldWindU; /*!< Surface wind field, U component (end of the time step) */
	FFArray<double>* oldWindV; /*!< Surface wind field, V component (end of the time step) */

	FFArray<double>* topography; /*!< Topography provided by the atmospheric model for real cases */

	/*! \brief Setting all the size parameters according to atmospheric simulation */
	void setSize(const size_t&, const size_t&);

	/*! \brief Getting all the size parameters */
	size_t getSize();
	size_t getSizeX();
	size_t getSizeY();

};

}

#endif /* ATMOSPHERICDATA_H_ */
