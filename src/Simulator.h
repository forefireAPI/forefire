/**
 * @file Simulator.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
