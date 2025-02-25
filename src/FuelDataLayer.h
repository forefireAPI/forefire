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

#ifndef FUELDATALAYER_H_
#define FUELDATALAYER_H_
#include "DataLayer.h"
#include "FFArrays.h"
#include "PropagationModel.h"
#include "FluxModel.h"

using namespace std;

namespace libforefire {

/*! \class FuelDataLayer
 * \brief Data Layer for fuel parameters stored in NetCDF format
 *
 *  FuelDataLayer allows to create layer related to fuel parameters stored
 *  in a NetCDF format.
 */
template<typename T> class FuelDataLayer : public DataLayer<T> {

	/* Defining the map of fuels */
	int* fuelMap;
	FFArray<int>* myFuelMap; /*!< pointer to the FFArray containing the data */

	double SWCornerX; /*!< origin in the X direction */
	double SWCornerY; /*!< origin in the Y direction */
	double SWCornerZ; /*!< origin in the Z direction */
	double startTime; /*!< origin of time */

	double NECornerX; /*!< origin in the X direction */
	double NECornerY; /*!< origin in the Y direction */
	double NECornerZ; /*!< origin in the Z direction */
	double endTime; /*!< origin of time */

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t nz; /*!< size of the array in the Z direction */
	size_t nt; /*!< size of the array in the T direction */
	size_t size; /*!< size of the array */

	double dx; /*!< increment in the X direction */
	double dy; /*!< increment in the Y direction */
	double dz; /*!< increment in the Z direction */
	double dt; /*!< increment in the T direction */

	/* Defining the fuel properties requested by the propagation model */
	vector<map<string, double> > fuelPropertiesTable;

	size_t fuelsNum;

	/*! \brief Interpolation method: lowest order */
	int getFuelAtLocation(FFPoint, double time);


	size_t getPos(FFPoint& loc, double& time);

public:
    
	static const size_t MAXNUMFUELS = 1024;
	/*! \brief Default constructor */
	FuelDataLayer() : DataLayer<T>() {};
	/*! \brief Constructor for a lone fuel */
	FuelDataLayer(string name, int& findex) : DataLayer<T>(name) {
		nx = 1;
		ny = 1;
		nz = 1;
		nt = 1;
		size = 1;
		fuelMap = new int[size];
		fuelMap[0] = findex;
	}
	/*! \brief Constructor with a given file and given variable */
	FuelDataLayer(string name, FFPoint& SWCorner, double& t0
			, FFPoint& extent, double& timespan, size_t& nnx, size_t& nny
			, size_t& nnz, size_t& nnt, int* fmap)
		: DataLayer<T>(name), startTime(t0)
		  , nx(nnx), ny(nny), nz(nnz), nt(nnt) {

		size = (size_t) nx*ny*nz*nt;
		SWCornerX = SWCorner.getX();
		SWCornerY = SWCorner.getY();
		SWCornerZ = SWCorner.getZ();
		NECornerX = SWCornerX + extent.getX();
		NECornerY = SWCornerY + extent.getY();
		NECornerZ = SWCornerZ + extent.getZ();
		endTime = startTime + timespan;
		fuelMap = new int[size];

		for ( size_t i = 0; i<size; i++ ){
			fuelMap[i] = fmap[i];
		}

		dx = extent.getX()/nx;
		dy = extent.getY()/ny;
		dz = extent.getZ()/nz;
		dt = timespan/nt;

	}
	/*! \brief destructor */
	~FuelDataLayer();

	/*! \brief obtains the fuel index at a given position */
	int getFuel(size_t = 0, size_t = 0, size_t = 0, size_t = 0);
	/*! \brief obtains the fuel index at a given position */
	int getFuel(size_t = 0);

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief computes the value at a given location and time */
	void setValueAt(FFPoint,  double, T value);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);

	/*! \brief getter to the desired data at surface for a given time (should not be used) */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a given array (should not be used) */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	int* getFuelMap(){return fuelMap;}
	size_t getDim(string = "total");
	/*! \brief print the related data (should not be used) */
	string print();
	string print2D(size_t, size_t);
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);

};

template<typename T>
FuelDataLayer<T>::~FuelDataLayer() {
	delete [] fuelMap;
}

template<typename T>
int FuelDataLayer<T>::getFuel(size_t pos){
	return fuelMap[pos];
}

template<typename T>
int FuelDataLayer<T>::getFuel(size_t i, size_t j, size_t k, size_t t){
	return fuelMap[i*ny*nz*nt + j*nz*nt + k*nt + t];
}

template<typename T>
T FuelDataLayer<T>::getValueAt(FireNode* fn){
	FFPoint loc = fn->getLoc();
	T retval = (T) fuelMap[getPos(loc, fn->getTime())];
   return retval;
}

template<typename T>
T FuelDataLayer<T>::getValueAt(FFPoint loc, const double& time){
	double mytime = time;
	T retval = (T) fuelMap[getPos(loc, mytime)];
   return retval;
}

template<typename T>
size_t FuelDataLayer<T>::getValuesAt(
		FireNode* fn, PropagationModel* model, size_t curPosition){
	/* Getting the fuel at the given location */
	int fuelIndex = getFuelAtLocation(fn->getLoc(), fn->getTime());

	//cout << "getting indice "<< fuelIndex <<" at location "<< fn->getLoc().x <<";"<< fn->getLoc().y<<endl;
        /* writing the parameters' values in the desired array at desired location */
	for ( size_t param = 0; param < model->numFuelProperties; param++ ){
		(model->properties)[curPosition+param] = (*(model->fuelPropertiesTable))(fuelIndex, param);
	}
	/* returning the number of parameters written in the array */
	return model->numFuelProperties;
}

template<typename T>
size_t FuelDataLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curPosition){
	/* Getting the fuel at the given location */
	int fuelIndex = getFuelAtLocation(loc, t);
	//cout << "getting Not Prop "<< fuelIndex <<" at location "<< loc.x <<";"<< loc.y<<endl;
	/* writing the parameters' values in the desired array at desired location */
	for ( size_t param = 0; param < model->numFuelProperties; param++ ){
		(model->properties)[curPosition+param] = (*(model->fuelPropertiesTable))(fuelIndex, param);
	}
	/* returning the number of parameters written in the array */
	return model->numFuelProperties;
}

template<typename T>
int FuelDataLayer<T>::getFuelAtLocation(FFPoint loc, double time){
	if ( size == 1 ) return fuelMap[0];
	return fuelMap[getPos(loc, time)];
}

template<typename T>
void FuelDataLayer<T>::setValueAt(FFPoint loc,  double timeV, T value){
	fuelMap[getPos(loc, timeV)] = (int)value;
}
template<typename T>
size_t FuelDataLayer<T>::getDim(string dim){
	if ( dim == "x" ) return nx;
	if ( dim == "y" ) return ny;
	if ( dim == "z" ) return nz;
	if ( dim == "t" ) return nt;
	return size;
}

template<typename T>
size_t FuelDataLayer<T>::getPos(FFPoint& loc, double& t){
	int i, j, k, tt;
	nx>1 ? i = ((int) (loc.getX()-SWCornerX)/dx) : i = 0;
	ny>1 ? j = ((int) (loc.getY()-SWCornerY)/dy) : j = 0;
	nz>1 ? k = ((int) (loc.getZ()-SWCornerZ)/dz) : k = 0;
	nt>1 ? tt = ((int) (t-startTime)/dt) : tt = 0;

	if ( i < 0 or i > ((int) nx)-1 ) return 0;
	if ( j < 0 or j > ((int) ny)-1 ) return 0;
	if ( k < 0 or k > ((int) nz)-1 ) return 0;
	if ( tt < 0 or tt > ((int) nt)-1 ) return 0;
	size_t pos = (size_t) i*ny*nz*nt + j*nz*nt + k*nt + tt;
	return pos;
}

template<typename T>
void FuelDataLayer<T>::getMatrix(
		FFArray<T>** lmatrix, const double& time){
			double res = 10;
			double ddx = res;
			int nnx = (NECornerX - SWCornerX) / res;
			double ddy = res;
			int nny = (NECornerY - SWCornerY) / res;
			int nnz = 1;
			int nnt = 1;
		
			// Use a std::vector to allocate the array dynamically on the heap.
			std::vector<double> vals(nnx * nny);
		
			FFPoint loc;
			loc.setX(SWCornerX + 0.5 * ddx);
		
			for (int i = 0; i < nnx; i++) {
				loc.setY(SWCornerY + 0.5 * ddy);
				for (int j = 0; j < nny; j++) {
					vals[i * nny + j] = static_cast<double>(getFuelAtLocation(loc, startTime));
					loc.setY(loc.getY() + ddy);
				}
				loc.setX(loc.getX() + ddx);
			}
			
			*lmatrix = new FFArray<double>("fuel", 1, nnx, nny, nnz, nnt);
			(*lmatrix)->setVal(vals.data());


	// TODO extracting the data at the given time
}

template<typename T>
void FuelDataLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	// writing the incoming data in matrix
	// should not be done with this type of layer
}

template<typename T>
string FuelDataLayer<T>::print(){
	return print2D(0, 0);
}

template<typename T>
string FuelDataLayer<T>::print2D(size_t k, size_t t){
	ostringstream oss;
 
	for ( int j = ny-1; j >= 0; j -= 1 ){
		 
		for ( size_t i = 0; i < nx; i += 1 ){
			oss<<getFuel(i, j, k, t)<<" ";
		}
		oss<<endl;
	}
	return oss.str();
}

template<typename T>
void FuelDataLayer<T>::dumpAsBinary(string filename, const double& t
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){

	double vals[nnx*nny];

	double ddx = (NEC.getX()-SWC.getX())/nnx;
	double ddy = (NEC.getY()-SWC.getY())/nny;

	FFPoint loc;
	loc.setX(SWC.getX()+0.5*ddx);
	for ( size_t i = 0; i < nnx; i++ ) {
		loc.setY(SWC.getY()+0.5*ddy);
		for ( size_t j = 0; j < nny; j++ ) {
			vals[i*nny+j] = (double) getFuelAtLocation(loc, t);
			loc.setY(loc.getY()+ddy);
		}
		loc.setX(loc.getX()+ddx);
	}

	/* writing the matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nnx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(vals), sizeof(vals));
	FileOut.close();

}

}

#endif /* FUELDATALAYER_H_ */
