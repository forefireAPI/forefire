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

#ifndef FIRENODEDATA_H_
#define FIRENODEDATA_H_

#include "FFPoint.h"
#include "FFVector.h"
#include "FireNode.h"

namespace libforefire {

class FireNodeData {

public:

	double id;
	double posX;
	double posY;
	double velX;
	double velY;
	double time;
	double pid;
	double nid;
	double fdepth;
	double curvature;
	string state;

	FireNodeData();
	FireNodeData(const double&, const double&, const double&
			, const double, const double&, const double&
			, const double& = 0, const double& = 0
			, const double& = 0, const double& = 0, string = "init");
	FireNodeData(FireNode*);
	virtual ~FireNodeData();

	double distance(FireNodeData*);
	double distance(FireNode*);
	double distance(FFPoint&);
	double distance(const double&, const double&);

	string toString();
};

}

#endif /* FIRENODEDATA_H_ */
