/**
 * @file ParallelData.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
