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
