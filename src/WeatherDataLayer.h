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

#ifndef WEATHERDATALAYER_H_
#define WEATHERDATALAYER_H_


#include "DataLayer.h"

#include "FluxModel.h"

#include <cmath> // For atan2 and M_PI

using namespace std;

namespace libforefire {

// TODO X, XY and XYZ classes

/*! \class WeatherDataLayer
 * \brief Data Layer for variables stored in NetCDF format
 *
 *  WeatherDataLayer allows to create layers related to properties stored
 *  in a NetCDF format up to 4D. This class extracts one specific
 *  NetCDF variable of the file to store it in memory.
 */

template<typename T> class WeatherDataLayer : public DataLayer<T> {

	double SWCornerX; /*!< origin in the X direction */
	double SWCornerY; /*!< origin in the Y direction */
	double SWCornerZ; /*!< origin in the Z direction */
	double startTime; /*!< origin of time */

	double NECornerX; /*!< origin in the X direction */
	double NECornerY; /*!< origin in the Y direction */
	double NECornerZ; /*!< origin in the Z direction */
	double endTime; /*!< origin of time */

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the X direction */
	size_t nz; /*!< size of the array in the Z direction */
	size_t nt; /*!< size of the array in the T direction */
	size_t size; /*!< size of the array */

	double dx; /*!< increment in the X direction */
	double dy; /*!< increment in the Y direction */
	double dz; /*!< increment in the Z direction */
	double dt; /*!< increment in the T direction */

	size_t interDirID0 ;
	size_t interDirID1;
	double interDirRatio ;
	double interDirScaler ;
	string nameOf;


	FFArray<T>* array; /*!< pointer to the FFArray containing the data */

	/*! \brief Interpolation method: lowest order */
	T getNearestData(FFPoint, const double&);

	/*! \brief Interpolation method: multilinear */
	FFPoint posToIndices(FFPoint&);
	T bilinearInterp(FFPoint, const double&);

public:


	/*! \brief Default constructor */
	WeatherDataLayer() : DataLayer<T>() {
		startTime = 0;
			nx = 1;
			ny = 1;
			nz = 1;
			nt = 1;
			array = NULL;
			size = 1;
			SWCornerX = 0.;
			SWCornerY = 0.;
			SWCornerZ = 0.;
			NECornerX = 0.;
			NECornerY = 0.;
			NECornerZ = 0.;
			endTime = 0.;
			dx = 0.;
			dy = 0.;
			dz = 0.;
			dt = 0.;
			interDirID0 = 0 ;
			interDirID1 = 1 ;
			interDirRatio = 1  ;
			interDirScaler = 1  ;

			interp = InterpolationBilinear;

	};
	/*! \brief Constructor with a sole value */
	WeatherDataLayer(string name, T val) :
		DataLayer<T>(name) {
		startTime = 0;
		nx = 1;
		ny = 1;
		nz = 1;
		nt = 1;
		array = new FFArray<T>(name, val, nx, ny, nz, nt);
		size = 1;
		SWCornerX = 0.;
		SWCornerY = 0.;
		SWCornerZ = 0.;
		NECornerX = 0.;
		NECornerY = 0.;
		NECornerZ = 0.;
		endTime = 0.;
		dx = 0.;
		dy = 0.;
		dz = 0.;
		dt = 0.;
		interDirID0 = 0 ;
		interDirID1 = 1 ;
		interDirRatio = 0.5  ;
		interDirScaler = 1  ;
		
		interp = InterpolationBilinear;

	
	
	}
	/*! \brief Constructor with a given file and given variable */
	WeatherDataLayer(string name, FFPoint& SWCorner, double& t0
			, FFPoint& extent, double& timespan
			, size_t& nnx, size_t& nny, size_t& nnz, size_t& nnt, T* vals) :
		DataLayer<T>(name), startTime(t0)
		, nx(nnx), ny(nny), nz(nnz), nt(nnt) { 
		array = new FFArray<T>(name, vals, nnx, nny, nnz, nnt);
		size = (size_t) nx*ny*nz*nt;
 
		SWCornerX = SWCorner.getX();
		nameOf=name;
		SWCornerY = SWCorner.getY();
		SWCornerZ = SWCorner.getZ();
		NECornerX = SWCornerX + extent.getX();
		NECornerY = SWCornerY + extent.getY();
		NECornerZ = SWCornerZ + extent.getZ(); 
		endTime = startTime + timespan;
 
		dx = extent.getX()/nx;
		dy = extent.getY()/ny;
		dz = extent.getZ()/nz;
		dt = 0;
		if((timespan < 3600*24)&&	(nt>1)){	 
			dt = timespan/(nt-1);
		}
 
		interp = InterpolationBilinear; 
		interDirID0 = 0 ;
		interDirID1 = 1 ;
		interDirRatio = 1  ;
		interDirScaler = 1  ;

		if(nt>1){
			interp = InterpolationBilinearT;
		}
		

	}
	/*! \brief destructor */
	~WeatherDataLayer();

	/*! \brief interpolation method */
	static const short InterpolationNearestData = 0;
	static const short  InterpolationBilinear = 1;
	static const short  InterpolationBilinearT = 2;
	static const short  InterpolationBilinearDir = 3;
	static const short  InterpolationBilinearCoeff  = 5;
	static const short  InterpolationBilinearZ = 4;

	/*! \brief interpolation method */
	short interp;

	/*! \brief obtains the value at a given position in the array */
	T getVal(size_t = 0, size_t = 0, size_t = 0, size_t = 0);
	/*! \brief obtains the value at a given position in the array */
	T getVal(size_t = 0);
	/*! \brief obtains the position in the array for given location and time */
	size_t getPos(FFPoint&, const double&);

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief sets the value at a given location and time */
	void setValueAt(FFPoint,  double, T value);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);

	/*! \brief getter to the desired array at surface for a given time (should not be used) */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores array from a given array (should not be used) */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	/*! \brief print the related array (should not be used) */
	string print();
	string print2D(size_t, size_t);
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);

	void setProjectionDirVector(FFVector& ,FFPoint& );

};

template<typename T>
WeatherDataLayer<T>::~WeatherDataLayer() {
	delete array;
}

template<typename T>
T WeatherDataLayer<T>::getVal(size_t pos){
	return (*array)(pos);
}

template<typename T>
T WeatherDataLayer<T>::getVal(size_t i, size_t j, size_t k, size_t t){
	return (*array)(i, j, k, t);
}

template<typename T>
size_t WeatherDataLayer<T>::getPos(FFPoint& loc, const double& t){
	int i = nx>1 ? (int) (loc.getX()-SWCornerX)/dx : 0;
	int j = ny>1 ? (int) (loc.getY()-SWCornerY)/dy : 0;
	int k = nz>1 ? (int) (loc.getZ()-SWCornerZ)/dz : 0;
	int tt = nt>1 ? (int) (t-startTime)/dt : 0;

	if ( i < 0 or i > (int) nx-1 ){
		cout<<loc.getX()<<" is not within the 'x' domain"
				<<" of validity of array for "<<this->getKey()
				<<" ("<<SWCornerX<<"->"<<NECornerX<<")"<< endl;
		return 0;
	}
	if ( j < 0 or j > (int) ny-1 ){
		cout<<loc.getY()<<" is not within the 'y' domain"
				<<" of validity of array for "<<this->getKey()
				<<" ("<<SWCornerY<<"->"<<NECornerY<<")"<< endl;
		return 0;
	}
	if ( k < 0 or k > (int) nz-1 ){
		cout<<loc.getZ()<<" is not within the 'z' domain"
				<<" of validity of array for "<<this->getKey()
				<<" ("<<SWCornerZ<<"->"<<NECornerZ<<")"<< endl;
		return 0;
	}
	if ( tt < 0 or tt > (int) nt-1 ){
		cout<<t<<" is not within the 't' domain"
				<<" of validity of array for "<<this->getKey()
				<<" ("<<startTime<<"->"<<endTime<<")"<< endl;
		return 0;
	}
	return (size_t) i*ny*nz*nt + j*nz*nt + k*nt + tt;
}

template<typename T>
T WeatherDataLayer<T>::getValueAt(FireNode* fn){
	return getValueAt( fn->getLoc(), fn->getTime());
}

template<typename T>
T WeatherDataLayer<T>::getValueAt(FFPoint loc, const double& t){


	if ( size == 1 ) return (*array)(0, 0, 0, 0);

	if ( interp == InterpolationNearestData ) {
		return ((*array)(getPos(loc,t)));
	}


	    /* This method implements a multilinear interpolation */
		/* At this time no interpolation in 'Z' is done       */

		/* searching the coordinates of the nodes around      */
		FFPoint indices = posToIndices(loc);



		double ud = indices.getX() + EPSILONX;
		double vd = indices.getY() + EPSILONX;

		int uu = (int) ceil(ud-1);
		int vv = (int) ceil(vd-1);

		if ( uu < 0 ) uu = 0;
		if ( uu > ((int) nx) - 2 ) uu = ((int) nx) - 2;
		if ( vv < 0 ) vv = 0;
		if ( vv > ((int) ny) - 2 ) vv = ((int) ny) - 2;

		double udif = ud - ((double) uu);
		double vdif = vd - ((double) vv);

		double csw = (1.-udif) * (1.-vdif);
		double cse = udif * (1 - vdif);
		double cnw = (1 - udif) * vdif;
		double cne = udif * vdif;


		if ( interp == InterpolationBilinear )
		{
			T tsw = getVal(uu,vv);
			T tnw = getVal(uu,vv+1);
			T tne = getVal(uu+1,vv+1);
			T tse = getVal(uu+1,vv);
			return csw*tsw + cse*tse + cnw*tnw + cne*tne;

		}

		if ( interp == InterpolationBilinearT ){
					int it = (int) (t-startTime)/dt;

					T tsw1 = getVal(uu,vv,0,it);
					T tnw1 = getVal(uu,vv+1,0,it);
					T tne1 = getVal(uu+1,vv+1,0,it);
					T tse1 = getVal(uu+1,vv,0,it);

					T tsw2 = getVal(uu,vv,0,it+1);
					T tnw2 = getVal(uu,vv+1,0,it+1);
					T tne2 = getVal(uu+1,vv+1,0,it+1);
					T tse2 = getVal(uu+1,vv,0,it+1);

					T val1 = csw*tsw1 + cse*tse1 + cnw*tnw1 + cne*tne1;
					T val2 = csw*tsw2 + cse*tse2 + cnw*tnw2 + cne*tne2;

					/* interpolation in time */
					double at = ( t - it*dt )/dt;
					return at*val1 + (1.-at)*val2;

				}
		if ( interp == 	InterpolationBilinearCoeff ){
					//along X axis
					T tsw1 = getVal(uu,vv,0,0);
					T tnw1 = getVal(uu,vv+1,0,0);
					T tne1 = getVal(uu+1,vv+1,0,0);
					T tse1 = getVal(uu+1,vv,0,0);
					//along Y axis
					T tsw2 = getVal(uu,vv,1,0);
					T tnw2 = getVal(uu,vv+1,1,0);
					T tne2 = getVal(uu+1,vv+1,1,0);
					T tse2 = getVal(uu+1,vv,1,0);

					T val1 = csw*tsw1 + cse*tse1 + cnw*tnw1 + cne*tne1;
					T val2 = csw*tsw2 + cse*tse2 + cnw*tnw2 + cne*tne2;

					/* interpolation in dir + scale*/

					return interDirScaler*val1 + interDirRatio*val2;
				
			}
		if ( interp == InterpolationBilinearDir ){

					T tsw1 = getVal(uu,vv,interDirID0,0);
					T tnw1 = getVal(uu,vv+1,interDirID0,0);
					T tne1 = getVal(uu+1,vv+1,interDirID0,0);
					T tse1 = getVal(uu+1,vv,interDirID0,0);

					T tsw2 = getVal(uu,vv,interDirID1,0);
					T tnw2 = getVal(uu,vv+1,interDirID1,0);
					T tne2 = getVal(uu+1,vv+1,interDirID1,0);
					T tse2 = getVal(uu+1,vv,interDirID1,0);

					T val1 = csw*tsw1 + cse*tse1 + cnw*tnw1 + cne*tne1;
					T val2 = csw*tsw2 + cse*tse2 + cnw*tnw2 + cne*tne2;

					/* interpolation in dir + scale*/
					return interDirScaler*(interDirRatio*val1 + (1.-interDirRatio)*val2);

				}

	cout<<"WARNING: unknown interpolation method in "
			<<"Array2DdataLayer<T>::getValueAt(FireNode*)"<<endl;
	return (T) 0;
}

template<typename T>
void WeatherDataLayer<T>::setValueAt(FFPoint loc,  double t, T value){

	cout<<"WARNING: setting value not implemented"<<endl;

}


template<typename T>
size_t WeatherDataLayer<T>::getValuesAt(
		FireNode* fn, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t WeatherDataLayer<T>::getValuesAt(
		FFPoint loc, const double& t, FluxModel* model, size_t curItem){

	(model->properties)[curItem] = 	getValueAt( loc, t);

	return 1;
}

template<typename T>
T WeatherDataLayer<T>::getNearestData(FFPoint loc, const double& time){
	return (*array)(getPos(loc,time));
}

template<typename T>
FFPoint WeatherDataLayer<T>::posToIndices(FFPoint& loc){
	FFPoint ind;
	ind.setX((loc.getX()-SWCornerX)/dx-0.5);
	if ( ny > 1 ) ind.setY((loc.getY()-SWCornerY)/dy-0.5);
	if ( nz > 1 ) ind.setZ((loc.getZ()-SWCornerZ)/dz-0.5);
	return ind;
}



template<typename T>
void WeatherDataLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& time){
	*matrix = array;
}

template<typename T>
void WeatherDataLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	// writing the incoming (array in matrix
	// should not be done with this type of layer
}

template<typename T>
string WeatherDataLayer<T>::print(){
	return print2D(0,0);
}

template<typename T>
string WeatherDataLayer<T>::print2D(size_t k, size_t t){
	ostringstream oss;
	
	for ( int j = ny-1; j >= 0; j -= 10 ){
		 
		for ( size_t i = 0; i < nx; i += 10 ){
			oss<<getVal(i, j, k, t)<<" ";
		}
		oss<<endl;
	}
	return oss.str();
}

template<typename T>
void WeatherDataLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){

	T vals[nnx*nny];

	double ddx = (NEC.getX()-SWC.getX())/nnx;
	double ddy = (NEC.getY()-SWC.getY())/nny;

	FFPoint loc;
	loc.setX(SWC.getX());
	for ( size_t i = 0; i < nnx; i++ ) {
		loc.setY(SWC.getY());
		for ( size_t j = 0; j < nny; j++ ) {
			vals[i*nny+j] = getValueAt(loc, time);
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
	FileOut.write(reinterpret_cast<const char*>(&vals), sizeof(vals));
	FileOut.close();
}

template<typename T>
void WeatherDataLayer<T>::setProjectionDirVector(FFVector& stimuliScaler, FFPoint& stimuliLocation){
		if ( interp == 	InterpolationBilinearCoeff ){
			interDirScaler=stimuliScaler.getVx();
			interDirRatio=stimuliScaler.getVy();
		}
		if ( interp == 	InterpolationBilinearDir ){
			double Pi = 3.14159265358;
			interDirScaler = stimuliScaler.norm()/10;
			double angle = 0;
			if ( interDirScaler > 0  ){
				angle = stimuliScaler.toAngle()-Pi/2;
				if (angle <0) angle += 2*Pi;
			}
			double anglePied = (angle/(2*Pi))*nz;

			interDirID0 = floor(anglePied);
			interDirID1 = ceil(anglePied);
			interDirRatio = ( interDirID1 - anglePied );

			if (interDirID1 >=nz) interDirID1 = 0;
		}

}

}

#endif  /* WEATHERDATALAYER_H_ */
