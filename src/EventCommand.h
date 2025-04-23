/**
 * @file EventCommand.h
 * @brief  Definitions for the class that defines hos to send a specific command to the interpreter at a scheduled time (an time Atom that can be schedueled)
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

/*! \class EventCommand
 * \brief TODO
 *
 *  Detail
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
