/**
 * @file FireNodeData.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
