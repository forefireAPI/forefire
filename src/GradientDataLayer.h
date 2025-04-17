/**
 * @file GradientDataLayer.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
			
	double getDx(){ return parent->getDx(); };
	double getDy(){ return parent->getDy(); };
	double getDz(){ return parent->getDz(); };
	double getOriginX(	){ return parent->getOriginX(); };
	double getOriginY(){ return parent->getOriginY(); };
	double getOriginZ(){ return parent->getOriginZ(); };
	double getWidth(){ return parent->getWidth(); };
	double getHeight(){ return parent->getHeight(); };
	double getDepth(){ return parent->getDepth(); };

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
T GradientDataLayer<T>::getValueAt(FFPoint loc, const double& time) {
    // Get the current value at the given location.
    T currentValue = parent->getValueAt(loc, time);

    // Prepare the eight unit directions.
    vector<FFPoint> directions;
    directions.push_back(FFPoint(1, 0,0));
    directions.push_back(FFPoint(1, 1,0));
    directions.push_back(FFPoint(0, 1,0));
    directions.push_back(FFPoint(-1, 1,0));
    directions.push_back(FFPoint(-1, 0,0));
    directions.push_back(FFPoint(-1, -1,0));
    directions.push_back(FFPoint(0, -1,0));
    directions.push_back(FFPoint(1, -1,0));


    // Compute the gradient in each direction and find the maximum.
    T maxGradient = 0.0;
    for (size_t i = 0; i < directions.size(); ++i) {
        FFPoint nextLoc = loc + (10 * directions[i]);
        T neighborValue = parent->getValueAt(nextLoc, time);
        // Since the displacement magnitude is dx in every case (after normalization),
        // the gradient is computed as the difference divided by dx.
        T gradient = (neighborValue - currentValue) / 10;
        if (gradient > maxGradient) {
            maxGradient = gradient;
        }
    }
    return maxGradient;
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
