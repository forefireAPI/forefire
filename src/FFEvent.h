/**
 * @file FFEvent.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
