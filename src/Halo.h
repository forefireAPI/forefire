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

#ifndef HALO_H_
#define HALO_H_

#include "FFPoint.h"
#include "FDCell.h"
#include "FireNodeData.h"

namespace libforefire {

class FireDomain;

class Halo {

	string name;

	FireDomain* domain;

	size_t iComStart, jComStart;
	size_t iComEnd, jComEnd;
	size_t iComIncrement, jComIncrement;
	size_t iCurrent, jCurrent, kCurrent;

	bool init;
	bool isValid(list<FireNodeData*>&);

	static const string FireNodesId;
	static const string FireNodesPosX;
	static const string FireNodesPosY;
	static const string FireNodesVelX;
	static const string FireNodesVelY;
	static const string FireNodesTime;

public:

	static bool outputs; /*! boolean for outputs */

	list<FDCell*> cells;

	list<FireNodeData*> nodeDataList;

	bool isActive;
	bool hasTriggered;
	bool verticalHalo;
	double limSup;
	double limInf;
	double chainComSpread;
	

	Halo();
	Halo(string, FireDomain*, list<FDCell*>&
			, const size_t&, const size_t&
			, const size_t&, const size_t&
			, const size_t&, const size_t&);
	virtual ~Halo();

	void writeHaloNodeList(const double&, const double&);
	void readHaloNodeList(const double&, const double&);

	FireNode* getFireNodebyID(const double&);

	void initializePositionInMatrix();
	void getNewPositionInMatrix(size_t&, size_t&, size_t&);

	void setLimits(double, double);

	string getName();

};

}

#endif /* HALO_H_ */
