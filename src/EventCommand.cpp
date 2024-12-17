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

