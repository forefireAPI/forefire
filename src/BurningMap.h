/**
 * @file BurningMap.h
 * @brief Definitions for the class containing the arival time data for the whole domain.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef BURNINGMAP_H_
#define BURNINGMAP_H_

#include "include/Futils.h"
#include "FFPoint.h"
#include "FFArrays.h"
#include "include/FFConstants.h"

using namespace std;

namespace libforefire
{

/*! \class BurningMap
 * \brief Eulerian matrices for burning area computations
 *
 *  BurningMap defines the eulerian matrix of arrival times to localize
 *  the burning area and compute the resulting fluxes.
 *
 */
class BurningMap
{

	size_t sizeX, sizeY; /*!< size of the burning matrix */
	FFPoint SWCorner, NECorner; /*!< corner points of the simulated domain */
	double dx, dy; /*!< spatial resolution of burning matrix */

	FFArray<double>* arrivalTimeMap; /*!< matrix of the arrival times of the fire */

public:

	/*! \brief Default constructor */
	BurningMap();
	/*! \brief Constructor with specified corner points and spatial resolution */
	BurningMap(const FFPoint&, const FFPoint&
			, const size_t&, const size_t&);
	/*! \brief Destructor */
	virtual
	~BurningMap();

	/*!  \brief accessor of the data contained at position (i,j)  */
	double operator() (size_t, size_t = 0) const;
	/*!  \brief mutator of the data contained at position (i,j)  */
	double& operator() (size_t, size_t = 0);

	/*!  \brief pointer to the data of the matrix of arrival times */
	FFArray<double>* getMap();

	/*!  \brief getters of the sizes of the matrix of arrival times */
	size_t getSizeX();
	size_t getSizeY();

	/*!  \brief getters of the resolution of the matrix of arrival times */
	double getDx();
	double getDy();
	double maxTime();
	void loadBin(std::ifstream&  );
	/*!  \brief getters of the location of the center of a given cell */
	FFPoint getCenter(const size_t&, const size_t&);

	/*! \brief mutator of the burning end time at a given position  */
	void setBurning(FFPoint&, const double&);

	/*! \brief boolean to decide is something is still burning  */
	bool nothingBurning(const double&);

	string toString(const double&);
};

}

#endif /* BURNINGEULERIANMAP_H_ */
