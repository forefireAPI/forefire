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

#ifndef FFEVENT_H_
#define FFEVENT_H_

#include "ForeFireAtom.h"

namespace libforefire {

class FFEvent {
	double eventTime;
	ForeFireAtom* atom;
	FFEvent* next;
	FFEvent* prev;
public:

	// type of the event
	bool input, output;

	// constructors and destructors
	FFEvent();
	FFEvent(ForeFireAtom*);
	FFEvent(ForeFireAtom*, const string&);
	FFEvent(ForeFireAtom*, const double&, const string&);
	FFEvent(const FFEvent&);
	virtual ~FFEvent();

	/*!  \brief overloaded operator ==  */
	friend int operator==(const FFEvent&, const FFEvent&);
	/*!  \brief overloaded operator !=  */
	friend int operator!=(const FFEvent&, const FFEvent&);

	double getTime() const;
	ForeFireAtom* getAtom();
	FFEvent* getNext();
	FFEvent* getPrev();

	void setNewTime(double);
	void setNext(FFEvent*);
	void setPrev(FFEvent*);

	void insertBefore(FFEvent*);
	void insertAfter(FFEvent*);

	void makeInput(FFEvent*);
	void makeOutput(FFEvent*);

	string toString();
};

}

#endif /* FFEVENT_H_ */
