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

#ifndef TIMETABLE_H_
#define TIMETABLE_H_

#include "ForeFireAtom.h"
#include "FFEvent.h"
#include "include/FFConstants.h"

using namespace std;

namespace libforefire {

/*! \class TimeTable
 * \brief Class providing control over the succession of 'FFEvents'
 *
 *  The 'TimeTable' class provides an efficient way to deal
 *  with the succession of events occurring the simulation,
 *  i.e. 'FFEvents'. The timetable points to the 'head' event,
 *  which in turn points to the next one, etc... A 'rbin'
 *  is provided to trash events.
 */
class TimeTable {

	FFEvent* head; /*!< Next event to happen */
	FFEvent* rbin; /*!< Bin for thrashing events */

	size_t size(); /*!< Number of events in the timetable */
	size_t incr; /*!< Total number of inserted events */
	void increment();
	size_t decr; /*!< Total number of deleted events */
	void decrement();


	void commonInitialization();
public:
	/*! \brief Default constructor */
	TimeTable();
	/*! \brief Constructor providing one 'FFEvent' */
	TimeTable(FFEvent*);
	/*! \brief Default destructor */
	virtual ~TimeTable();

	/*! \brief Mutator of the 'head' of the timetable */
	void setHead(FFEvent*);
	friend void FFEvent::setNext(FFEvent*);
	friend void FFEvent::setPrev(FFEvent*);
	/*! \brief Accessor to the next event */
	FFEvent* getUpcomingEvent();
	/*! \brief Accessor to the head */
	FFEvent* getHead();
	/*! \brief Inserting a new event before the head */
	void insertBefore(FFEvent*);
	/*! \brief Inserting a new event according to its time of activation */
	void insert(FFEvent*);
	/*! \brief Removing an event */
	void dropEvent(FFEvent*);
	/*! \brief Removing all the events associated to a ForeFireAtom */
	void dropAtomEvents(ForeFireAtom*);

	/*! \brief Getting the current time of the timetable */
	double getTime();

	/*! \brief printing the timetable */
	string print();
};

}

#endif /* TIMETABLE_H_ */
