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
	void visit(FireFront*);
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
