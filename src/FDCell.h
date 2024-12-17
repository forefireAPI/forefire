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

#ifndef FDCELL_H_
#define FDCELL_H_

#include "FFPoint.h"
#include "FFVector.h"
#include "FireNode.h"
#include "BurningMap.h"
#include "ParallelException.h"
#include "ArrayDataLayer.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire {

class FireDomain;

class FDCell {

	FireDomain* domain; /*!< FireDomain containing the cells */
	FFPoint SWCorner, NECorner; /*!< corners of the cell */
	size_t globalI, globalJ; /*!< indices of the cell in the FireDomain */

	// Internal variables for the burning matrices
	size_t mapSizeX, mapSizeY, mapSize; /*!< size of the matrix */
	double dx, dy; /*!< resolution of the matrix */

	BurningMap* arrivalTimes; /*!< Burning map inside the cell */
	bool allocated; /*!< boolean for the allocation of the burning map */

	list<FireNode*>::iterator ifn;

	static const double infinity;

public:

	static bool outputs; /*! boolean for outputs */
	size_t toDumpDomainID;/*! for paralleIO assigned Domain */
	bool allDumped; /*! boolean to state it has fully dumped */
	
    bool hasPassedCom; /*!< boolean to state if has already been collected for com parallel*/
	list<FireNode*> fireNodes; /*!< firenodes present in the cell */

	/*! \brief default constructor */
	FDCell(FireDomain* = 0, size_t = 0, size_t = 0);
	/*! \brief destructor */
	virtual ~FDCell();

	/*!  \brief overloaded operator ==  */
	friend int operator==(const FDCell&, const FDCell&);
	/*!  \brief overloaded operator !=  */
	friend int operator!=(const FDCell&, const FDCell&);

	/*! \brief mutator of the FireDomain */
	void setDomain(FireDomain*);

	/*! \brief mutator of the corners */
	void setCorners(const FFPoint&, const FFPoint&);

	/*! \brief mutator of the global coordinates */
	void setGlobalCoordinates(const size_t&, const size_t&);

	/*! \brief mutator of the size of the burning matrix */
	void setMatrixSize(const size_t&, const size_t&);

	/*! \brief mutator of the burning matrix */
	void setArrivalTime(const size_t&, const size_t&, double);

	/*! \brief getter of the FireDomain */
	FireDomain* getDomain();

	double getArea();

	double getBmapElementArea();

	/*! \brief getter of the map of arrival times */
	BurningMap* getBurningMap();

	/*! \brief accessors to the global coordinates */
	size_t getI();
	size_t getJ();

	/*! \brief accessors to the global coordinates */
	FFPoint& getSWCorner();
	FFPoint& getNECorner();

	/*! \brief accessors to the burning matrix and properties */
	FFArray<double>* getBurningMatrix();
	size_t getBMapSizeX();
	size_t getBMapSizeY();

	/*! \brief accessor to the burning matrix */
	double getArrivalTime(const size_t&, const size_t&);

	/*! \brief number of firenodes */
	size_t getNumFN();

	/*! \brief finding the firenode with a given Id */
	FireNode* getFirenodeByID(const long&);
	FireNode* getFirenodeByID(const double&);

	/*! \brief adding a firenode into that cell */
	void addFireNode(FireNode*);
	/*! \brief removing a firenode into that cell */
	void removeFireNode(FireNode*);

	/*! \brief comStatus for Parallel */
	bool hasFiredInHalo();
	bool isActive();
	bool isActiveForDump();
	void setIfAllDumped();
	void setFiredInHalo(bool val);
	
	/*! \brief computing the burning ratio of the cell */
	double getBurningRatio(const double&);

	/*! \brief computing the max ROS of the cell */
	double getMaxSpeed(const double&);

	/*! \brief computing a specified flux on the cell */
	double applyModelsOnBmap(string, const double&, const double&,int* );

	/*! \brief computes a count of each model active on a layer*/
	int activeModelsOnBmap(string  , const double& , int*);

	/*! \brief interpolating the map of arrival times from a prescribed one */
	void interpolateArrivalTimes(Array2DdataLayer<double>*
			, const int&, const int&, const int&);

	/*! \brief validating the topology for each firenode in the cell */
	void validateTopology(string);

	/*! \brief managing the trash cell of the domain */
	void makeTrash();
	void loadBin(std::ifstream&  );
	void setBMapValues(const double* );
	/*! \brief string for cell information */
	string toString();

	/*! \brief string for model information */
	string getFluxModelName(int );

};

}

#endif /* FDCELL_H_ */
