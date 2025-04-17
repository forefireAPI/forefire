/**
 * @file DataLayer.h
 * @brief Abstract layer for Layer classes
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef DATALAYER_H_
#define DATALAYER_H_

#include "FireNode.h"
#include "FFArrays.h"

using namespace std;

namespace libforefire {

class PropagationModel;
class FluxModel;

/*! \class DataLayer
 * \brief Purely virtual template data layer object
 *
 *  DataLayer implements common behavior for all data layers
 */
template<typename T> class DataLayer {

	string key; /*!< key to the data layer */

public:
	/*! \brief Default constructor */
	DataLayer(){};
	/*! \brief Constructor with key */
	DataLayer(string name) : key(name){};
	virtual ~DataLayer(){};

	/*! \brief getter to the key of the layer */
	string getKey(){return key;}
	/*! \brief setter of the key of the layer */
	void setKey(string name){key = name;}

	/*! \brief computes the value at a given firenode */
	virtual T getValueAt(FireNode*) = 0;
	/*! \brief computes the value at a given location and time */
	virtual T getValueAt(FFPoint, const double&) =0;

	virtual double getDx() = 0;
	virtual double getDy() = 0;
	virtual double getDz() = 0;
	virtual double getOriginX() = 0;
	virtual double getOriginY() = 0;
	virtual double getOriginZ() = 0;
	virtual double getWidth() = 0;
	virtual double getHeight() = 0;
	virtual double getDepth() = 0;
	
	
	/*! \brief set the value at a given location and time */

	virtual void setValueAt(FFPoint ,  double , T ) = 0;
	/*! \brief directly stores the desired values in a given array */
	virtual size_t getValuesAt(FireNode*, PropagationModel*, size_t) = 0;

	/*! \brief directly stores the desired values in a given array */
	virtual size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t) = 0;

	/*! \brief getter to the desired data */
	virtual void getMatrix(FFArray<T>**, const double&) = 0;
	/*! \brief stores data from a given array */
	virtual void setMatrix(string&, double*, const size_t&, size_t&, const double&) = 0;

	/*! \brief print the related data */
	virtual string print() = 0;
	virtual void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&) = 0;


};

}

#endif /* DATALAYER_H_ */
