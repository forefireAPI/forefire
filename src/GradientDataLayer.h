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

#ifndef GRADIENTDATALAYER_H_
#define GRADIENTDATALAYER_H_

#include "DataLayer.h"
#include "FireNode.h"
#include "SimulationParameters.h"

using namespace std;

namespace libforefire {

/*! \class GradientDataLayer
 * \brief Implicit data layer for having a view on the gradient of a given data layer
 *
 *  GradientDataLayer implements an implicit data layer (no data stored) to compute
 *  at a given location, and for a given directions, the gradient of a given "parent" property.
 */
template<typename T> class GradientDataLayer: public DataLayer<T> {

	DataLayer<T>* parent; /*!< pointer to the data layer for the parent property */
	double dx; /*!< spatial increment for the calculation of the gradient */

public:
	/*! \brief Default constructor */
	GradientDataLayer() : DataLayer<T>() {};
	/*! \brief Constructor with all necessary information */
	GradientDataLayer(string name, DataLayer<T>* primary, const double ddx)
	: DataLayer<T>(name), parent(primary), dx(ddx) {
		// nothing more to do
	}
	/*! \brief Destructor */
	~GradientDataLayer(){};

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);

	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);

	void setValueAt(FFPoint p ,  double vt, T value){};
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);

	/*! \brief getter to the desired data (should not be used) */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a given array (should not be used) */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	/*! \brief print the related data (should not be used) */
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);

};

template<typename T>
T GradientDataLayer<T>::getValueAt(FireNode* fn){
	/* Computing the gradient between the next and present location */
	T currentValue = parent->getValueAt(fn);
	T nextValue;
	FFPoint nextLoc = fn->getLoc() + dx*(fn->getNormal().toPoint());
	nextValue = parent->getValueAt(nextLoc,fn->getUpdateTime());
	return (nextValue - currentValue)/dx;
}

template<typename T>
T GradientDataLayer<T>::getValueAt(FFPoint loc, const double& time){
	cout << "this call shouln't be used in a gradient layer" << endl;
	return 0.;
}

template<typename T>
size_t GradientDataLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t GradientDataLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
void GradientDataLayer<T>::getMatrix(
		FFArray<T>** matrix, const double& time){
}

template<typename T>
void GradientDataLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	// writing the incoming data in matrix
	// should not be done with this type of layer
}

template<typename T>
string GradientDataLayer<T>::print(){
	return "";
}

template<typename T>
void GradientDataLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){

}

}

#endif /* GRADIENTDATALAYER_H_ */
