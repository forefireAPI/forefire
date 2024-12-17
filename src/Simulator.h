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

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "TimeTable.h"
#include "FFEvent.h"
#include "include/FFConstants.h"
#include "SimulationParameters.h"
#include "include/Futils.h"

namespace libforefire {

/*! \class Simulator
 * \brief Class providing control over the simulation
 *
 *  The 'Simulator' class provides methods in order
 *  to search through a 'schedule' ('TimeTable' object)
 *  and treating events of the simulation one after
 *  the other
 */
class Simulator {

	TimeTable* schedule; /*!< schedule of the events of the simulation */

	bool outputs; /*!< boolean for ouputs */

public:

	/*! \brief Default constructor */
	Simulator();
	/*! \brief Constructor provided a 'TimeTable' */
	Simulator(TimeTable*, bool = false);
	/*! \brief Default destructor */
	virtual ~Simulator();

	/*! \brief Advancing the simulation of the prescribed step */
	void goTo(const double&);

	TimeTable* getSchedule();
	void setTimeTable(TimeTable*);

	/*! \brief treating the next event and updating the 'schedule' */
	void treatNextEvent();
};

}

#endif /* SIMULATOR_H_ */
