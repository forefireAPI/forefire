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

#ifndef VISITOR_H_
#define VISITOR_H_

#include "ForeFireAtom.h"
#include "FireDomain.h"
#include "FireFront.h"
#include "FireNode.h"

using namespace std;

namespace libforefire{

/*! \class Visitor
 * \brief Abstract class for visitors
 *
 *  The 'Visitor' abstract class conforms to the
 *  Visitor pattern to obtain information on the
 *  simulation through external objects. Visitors
 *  in LibForeFire are also 'ForeFireAtom' objects
 *  so they can be called by the simulator, i.e.
 *  an event encapsulates the visitor and can be
 *  called at different times of the simulation.
 */
class Visitor: public ForeFireAtom {
public:
	/*! \brief Default constructor */
	Visitor() : ForeFireAtom(0.) {};
	/*! \brief Default destructor */
	virtual ~Visitor(){};

	/*! \brief Visit function for the 'FireDomain' objects */
	virtual void visit(FireDomain*)=0;
	virtual void postVisitInner(FireDomain*)=0;
	virtual void postVisitAll(FireDomain*)=0;
	
	/*! \brief Visit function for the 'FireFront' objects */
	virtual void visit(FireFront*)=0;
	virtual void postVisitInner(FireFront*)=0;
	virtual void postVisitAll(FireFront*)=0;
	
	/*! \brief Visit function for the 'FireNode' objects */
	virtual void visit(FireNode*)=0;

	/*! \brief Visit function for the 'FireNode' objects */
	virtual void increaseLevel()=0;
	virtual void decreaseLevel()=0;
	virtual size_t getLevel()=0;
};

}

#endif /* VISITOR_H_ */
