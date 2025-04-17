/**
 * @file EventCommand.h
 * @brief  Definitiosn for the class that defines hos to send a specific command to the interpreter at a scheduled time (an time Atom that can be schedueled)
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef EVENTCOMMAND_H_
#define EVENTCOMMAND_H_

#include "Command.h"
#include "include/Futils.h"
#include "ForeFireAtom.h"

using namespace std;

namespace libforefire{

/*! \class Visitor
 * \brief Abstract class for visitors
 *
 *  The 'Visitor' abstract class conforms to the
 *  Visitor pattern to obtain information on the
 *  simulation through external objects. Visitors
 *  in LibForeFire are also 'ForeFireAtom' objects
 *  so they can be called by the simulator, i.e.
 *  an event encapsulates the visitor and can be
 *  called at different times of the simulation.
 */
class EventCommand: public ForeFireAtom {

	string schedueledCommand;

public:
	/*! \brief Default constructor */
	EventCommand() : ForeFireAtom(0.) {};
	/*! \brief standard constructor */
	EventCommand( string, double ) ;
	/*! \brief Default destructor */
	~EventCommand();

	/* making the 'update()', 'timeAdvance()' and 'accept()'
	 * virtual functions of 'ForeFireAtom' not virtual */

	void input();
	void update();
	void timeAdvance();
	void output();

	string  toString();
};

}

#endif /* EVENTCOMMAND_H_ */
