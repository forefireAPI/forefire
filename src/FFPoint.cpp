/**
 * @file FFPoint.cpp
 * @brief Implements the methods of the FFPoint class
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FFPoint.h"

using namespace std;

namespace libforefire{

const double FFPoint::Pi = 3.141592653589793;

// constructors and destructor
FFPoint::FFPoint(){
	x = 0.;
	y = 0.;
	z = 0.;
}

FFPoint::FFPoint(const double x0,const  double y0,const  double z0){
	x = x0;
	y = y0;
	z = z0;
}
FFPoint::~FFPoint(){
	// TODO
}
FFPoint::FFPoint(const FFPoint& p) : x(p.x), y(p.y), z(p.z) {
	// nothing else to do
}

// overloading operators
const FFPoint operator+(const FFPoint& left, const FFPoint& right){
	return FFPoint(left.x+right.x,left.y+right.y,left.z+right.z);
}
const FFPoint operator-(const FFPoint& left, const FFPoint& right){
	return FFPoint(left.x-right.x,left.y-right.y,left.z-right.z);
}
const FFPoint operator*(const double& k, const FFPoint& right){
	return FFPoint(k*right.x,k*right.y,k*right.z);
}
FFPoint& operator+=(FFPoint& left, const FFPoint& right){
	left.x += right.x;
	left.y += right.y;
	left.z += right.z;
	//
	//
	return left;
}
FFPoint& operator-=(FFPoint& left, const FFPoint& right){
	left.x -= right.x;
	left.y -= right.y;
	left.z -= right.z;
	return left;
}
FFPoint& operator*=(FFPoint& left, const double& k){
	left.x *= k;
	left.y *= k;
	left.z *= k;
	return left;
}
int operator==(const FFPoint& left, const FFPoint& right){
	return (left.x==right.x)&&(left.y==right.y)&&(left.z==right.z);
}
int operator!=(const FFPoint& left, const FFPoint& right){
	return (left.x!=right.x)||(left.y!=right.y)||(left.z!=right.z);
}

// Accessors
double& FFPoint::getX(){
	return x;
}
double& FFPoint::getY(){
	return y;
}
double& FFPoint::getZ(){
	return z;
}
FFPoint& FFPoint::getLoc(){
	return *this;
}

// Mutators
void FFPoint::setX(const double& x0){
	x = x0;
}
void FFPoint::setY(const double& y0){
	y = y0;
}
void FFPoint::setZ(const double& z0){
	z = z0;
}
void FFPoint::setLoc(const double& x0, const double& y0, const double& z0){
	x = x0;
	y = y0;
	z = z0;
}
void FFPoint::setLoc(FFPoint p){
	x = p.getX();
	y = p.getY();
	z = p.getZ();
}

// norm function
double FFPoint::norm(){
	double sqnorm = x*x + y*y + z*z;
	return sqrt(sqnorm);
}

// distance function
double FFPoint::distance(FFPoint p){
	return sqrt((p.x-x)*(p.x-x) + (p.y-y)*(p.y-y) + (p.z-z)*(p.z-z));
}

double FFPoint::distance2D(FFPoint p){
	return sqrt((p.x-x)*(p.x-x) + (p.y-y)*(p.y-y));
}

double FFPoint::distance2D(double& px, double& py){
	return sqrt((px-x)*(px-x) + (py-y)*(py-y));
}

// scalar product with another vector
double FFPoint::scalarProduct(FFPoint p){
	return x*p.x + y*p.y+ z*p.z;
}

// cross product function
FFPoint FFPoint::crossProduct(FFPoint p){
	if ( z == 0. or p.z == 0. ) return FFPoint(0.,0.,x*p.y-y*p.x);
	return FFPoint(y*p.z-z*p.y,x*p.z-z*p.x,x*p.y-y*p.x);
}

// 2D angle with another point
double FFPoint::angle2D(FFPoint p){
   double theta1 = atan2(y,x);
   double theta2 = atan2(p.y,p.x);
   double dtheta = theta2-theta1;
   while ( dtheta > Pi ) dtheta -= 2.*Pi;
   while ( dtheta < -Pi ) dtheta += 2.*Pi;
   return dtheta;
}

// Distance to a segment
double FFPoint::distanceToSegment(double& ax, double& ay
		, double& bx, double& by){
	// Return minimum distance between line segment ab and point
	const double ab = (ax-bx)*(ax-bx) + (ay-by)*(ay-by);  // i.e. |b-a|^2 -  avoid a sqrt
	if (ab == 0.0) return distance2D(ax, ay);   // a == b case
	// Consider the line extending the segment, parameterized as a + t (b - a).
	// We find projection of point onto the line.
	// It falls where t = [(p-a) . (b-a)] / |b-a|^2
	const double t = ((x-ax)*(bx-ax)+(y-ay)*(by-ay))/ab;
	if (t < 0.0) return distance2D(ax, ay);       // Beyond the 'a' end of the segment
	else if (t > 1.0) return distance2D(bx, by);  // Beyond the 'b' end of the segment
	double px = ax + t*(bx-ax);  // Projection falls on the segment
	double py = ay + t*(by-ay);  // Projection falls on the segment
	return distance2D(px, py);
}


// Signed distance between a point and a polygon
double FFPoint::signedDistanceToPolygon(
		size_t& nvert, double* vertx, double* verty, bool expanding){
	/* signed distance between a point and a polygon.  */
	double d = distanceToSegment(vertx[0], verty[0], vertx[nvert-1], verty[nvert-1]);
	double di;
	for ( size_t i = 0; i < nvert-1; i++ ){
		di = distanceToSegment(vertx[i], verty[i], vertx[i+1], verty[i+1]);
		if ( di < d ) d = di;
	}
	if ( pointInPolygon(nvert, vertx, verty) xor expanding ) return -d;
	return d;
}

// Testing if a point lies in a given polygon
bool FFPoint::pointInPolygon(size_t& nvert, double* vertx, double* verty){
	/* run a semi-infinite ray horizontally (increasing x, fixed y)
	 * out from the test point, and count how many edges it crosses.
	 * At each crossing, the ray switches between inside and outside.
	 * This is called the Jordan curve theorem */
	size_t i;
	size_t j = nvert-1 ;
	bool oddNodes = false;

	for ( i = 0 ; i < nvert; i++) {
		if ( ((verty[i]< y && verty[j] >= y) or (verty[j]< y && verty[i] >= y) )
				and ( (vertx[i] <= x) or (vertx[j] <= x))) {
			if (vertx[i]+(y-verty[i])/(verty[j]-verty[i])*(vertx[j]-vertx[i])<x){
				oddNodes = !oddNodes;
			}
		}
		j=i;
	}
	return oddNodes;
}


double FFPoint::projectLat(double refLatitude, double metersPerDegreesLat) {
    // Compute the latitude offset (in degrees) from the northward displacement.
    return refLatitude + (y / metersPerDegreesLat);
}

double FFPoint::projectLon(double refLongitude, double metersPerDegreesLon) {
    // Compute the longitude offset (in degrees) from the eastward displacement.
    return refLongitude + (x / metersPerDegreesLon);
}

// print function
string FFPoint::print(){
	ostringstream oss;
	oss << "(" << x << "," << y << "," << z << ")";
	return oss.str();
}


}
