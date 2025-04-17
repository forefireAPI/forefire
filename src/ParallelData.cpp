/**
 * @file ParallelData.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "ParallelData.h"

namespace libforefire {

ParallelData::ParallelData() {
	FireNodesInCellPosX = 0;
	FireNodesInCellPosY = 0;
	FireNodesInCellVelX = 0;
	FireNodesInCellVelY = 0;
	FireNodesInCellTime = 0;
	FireNodesInCellId = 0;
}

ParallelData::~ParallelData() {
	// nothing to do
}

void ParallelData::setSize(
		const size_t& nx, const size_t& ny
		, const size_t& nz, const double& initval){
	if ( FireNodesInCellPosX ) delete FireNodesInCellPosX;
	FireNodesInCellPosX = new FFArray<double>("FireNodesInCellPosX", 0., nx, ny, nz);
	if ( FireNodesInCellPosY ) delete FireNodesInCellPosY;
	FireNodesInCellPosY = new FFArray<double>("FireNodesInCellPosY", 0., nx, ny, nz);
	if ( FireNodesInCellVelX ) delete FireNodesInCellVelX;
	FireNodesInCellVelX = new FFArray<double>("FireNodesInCellVelX", 0., nx, ny, nz);
	if ( FireNodesInCellVelY ) delete FireNodesInCellVelY;
	FireNodesInCellVelY = new FFArray<double>("FireNodesInCellVelY", 0., nx, ny, nz);
	if ( FireNodesInCellTime ) delete FireNodesInCellTime;
	FireNodesInCellTime = new FFArray<double>("FireNodesInCellTime", initval, nx, ny, nz);
	if ( FireNodesInCellId ) delete FireNodesInCellId;
	FireNodesInCellId = new FFArray<double>("FireNodesInCellId", 0., nx, ny, nz);
}

}
