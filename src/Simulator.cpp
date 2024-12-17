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

#include "Simulator.h"

namespace libforefire {

Simulator::Simulator() {
	outputs = false;
}

Simulator::Simulator(TimeTable* tt, bool outs) : schedule(tt) {
	outputs = outs;
}

Simulator::~Simulator() {
}

void Simulator::setTimeTable(TimeTable* tt){
	schedule = tt;
}

TimeTable* Simulator::getSchedule(){
	return schedule;
}

void Simulator::goTo(const double& endTime){
	while ( schedule->getTime() <= endTime + EPSILONT ) {
		FFEvent* upEvent = schedule->getUpcomingEvent();
		if ( !upEvent ){
		   cout << "no more events !!"<<endl;
		   return;
		}

		// Treating the desired actions on the Atom
		// Possible inputs
		if ( upEvent->input ) upEvent->getAtom()->input();

		// Update the event, i.e. update the properties
		upEvent->getAtom()->update();

		// Advance the event in time, i.e. calculate new properties at next time
		upEvent->getAtom()->timeAdvance();

		// Possible outputs
		if ( upEvent->output ) upEvent->getAtom()->output();

		// Re-locate the event in the schedule
		upEvent->setNewTime(upEvent->getAtom()->getUpdateTime());
		schedule->insert(upEvent);

	}
}



}
