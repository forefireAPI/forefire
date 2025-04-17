/**
 * @file StringRepresentation.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef STRINGREPRESENTATION_H_
#define STRINGREPRESENTATION_H_

#include "Visitor.h"
#include "SimulationParameters.h"
#include "include/Futils.h"

namespace libforefire {

class StringRepresentation: public Visitor {

	FireDomain* domain;
	static size_t currentLevel;

	double updateStep;

public:

	static ostringstream outputstr;

/*	StringRepresentation();*/
	StringRepresentation(FireDomain*);
	virtual ~StringRepresentation();

	/* making the 'update()', 'timeAdvance()' and 'accept()'
	 * virtual functions of 'ForeFireAtom' not virtual */
	void input();
	void update();
	void timeAdvance();
	void output();

	size_t getLevel();

	/* Visitors of the elements */
	void visit(FireDomain*);
	void postVisitInner(FireDomain*);
	void postVisitAll(FireDomain*);
	
	void visit(FireFront*);
	void postVisitInner(FireFront*);
	void postVisitAll(FireFront*);
	
	void visit(FireNode*);


	void setOutPattern(string);

	void increaseLevel();
	void decreaseLevel();

	string dumpStringRepresentation();

	string toString();
    
	string outPattern;
    int dumpMode;
    int lastLevel;
};

}

#endif /* STRINGREPRESENTATION_H_ */
