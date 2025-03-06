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

#ifndef BURNINGMAPLAYER_H_
#define BURNINGMAPLAYER_H_

#include "DataLayer.h"
#include "FireDomain.h"

namespace libforefire {

/*! \class BurningMapLayer
 * \brief Template data layer object specific to burning maps
 *
 *  BurningMapLayer gives access to the burning map.
 *  Whenever getMatrix() is called the array is re-computed.
 */
template<typename T> class BurningMapLayer : public DataLayer<T> {

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t size; /*!< size of the array */

	FireDomain* domain; /*!< domain related to the map of arrival times */

	FFArray<T>* arrivalTimes; /*!< pointer to the array of arrival times */

	double latestCall; /*!< time of the latest call to getMatrix() */
	SimulationParameters* params;

	/*! \brief interpolation method: lowest order */
	T getNearestData(FFPoint);
public:
	/*! \brief Default constructor */
	BurningMapLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	BurningMapLayer(string name, FireDomain* fd
			, const size_t& nnx, const size_t& nny)
	: DataLayer<T>(name), nx(nnx), ny(nny), domain(fd) {
		domain=fd;
		size = nx*ny;
		arrivalTimes = new FFArray<T>("BMap", 0., nx, ny);
		latestCall = -1.;
		params = SimulationParameters::GetInstance();
	};
	/*! \brief Destructor */
	virtual ~BurningMapLayer(){
		delete arrivalTimes;
	}

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

	/*! \brief print the related FFArray */
	string print2D(size_t, size_t);
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);

	double getDx(){ return getWidth()/nx; };
	double getDy(){ return getHeight()/ny; };
	double getDz(){ return 0; };
	double getOriginX(	){ return domain->getSWCorner().getX(); };
	double getOriginY(){ return domain->getSWCorner().getY(); };
	double getOriginZ(){ return 0;};
	double getWidth(){ return domain->getNECorner().getX()-domain->getSWCorner().getX(); };
	double getHeight(){ return domain->getNECorner().getY()-domain->getSWCorner().getY(); };
	double getDepth(){ return 0; };


};

template<typename T>
T BurningMapLayer<T>::getVal(size_t i, size_t j){
	return (*arrivalTimes)(i, j);
}

template<typename T>
T BurningMapLayer<T>::getValueAt(FireNode* fn){
	return getNearestData(fn->getLoc());
}

template<typename T>
T BurningMapLayer<T>::getValueAt(FFPoint loc, const double& time){
	return getNearestData(loc);
}

template<typename T>
size_t BurningMapLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t BurningMapLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
T BurningMapLayer<T>::getNearestData(FFPoint loc){
	cout<<"BurningMapLayer<T>::getNearestData() "
			<<"shouldn't have been called"<<endl;
	return 0.;
}

template<typename T>
void BurningMapLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& t){
	if ( t != latestCall ){

		for ( size_t i=0; i < nx; i++ ){
			for ( size_t j=0; j < ny; j++ ){
				(*arrivalTimes)(i,j) = domain->getArrivalTime(i, j);
			}
		}
		latestCall = t;
	}
	// Affecting the computed matrix to the desired array
	*matrix = arrivalTimes;
}

template<typename T>
void BurningMapLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( arrivalTimes->getSize() == sizein ){
		arrivalTimes->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve data for data layer "
				<<this->getKey()<<", matrix size not matching";
	}
}

template<typename T>
string BurningMapLayer<T>::print(){
	return print2D(0,0);
}

template<typename T>
string BurningMapLayer<T>::print2D(size_t i, size_t j){
	return arrivalTimes->print2D(i,j);
}

template<typename T>
void BurningMapLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){
	/* writing the matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&ny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(arrivalTimes->getData()), arrivalTimes->getSize()*sizeof(T));
	FileOut.close();
}

} /* namespace libforefire */
#endif /* BURNINGMAPLAYER_H_ */
