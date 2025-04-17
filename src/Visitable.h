/**
 * @file Visitable.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef VISITABLE_H_
#define VISITABLE_H_

#include "include/Futils.h"

using namespace std;

namespace libforefire{

/*! \class Visitable
 * \brief Abstract class for visitable objects
 *
 *  The 'Visitable' abstract class provides
 *  the 'accept' pure abstract function for
 *  'Visitable' objects of the simulation.
 */
class Visitable {
public:
	/*! \brief Default Contructor */
	Visitable(){}
	/*! \brief Destructor */
	virtual ~Visitable(){};

	/*! \brief Accept function for the desired visitor (virtual) */
	virtual void accept(class Visitor *) = 0;

};

}

#endif /* VISITABLE_H_ */
