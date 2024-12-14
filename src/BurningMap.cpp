/*

Copyright (C) 2012 ForeFire Team, SPE, Universit� de Corse.

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

#include "BurningMap.h"

namespace libforefire {

BurningMap::BurningMap() {
	FFPoint fakeOrigin = FFPoint();
	double fakeRes = 123456789.;
	SWCorner = fakeOrigin;
	NECorner = fakeOrigin;
	dx = fakeRes;
	dy = fakeRes;
	sizeX = 1;
	sizeY = 1;
	arrivalTimeMap = 0;
}

BurningMap::
BurningMap(const FFPoint& sw, const FFPoint& ne
		, const size_t& nx, const size_t& ny) :
sizeX(nx), sizeY(ny), SWCorner(sw), NECorner(ne) {
	try {
		arrivalTimeMap = new FFArray<double>("ArrivalTime", numeric_limits<double>::infinity(), sizeX, sizeY);
		
		dx = ( NECorner.getX()-SWCorner.getX() )/sizeX;
		dy = ( NECorner.getY()-SWCorner.getY() )/sizeY;
	} catch ( const std::bad_alloc & ) {
		// the burning matrix is too refined, increasing the spatial resolution
		cout << "Burning matrix too refined for spatial resolution: "
				<< dx << ", " << dy << endl;
	}
}

BurningMap::~BurningMap() {
	delete arrivalTimeMap;
}

// Operators
double BurningMap::operator ()(size_t i, size_t j) const {
	if ( i >= sizeX or j >= sizeY ){
		cout << "Array subscript out of bounds when reading a value" << endl;
                return (*arrivalTimeMap)(0,0);
	}
	return (*arrivalTimeMap)(i,j);
}

double& BurningMap::operator ()(size_t i, size_t j){
	if ( i >= sizeX or j >= sizeY ){
		cout << "Array subscript out of bounds when writing a value" << endl;
                return (*arrivalTimeMap)(0,0);
	}
	return (*arrivalTimeMap)(i,j);
}



FFArray<double>* BurningMap::getMap(){
	return arrivalTimeMap;
}

// Mutators
void BurningMap::loadBin(std::ifstream&  FileIn){

		arrivalTimeMap->loadBin(	FileIn);
 
	
}
void BurningMap::setBurning(FFPoint& p, const double& time){
	// finding the indices in the burning matrix
	int kx = int((p.getX()-SWCorner.getX())/dx);
	int ky = int((p.getY()-SWCorner.getY())/dy);
	if ( (*arrivalTimeMap)(kx,ky) == 0. ){
		// set the time of the end of burning in that cell
		(*arrivalTimeMap)(kx,ky,time);
	}
}

bool BurningMap::nothingBurning(const double& time){
	// Assumption on the burning process
	double burningTime = 30.;
	for ( size_t i = 0; i < sizeX; i++ ) {
		for ( size_t j = 0; j < sizeY; j++ ) {
			if ( abs(time-(*arrivalTimeMap)(i,j)) < burningTime ) return false;
		}
	}
	return true;
}

double BurningMap::maxTime(){
	
	double infHere = numeric_limits<double>::infinity();
	double myMax = -9999;
	for ( size_t i = 0; i < sizeX; i++ ) {
		for ( size_t j = 0; j < sizeY; j++ ) {
			myMax = max((*arrivalTimeMap)(i,j),myMax);
			 if(myMax == infHere) return infHere;
		}
	}
	return myMax;
}

size_t BurningMap::getSizeX(){
	return sizeX;
}

size_t BurningMap::getSizeY(){
	return sizeY;
}

double BurningMap::getDx(){
	return dx;
}

double BurningMap::getDy(){
	return dy;
}

FFPoint BurningMap::getCenter(const size_t& i, const size_t& j){
	return FFPoint(SWCorner.getX()+(i+0.5)*dx, SWCorner.getY()+(j+0.5)*dy);
}

string BurningMap::toString(const double& time){
	ostringstream oss;
	for (size_t i=0;i<sizeX;i++){
		for (size_t j=0;j<sizeY;j++){
			if ( (*arrivalTimeMap)(i,j)-time > 0. ) {
				oss << 1 << " ";
			} else {
				oss << 0 << " ";
			}
		}
		oss << endl;
	}
	return oss.str();
}

}
