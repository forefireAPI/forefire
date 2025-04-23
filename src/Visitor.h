/**
 * @file Visitor.h
 * @brief Defines the Visitor abstract base class for implementing the Visitor design pattern on ForeFire objects.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
