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

#ifndef MULTIPLICATIVELAYER_H_
#define MULTIPLICATIVELAYER_H_

#include "DataLayer.h"
#include "FDCell.h"
#include "DataBroker.h"

using namespace std;

namespace libforefire {

/*! \class MultiplicativeLayer
 * \brief Data layer for variables proportional to a given variable
 *
 *  MultiplicativeLayer implements a simple way to construct data layers
 *  related to properties proportional to a "base" one. The layer also
 *  stores the data of the proportional property.
 */
template<typename T> class MultiplicativeLayer : public DataLayer<T> {

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t nz; /*!< size of the array in the Y direction */
	size_t size; /*!< size of the array */

	FFArray<T>* matrix; /*!< pointer to the array containing the data */
	DataLayer<T>* baseLayer; /*!< pointer to the base layer */

	double coefficient; /*!< multiplicative coefficient */

	double latestCall; /*!< time of the latest call to getMatrix() */


	SimulationParameters* params;

	/*! \brief Interpolation method: lowest order */
	T getNearestData(FFPoint);
public:
	/*! \brief Default constructor */
	MultiplicativeLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	MultiplicativeLayer(string name, FFArray<T>* mat
			, DataLayer<T>* blayer, const double& coef)
	: DataLayer<T>(name), matrix(mat)
	  , baseLayer(blayer), coefficient(coef) {
		nx = matrix->getDim("x");
		ny = matrix->getDim("y");
		nz = matrix->getDim("z");
		size =  matrix->getSize();

		latestCall = -1.;

		params = SimulationParameters::GetInstance();
	};
	virtual ~MultiplicativeLayer(){
		delete matrix;
	}

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

	/*! \brief print the related data */
	string print();
	string print2D(size_t, size_t);
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);

};

template<typename T>
T MultiplicativeLayer<T>::getVal(size_t i, size_t j, size_t k){
	return (*matrix)(i, j, k);
}

template<typename T>
T MultiplicativeLayer<T>::getValueAt(FireNode* fn){
	return coefficient*(baseLayer->getValueAt(fn));
}

template<typename T>
T MultiplicativeLayer<T>::getValueAt(FFPoint loc, const double& time){
	return coefficient*(baseLayer->getValueAt(loc, time));
}

template<typename T>
size_t MultiplicativeLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t MultiplicativeLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* props, size_t curItem){
	return 0;
}

template<typename T>
T MultiplicativeLayer<T>::getNearestData(FFPoint loc){
	cout<<"MultiplicativeLayer<T>::getNearestData() "
			<<"shouldn't have been called"<<endl;
	return 0.;
}

template<typename T>
void MultiplicativeLayer<T>::getMatrix(
		FFArray<T>** mat, const double& t){
	if ( t != latestCall ){
		FFArray<double>* baseMatrix;
		baseLayer->getMatrix(&baseMatrix, t);
		for ( size_t i=0; i < nx; i++ ){
			for ( size_t j=0; j < ny; j++ ){
				(*matrix)(i,j) = coefficient*(*baseMatrix)(i,j);
			}
		}
		latestCall = t;
	} else {
		//cout<<"latest call was at "<<latestCall<<", while asking at "<<time<<endl;
	}
	*mat = matrix;
}

template<typename T>
void MultiplicativeLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( matrix->getSize() == sizein ){
		matrix->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve data for data layer "
				<<this->getKey()<<", matrix size not matching";
	}
}

template<typename T>
string MultiplicativeLayer<T>::print(){
	return print2D(0,0);
}

template<typename T>
string MultiplicativeLayer<T>::print2D(size_t i, size_t j){
	return matrix->print2D(i,j);
}

template<typename T>
void MultiplicativeLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){
	/* writing the matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&ny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(matrix->getData()), matrix->getSize()*sizeof(T));
	FileOut.close();
}

}

#endif /* MULTIPLICATIVELAYER_H_ */
