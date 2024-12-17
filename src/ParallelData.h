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

#ifndef PARALLELDATA_H_
#define PARALLELDATA_H_

#include "FFArrays.h"

namespace libforefire {

class ParallelData {

public:

	FFArray<double>* FireNodesInCellPosX; /*!< Matrix for communications of the the positions x */
	FFArray<double>* FireNodesInCellPosY; /*!< Matrix for communications of the positions y */
	FFArray<double>* FireNodesInCellVelX; /*!< Matrix for communications of the velocities vx */
	FFArray<double>* FireNodesInCellVelY; /*!< Matrix for communications of the velocities vy */
	FFArray<double>* FireNodesInCellTime; /*!< Matrix for communications of the update times */
	FFArray<double>* FireNodesInCellId; /*!< Matrix for communications of id */

	ParallelData();
	virtual ~ParallelData();

	void setSize(const size_t&, const size_t&
			, const size_t&, const double&);
};

}

#endif /* PARALLELDATA_H_ */
