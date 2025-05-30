/**
 * @file Simulator.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
	schedule->clear();
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
