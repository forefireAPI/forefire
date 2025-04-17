/**
 * @file AtmosphericData.cpp
 * @brief Class for data exchange in atmospheric coupling
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "AtmosphericData.h"

namespace libforefire {

AtmosphericData::AtmosphericData() {
	windU = 0;
	oldWindU = 0;
	windV = 0;
	oldWindV = 0;
	topography = 0;
	oldTime = 0;
	currentTime=0;
	plumeTopHeight= 0; 
	plumeBottomHeight= 0; 
	smokeAtGround= 0; 
	tke = 0; 
}

AtmosphericData::~AtmosphericData() {
	// nothing to do
}

void AtmosphericData::setSize(const size_t& nx, const size_t& ny){
	if ( windU ) delete windU;
	windU = new FFArray<double>("WindU", 0., nx+2, ny+2);
	if ( oldWindU ) delete oldWindU;
	oldWindU = new FFArray<double>("OldWindU", 0., nx+2, ny+2);
	if ( windV ) delete windV;
	windV = new FFArray<double>("WindV", 0., nx+2, ny+2);
	if ( oldWindV ) delete oldWindV;
	oldWindV = new FFArray<double>("OldWindV", 0., nx+2, ny+2);
	if ( topography ) delete topography;
	topography = new FFArray<double>("Topography", 0., nx, ny);
	
	if ( plumeTopHeight ) delete plumeTopHeight;
	plumeTopHeight = new FFArray<double>("plumeTopHeight", 0., nx+2, ny+2);
	if ( plumeBottomHeight ) delete plumeBottomHeight;
	plumeBottomHeight = new FFArray<double>("plumeBottomHeight", 0., nx+2, ny+2);
	if ( smokeAtGround ) delete smokeAtGround;
	smokeAtGround = new FFArray<double>("smokeAtGround", 0.,nx+2, ny+2);
	if ( tke ) delete tke;
	tke = new FFArray<double>("tke", 0., nx+2, ny+2);

	//if ( oldplumeTopHeight ) delete oldplumeTopHeight;
	oldplumeTopHeight = new FFArray<double>("oldplumeTopHeight", 0.,  nx+2, ny+2);
//	if ( oldplumeBottomHeight ) delete oldplumeBottomHeight;
	oldplumeBottomHeight = new FFArray<double>("oldplumeBottomHeight", 0.,  nx+2, ny+2);
//	if ( oldsmokeAtGround ) delete oldsmokeAtGround;
	oldsmokeAtGround = new FFArray<double>("oldsmokeAtGround", 0.,  nx+2, ny+2);
//	if ( oldtke ) delete oldtke;
	oldtke = new FFArray<double>("oldtke", 0.,  nx+2, ny+2);


}

size_t AtmosphericData::getSize(){
	if (windU) return windU->getSize();
	return 0;
}

size_t AtmosphericData::getSizeX(){
	if (windU) return windU->getDim("x");
	return 0;
}

size_t AtmosphericData::getSizeY(){
	if (windU) return windU->getDim("y");
	return 0;
}

}
