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

	/* These were statics, shared by every simulation in the process: two
	 * concurrent print[] calls interleaved into one buffer. */
	size_t currentLevel = 0;
	bool firstGeoFeature = true; /*!< first feature of the GeoJSON list */
	std::vector< std::vector<std::string> > geojson_current_feature;

	double updateStep;

public:

	/*! \brief buffer the representation is built into
	 *
	 * Was static, so two simulations printing at once interleaved into one
	 * buffer. Command::dumpString reads it, hence public. */
	ostringstream outputstr;

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
