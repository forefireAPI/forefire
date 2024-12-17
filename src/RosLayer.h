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

#ifndef ROSLAYER_H_
#define ROSLAYER_H_

#include "DataLayer.h"
#include "FDCell.h"

using namespace std;

namespace libforefire {


/*! \class RosLayer
 * \brief Template data layer object specific to burning ratio in atmospheric cells
 *
 *  RosLayer gives access to the burning ratio in atmospheric cells.
 *  The ratio is defined as the ratio of burning surface/total surface. Whenever
 *  getMatrix() is called the array is re-computed.
 */
template<typename T> class RosLayer : public DataLayer<T> {

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t size; /*!< size of the array */

	FFArray<T>* rosMap; /*!< pointer to the array of burning ratios */

	FDCell** cells; /*!< pointers to the atmospheric cells */

	double latestCall; /*!< time of the latest call to getMatrix() */


	SimulationParameters* params;

	/*! \brief interpolation method: lowest order */
	T getNearestData(FFPoint);
public:
	/*! \brief Default constructor */
	RosLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	RosLayer(string name, const size_t& nnx, const size_t& nny, FDCell** FDcells)
	: DataLayer<T>(name), nx(nnx), ny(nny), cells(FDcells) {
		size = nx*ny;
		rosMap = new FFArray<T>("Ros", 0., nx, ny);
		latestCall = -1.;
		params = SimulationParameters::GetInstance();
	};
	/*! \brief Destructor */
	virtual ~RosLayer(){
		delete rosMap;
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

};

template<typename T>
T RosLayer<T>::getVal(size_t i, size_t j){
	return (*rosMap)(i, j);
}

template<typename T>
T RosLayer<T>::getValueAt(FireNode* fn){
	return getNearestData(fn->getLoc());
}

template<typename T>
T RosLayer<T>::getValueAt(FFPoint loc, const double& time){
	return getNearestData(loc);
}

template<typename T>
size_t RosLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t RosLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
T RosLayer<T>::getNearestData(FFPoint loc){
	cout<<"RosLayer<T>::getNearestData() "
			<<"shouldn't have been called"<<endl;
	return 0.;
}

template<typename T>
void RosLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& t){

		// computing the burning ratio matrix
		for ( size_t i=0; i < nx; i++ ){
			for ( size_t j=0; j < ny; j++ ){
				(*rosMap)(i,j) = cells[i][j].getMaxSpeed(t);
			}
		}
		*matrix = rosMap;
}

template<typename T>
void RosLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( rosMap->getSize() == sizein ){
		rosMap->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve data for data layer "
				<<this->getKey()<<", matrix size not matching";
	}
}

template<typename T>
string RosLayer<T>::print(){
	return print2D(0,0);
}

template<typename T>
string RosLayer<T>::print2D(size_t i, size_t j){
	return rosMap->print2D(i,j);
}

template<typename T>
void RosLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){
	/* writing the matrix in a binary file */
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&ny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(rosMap->getData()), rosMap->getSize()*sizeof(T));
	FileOut.close();
}

}

#endif /* ROSLAYER_H_ */
