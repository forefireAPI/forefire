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
Foundation, Inc., 51  Street, Fifth Floor, Boston, MA 02110-1301 US

*/

#ifndef ARRAYDATALAYER_H_
#define ARRAYDATALAYER_H_

#include <math.h>
#include "DataLayer.h"
#include "FFArrays.h"
#include "FireNode.h"
#include "include/FFConstants.h"

using namespace std;

namespace libforefire {

/*! \class Array3DdataLayer
 * \brief Template FFArray structure-based data layer object
 *
 *  Array3DdataLayer defines a template data layer which data
 *  is available by the means of a FFArray
 */
template<typename T> class Array3DdataLayer : public DataLayer<T> {

	double originX; /*!< abscissa of the SouthWest Corner */
	double originY; /*!< coordinate of the SouthWest Corner */
	double originZ; /*!< altitude of the SouthWest Corner */

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t nz; /*!< size of the array in the Z direction */
	size_t size; /*!< size of the array */

	double dx; /*!< resolution of the array in the X direction */
	double dy; /*!< resolution of the array in the Y direction */
	double dz; /*!< resolution of the array in the Z direction */

	FFArray<T>* array; /*!< pointer to the FFArray containing the data */

	/*! \brief checking to see if indices are within bounds */
	bool inBound(const size_t&, const size_t& = 0, const size_t& = 0);

	/*! \brief interpolation method: lowest order */
	FFPoint posToIndices(FFPoint);

	/*! \brief interpolation method: lowest order */
	T getNearestData(FFPoint);

	/*! \brief interpolation method: bilinear */
	T bilinearInterp(FFPoint);

public:
	/*! \brief Default constructor */
	Array3DdataLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	Array3DdataLayer(string name, FFArray<T>* matrix
			, FFPoint& SWCorner, FFPoint& NECorner)
	: DataLayer<T>(name), array(matrix) {
		nx = array->getDim("x");
		ny = array->getDim("y");
		nz = array->getDim("z");
		size = array->getSize();
		originX = SWCorner.getX();
		originY = SWCorner.getY();
		originZ = SWCorner.getZ();
		dx = ( NECorner.getX() - SWCorner.getX() )/nx;
		dy = NECorner.getY() - SWCorner.getY() > EPSILONX ?
			( NECorner.getY() - SWCorner.getY() )/ny : 1;
		dz = NECorner.getZ() - SWCorner.getZ() > EPSILONX ?
			( NECorner.getZ() - SWCorner.getZ() )/nz : 1;
		interp = bilinear;
	};
	/*! \brief Destructor */
	virtual ~Array3DdataLayer(){
		delete array;
	}

	double getDx(){ return dx; };
	double getDy(){ return dy; };
	double getDz(){ return dz; };
	double getOriginX(	){ return originX; };
	double getOriginY(){ return originY; };
	double getOriginZ(){ return originZ; };
	double getWidth(){ return dx*nx; };
	double getHeight(){ return dy*ny; };
	double getDepth(){ return dz*nz; };


	/*! \brief interpolation method enum type */
	enum InterpolationMethod {
		nearestData = 0,
		bilinear = 1
	};

	/*! \brief interpolation method */
	InterpolationMethod interp;

	/*! \brief obtains the value at a given position in the array */
	T getVal(size_t = 0, size_t = 0, size_t = 0);

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);

	void setValueAt(FFPoint loc,  double tval, T value){};
	/*! \brief getter to the pointer on the FFArray */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a fortran array into the FFArray */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	/*! \brief setter to interpolation method */
	void setInterpolationMethod(InterpolationMethod);

	/*! \brief print the related FFArray */
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);




};

template<typename T>
T Array3DdataLayer<T>::getVal(size_t i, size_t j, size_t k){
	return (*array)(i, j, k);
}

template<typename T>
T Array3DdataLayer<T>::getValueAt(FireNode* fn){
	if ( interp == nearestData ) return getNearestData(fn->getLoc());
	if ( interp == bilinear ) return bilinearInterp(fn->getLoc());
	cout<<"WARNING: unknown interpolation method in "
			<<"Array3DdataLayer<T>::getValueAt(FireNode*)"<<endl;
	return (T) 0;
}

template<typename T>
T Array3DdataLayer<T>::getValueAt(FFPoint loc, const double& time){
	if ( interp == nearestData ) return getNearestData(loc);
	if ( interp == bilinear ) return bilinearInterp(loc);
	cout<<"WARNING: unknown interpolation method in "
			<<"Array3DdataLayer<T>::getValueAt(FFPoint, const double&)"<<endl;
	return (T) 0;
}


double getDx();
double getDy();
FFPoint& getOriginPoint();
FFPoint& getSpan();

template<typename T>
size_t Array3DdataLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t Array3DdataLayer<T>::getValuesAt(FFPoint loc, const double&
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
bool Array3DdataLayer<T>::inBound(
		const size_t& ii, const size_t& jj, const size_t& kk){
	return (ii >= 0) && (ii < nx)
			&& (jj >= 0) && (jj < ny)
			&& (kk >= 0) && (kk < nz);
}

template<typename T>
void Array3DdataLayer<T>::setInterpolationMethod(InterpolationMethod method){
	interp = method;
}

template<typename T>
T Array3DdataLayer<T>::getNearestData(FFPoint loc){
	/* searching the coordinates of the nodes around */
	FFPoint indices = posToIndices(loc);
	/* getting the floor value */
	size_t i = (size_t) indices.getX();
	size_t j = (size_t) indices.getY();
	size_t k = (size_t) indices.getZ();
	return getVal(i, j, k);
}

template<typename T>
FFPoint Array3DdataLayer<T>::posToIndices(FFPoint loc){
	return FFPoint((loc.getX()-originX)/dx
			, (loc.getY()-originY)/dy
			, (loc.getZ()-originZ)/dz);
}

template<typename T>
T Array3DdataLayer<T>::bilinearInterp(FFPoint loc){
	/* This method is only a bilinear interpolation, not 3D */
	/* This method implements a bilinear interpolation in space
	 * and linear interpolation in time */

	T val = 0.;

	/* searching the coordinates of the nodes around */
	FFPoint indices = posToIndices(loc);

	double ud = indices.getX() + EPSILONX;
	double vd = indices.getY() + EPSILONX;

	int uu = (int) ceil(ud-1);
	int vv = (int) ceil(vd-1);

	if ( uu < 0 ) uu = 0;
	if ( uu > (int) nx - 2 ) uu = (int) nx - 2;
	if ( vv < 0 ) vv = 0;
	if ( vv > (int) ny - 2 ) vv = (int) ny - 2;

	double udif = ud - uu;
	double vdif = vd - vv;

	double csw = (1.-udif) * (1.-vdif);
	double cse = udif * (1 - vdif);
	double cnw = (1 - udif) * vdif;
	double cne = udif * vdif;

	double tsw = getVal(uu,vv);
	double tnw = getVal(uu,vv+1);
	double tne = getVal(uu+1,vv+1);
	double tse = getVal(uu+1,vv);

	val = csw*tsw + cse*tse + cnw*tnw + cne*tne;
	return val;
}

template<typename T>
void Array3DdataLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& time){
	*matrix = array;
}

template<typename T>
void Array3DdataLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( array->getSize() == sizein ){
		// copying data from atmospheric matrix
		array->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve data for layer "
				<<this->getKey()<<", matrix size not matching";
	}
}

template<typename T>
string Array3DdataLayer<T>::print(){
	ostringstream oss;
	oss<<"For layer "<<this->getKey()<<" at surface:"<<endl;
	oss<<array->print2D();
	return oss.str();
}

template<typename T>
void Array3DdataLayer<T>::dumpAsBinary(string filename, const double& t
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){

	T vals[nnx*nny];

	double ddx = (NEC.getX()-SWC.getX())/nnx;
	double ddy = (NEC.getY()-SWC.getY())/nny;

	FFPoint loc;
	loc.setX(SWC.getX()+0.5*ddx);
	for ( size_t i = 0; i < nnx-1; i++ ) {
		loc.setY(SWC.getY()+0.5*ddy);
		for ( size_t j = 0; j < nny-1; j++ ) {
			vals[i*nny+j] = getValueAt(loc, t);
			loc.setY(loc.getY()+ddy);
		}
		loc.setX(loc.getX()+ddx);
	}

	/* writing the interpolated matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nnx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(vals), sizeof(vals));
	FileOut.close();
}

/*! \class Array2DdataLayer
 * \brief Template FFArray structure-based data layer object
 *
 *  Array2DdataLayer defines a template data layer which data
 *  is available by the means of a FFArray
 */
template<typename T> class Array2DdataLayer : public DataLayer<T> {

	double originX; /*!< abscissa of the SouthWest Corner */
	double originY; /*!< coordinate of the SouthWest Corner */

	FFPoint origin;
	FFPoint span;
	

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t size; /*!< size of the array */

	double dx; /*!< resolution of the array in the X direction */
	double dy; /*!< resolution of the array in the Y direction */

	FFArray<T>* array; /*!< pointer to the FFArray containing the data */

	/*! \brief checking to see if indices are within bounds */
	bool inBound(const size_t&, const size_t& = 0);

	/*! \brief interpolation method: lowest order */
	FFPoint posToIndices(FFPoint);

	/*! \brief interpolation method: lowest order */
	T getNearestData(FFPoint);

	/*! \brief interpolation method: bilinear */
	T bilinearInterp(FFPoint);

public:
	/*! \brief Default constructor */
	Array2DdataLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	Array2DdataLayer(string name, FFArray<T>* matrix
			, FFPoint& SWCorner, FFPoint& NECorner)
	: DataLayer<T>(name), array(matrix) {
		nx = array->getDim("x");
		ny = array->getDim("y");
		size = array->getSize();
		originX = SWCorner.getX();
		originY = SWCorner.getY();
		dx = ( NECorner.getX() - SWCorner.getX() )/nx;
		dy = NECorner.getY() - SWCorner.getY() > EPSILONX ?
			( NECorner.getY() - SWCorner.getY() )/ny : 1;
		interp = bilinear;
	};
	/*! \brief Destructor */
	virtual ~Array2DdataLayer(){
		if (array) delete array;
	}

	/*! \brief interpolation method enum type */
	enum InterpolationMethod {
		nearestData = 0,
		bilinear = 1
	};

	/*! \brief interpolation method */
	InterpolationMethod interp;

	/*! \brief obtains the value at a given position in the array */
	T getVal(size_t = 0, size_t = 0);

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);
	void setValueAt(FFPoint loc,  double tval, T value){};
	/*! \brief getter to the pointer on the FFArray */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a fortran array into the FFArray */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	/*! \brief setter to interpolation method */
	void setInterpolationMethod(InterpolationMethod);

	/*! \brief print the related FFArray */
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);
			
	double getDx(){ return dx; };
	double getDy(){ return dy; };
	double getDz(){ return 0; }; 
	double getOriginX(	){ return originX; };
	double getOriginY(){ return originY; };
	double getOriginZ(){ return 0; };
	double getWidth(){ return dx*nx; };
	double getHeight(){ return dy*ny; };
	double getDepth(){ return 0; };

};

template<typename T>
T Array2DdataLayer<T>::getVal(size_t i, size_t j){
	return (*array)(i, j);
}

template<typename T>
T Array2DdataLayer<T>::getValueAt(FireNode* fn){
	if ( interp == nearestData ) return getNearestData(fn->getLoc());
	if ( interp == bilinear ) return bilinearInterp(fn->getLoc());
	cout<<"WARNING: unknown interpolation method in "
			<<"Array2DdataLayer<T>::getValueAt(FireNode*)"<<endl;
	return (T) 0;
}

template<typename T>
T Array2DdataLayer<T>::getValueAt(FFPoint loc, const double& time){
	if ( interp == nearestData ) return getNearestData(loc);
	if ( interp == bilinear ) return bilinearInterp(loc);
	cout<<"WARNING: unknown interpolation method in "
			<<"Array3DdataLayer<T>::getValueAt(FFPoint, const double&)"<<endl;
	return (T) 0;
}

template<typename T>
size_t Array2DdataLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t Array2DdataLayer<T>::getValuesAt(FFPoint loc, const double&
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
bool Array2DdataLayer<T>::inBound(
		const size_t& ii, const size_t& jj){
	return (ii >= 0) && (ii < nx)
			&& (jj >= 0) && (jj < ny);
}

template<typename T>
void Array2DdataLayer<T>::setInterpolationMethod(InterpolationMethod method){
	interp = method;
}

template<typename T>
T Array2DdataLayer<T>::getNearestData(FFPoint loc){
	/* searching the coordinates of the nodes around */
	FFPoint indices = posToIndices(loc);
	/* getting the floor value */
	size_t i = (size_t) indices.getX();
	size_t j = (size_t) indices.getY();
	return getVal(i, j);
}

template<typename T>
FFPoint Array2DdataLayer<T>::posToIndices(FFPoint loc){
	return FFPoint((loc.getX()-originX)/dx
			, (loc.getY()-originY)/dy,0);
}

template<typename T>
T Array2DdataLayer<T>::bilinearInterp(FFPoint loc){
	/* This method is only a bilinear interpolation, not 3D */
	/* This method implements a bilinear interpolation in space
	 * and linear interpolation in time */

	T val = 0.;

	/* searching the coordinates of the nodes around */
	FFPoint indices = posToIndices(loc);

	double ud = indices.getX() + EPSILONX;
	double vd = indices.getY() + EPSILONX;

	int uu = (int) ceil(ud-1);
	int vv = (int) ceil(vd-1);

	if ( uu < 0 ) uu = 0;
	if ( uu > (int) nx - 2 ) uu = (int) nx - 2;
	if ( vv < 0 ) vv = 0;
	if ( vv > (int) ny - 2 ) vv = (int) ny - 2;

	double udif = ud - uu;
	double vdif = vd - vv;

	double csw = (1.-udif) * (1.-vdif);
	double cse = udif * (1 - vdif);
	double cnw = (1 - udif) * vdif;
	double cne = udif * vdif;

	double tsw = getVal(uu,vv);
	double tnw = getVal(uu,vv+1);
	double tne = getVal(uu+1,vv+1);
	double tse = getVal(uu+1,vv);

	val = csw*tsw + cse*tse + cnw*tnw + cne*tne;
	return val;
}

template<typename T>
void Array2DdataLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& time){
	*matrix = array;
}

template<typename T>
void Array2DdataLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( array->getSize() == sizein ){
		// copying data from atmospheric matrix
		array->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve data for layer "
				<<this->getKey()<<", matrix size not matching";
	}
}

template<typename T>
string Array2DdataLayer<T>::print(){
	ostringstream oss;
	oss<<"For layer "<<this->getKey()<<" at surface:"<<endl;
	oss<<array->print2D();
	return oss.str();
}

template<typename T>
void Array2DdataLayer<T>::dumpAsBinary(string filename, const double& t
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){

	T vals[nnx*nny];

	double ddx = (NEC.getX()-SWC.getX())/nnx;
	double ddy = (NEC.getY()-SWC.getY())/nny;

	FFPoint loc;
	loc.setX(SWC.getX()+0.5*ddx);
	for ( size_t i = 0; i < nnx-1; i++ ) {
		loc.setY(SWC.getY()+0.5*ddy);
		for ( size_t j = 0; j < nny-1; j++ ) {
			vals[i*nny+j] = getValueAt(loc, t);
			loc.setY(loc.getY()+ddy);
		}
		loc.setX(loc.getX()+ddx);
	}

	/* writing the interpolated matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nnx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(vals), sizeof(vals));
	FileOut.close();
}

}

#endif /* ARRAYDATALAYER_H_ */
