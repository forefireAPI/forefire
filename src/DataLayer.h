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
