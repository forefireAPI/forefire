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
