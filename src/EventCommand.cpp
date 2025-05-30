/**
 * @file EventCommand.cpp
 * @brief A class that defines hos to send a specific command to the interpreter at a scheduled time (an time Atom that can be schedueled)
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "EventCommand.h"
#include "Command.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire{




EventCommand::EventCommand(string scommand, double schedueledTime) : ForeFireAtom(schedueledTime) {
	schedueledCommand = scommand;

};

EventCommand::~EventCommand() {};

/* making the 'update()', 'timeAdvance()' and 'accept()'
 * virtual functions of 'ForeFireAtom' not virtual */


void EventCommand::update(){
	Command::ExecuteCommand(schedueledCommand);
}
void EventCommand::timeAdvance(){
	setUpdateTime(numeric_limits<double>::infinity());
}
void EventCommand::input(){};

void EventCommand::output(){};
string  EventCommand::toString(){
	ostringstream oss;

	oss<<schedueledCommand<<"@"<<getTime();
	return		oss.str();
}
};

