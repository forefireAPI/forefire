/**
 * @file FFVector.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FFVector.h"

namespace libforefire{

const double FFVector::epsilonv = 1.e-12;
const double FFVector::Pi = 3.141592653589793;

// constructors and destructor

FFVector::
FFVector() {
	vx = 0;
	vy = 0;
	vz = 0;
}
FFVector::
FFVector(const double vx0, const double vy0) {
	vx = vx0;
	vy = vy0;
	vz = 0;
}
FFVector::
FFVector(const double vx0, const double vy0, const double vz0) {
	vx = vx0;
	vy = vy0;
	vz = vz0;
}
FFVector::FFVector(FFPoint p1, FFPoint p2) {
	vx = p2.getX()-p1.getX();
	vy = p2.getY()-p1.getY();
	vz = p2.getZ()-p1.getZ();
}
FFVector::~FFVector() {
	// TODO Auto-generated destructor stub
}
FFVector::FFVector(const FFVector& v) : vx(v.vx), vy(v.vy), vz(v.vz){
	// nothing else to do
}

// overloading operators
const FFVector operator+(const FFVector& left, const FFVector& right){
	return FFVector(left.vx+right.vx,left.vy+right.vy,left.vz+right.vz);
}
const FFVector operator-(const FFVector& left, const FFVector& right){
	return FFVector(left.vx-right.vx,left.vy-right.vy,left.vz-right.vz);
}
const FFVector operator*(const double& k, const FFVector& right){
	return FFVector(k*right.vx,k*right.vy,k*right.vz);
}
FFVector& operator+=(FFVector& left, const FFVector& right){
	left.vx += right.vx;
	left.vy += right.vy;
	left.vz += right.vz;
	return left;
}
FFVector& operator-=(FFVector& left, const FFVector& right){
	left.vx -= right.vx;
	left.vy -= right.vy;
	left.vz -= right.vz;
	return left;
}
FFVector& operator*=(FFVector& left, const double& k){
	left.vx *= k;
	left.vy *= k;
	left.vz *= k;
	return left;
}
int operator==(const FFVector& left, const FFVector& right){
	return ( ( left.vx < right.vx + FFVector::epsilonv and left.vx > right.vx - FFVector::epsilonv )
			and ( left.vy < right.vy + FFVector::epsilonv and left.vy > right.vy - FFVector::epsilonv )
			and ( left.vz < right.vz + FFVector::epsilonv and left.vz > right.vz - FFVector::epsilonv ) );
}
int operator!=(const FFVector& left, const FFVector& right){
	return !(left==right);
}

// Accessors
double FFVector::getVx(){
	return vx;
}
double FFVector::getVy(){
	return vy;
}
double FFVector::getVz(){
	return vz;
}
FFVector& FFVector::getVec(){
	return *this;
}

// Mutators
void FFVector::setVx(const double& vx0){
	vx = vx0;
}
void FFVector::setVy(const double& vy0){
	vy = vy0;
}
void FFVector::setVz(const double& vz0){
	vz = vz0;
}
void FFVector::setVec(const double& vx0, const double& vy0, const double& vz0){
	vx = vx0;
	vy = vy0;
	vz = vz0;
}

// norm function
double FFVector::norm(){
	double sqnorm = vx*vx + vy*vy + vz*vz;
	return sqrt(sqnorm);
}

// normalization of the vector
void FFVector::normalize(){
	double a = norm();
	if ( a != 0. ) {
		vx = vx/a;
		vy = vy/a;
		vz = vz/a;
	}
}
FFVector FFVector::normed(){
	double a = norm();
	if ( a != 0. ) {
		return FFVector(vx/a, vy/a, vz/a);
	} else {
		return *this;
	}
}

// scalar product with another vector
double FFVector::scalarProduct(const FFVector& vec){
	return vx*vec.vx + vy*vec.vy+ vz*vec.vz;
}

// scalar product with another vector
FFVector FFVector::crossProduct(const FFVector& vec){
	if ( vz == 0. or vec.vz == 0. ) return FFVector(0.,0.,vx*vec.vy-vy*vec.vx);
	return FFVector(vy*vec.vz-vz*vec.vy,vz*vec.vx-vx*vec.vz,vx*vec.vy-vy*vec.vx);
}

// Cast into a 'FFpoint'
FFPoint FFVector::toPoint(){
	return FFPoint(vx,vy,vz);
}

double FFVector::toAngle(){
    if ( vx == 0 and vy == 0 ) {
        return 0;
    }
    double angle = (atan(vy /vx) + (Pi/2.)) ;
    if (vx < 0) {
        angle += Pi;
    }
   return angle;

}

// print function
string FFVector::print(){
	ostringstream oss;
	oss << "(" << vx << "," << vy << "," << vz << ")";
	return oss.str();
}

}


