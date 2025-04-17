/**
 * @file AtmosphericData.h
 * @brief Definition for Class for data exchange in atmospheric coupling
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef ATMOSPHERICDATA_H_
#define ATMOSPHERICDATA_H_

#include "FFArrays.h"

namespace libforefire {

/*! \class AtmosphericData
 * \brief Storage for all atmospheric data in fire/atmospheric coupling
 *
 *  Atmospheric data is a container for all the atmospheric variables
 *  needed in a coupled fire/atmosphere simulation
 */
class AtmosphericData {
public:
	/*! \brief Default constructor */
	AtmosphericData();
	/*! \brief Destructor */
	virtual ~AtmosphericData();

	// Information to be provided by the atmospheric model
	double currentTime; /*!< time at the start of the atmospheric time-step */
	FFArray<double>* windU; /*!< Surface wind field, U component (start of the time step) */
	FFArray<double>* windV; /*!< Surface wind field, V component (start of the time step) */

	double oldTime; /*!< time at the end of the atmospheric time-step */
	FFArray<double>* oldWindU; /*!< Surface wind field, U component (end of the time step) */
	FFArray<double>* oldWindV; /*!< Surface wind field, V component (end of the time step) */

	FFArray<double>* topography; /*!< Topography provided by the atmospheric model for real cases */
	FFArray<double>* plumeTopHeight; 
	FFArray<double>* plumeBottomHeight; 
	FFArray<double>* smokeAtGround; 
	FFArray<double>* tke; 


	FFArray<double>* oldplumeTopHeight; 
	FFArray<double>* oldplumeBottomHeight; 
	FFArray<double>* oldsmokeAtGround; 
	FFArray<double>* oldtke; 

	/*! \brief Setting all the size parameters according to atmospheric simulation */
	void setSize(const size_t&, const size_t&);

	/*! \brief Getting all the size parameters */
	size_t getSize();
	size_t getSizeX();
	size_t getSizeY();

};

}

#endif /* ATMOSPHERICDATA_H_ */
