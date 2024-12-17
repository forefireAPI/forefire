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

#ifndef FOREFIREATOM_H_
#define FOREFIREATOM_H_

#include "include/Futils.h"

using namespace std;

namespace libforefire{

/*! \class ForeFireAtom
 * \brief Abstract class for Atoms
 *
 *  A 'ForeFireAtom' is an object that evolves
 *  with time. All the objects of LibForeFire
 *  that evolves with time thus inherit from this
 *  class. The main attribute is the current
 *  time 'time', and the class implements the
 *  virtual functions 'timeAdvance()', which
 *  computes the properties of the object at
 *  the next step as well as the update time,
 *  and 'update()' that setVal the next properties
 *  to the current ones.
 */

class ForeFireAtom {
private:

	static long instanceNRCount; /*!< Instance Count */

	double time; /*!< current time of the object */
	double updateTime; /*!< next update time */

	long numID;
	short getModelMask(){return 40;};
	int domainMult(){return 10000000;};

public:
	/*! \brief Default constructor */
	ForeFireAtom(double t) : time(t), updateTime(t), numID() {
/*		getNewID(domainId);*/
	};

	/*! \brief Destructor */
	virtual ~ForeFireAtom(){};

	/*! \brief Accessors to the current and update times */
	double& getTime(){
		return time;
	}
	double& getUpdateTime(){return updateTime;};
	/*! \brief Mutators to the current and update times */
	void setTime(double t){time = t;};
	void setUpdateTime(double ut){
	//	if ( updateTime == ut ) cerr<<"update time is equal to current update time"<<endl;
		updateTime = ut;
	}
	/*! \brief Return long object ID */
	long getID(){return numID;};
	/*! \brief Set long object ID */
	void setID(long givenId){
		numID = givenId;
	}
	/*! \brief Set long object ID */
	void setID(const long& domainID, const long& shortID){
		setID(getIDfromLongs(domainID, shortID));
	}


	long getDomainID(const long& lid){
		return long((lid&0xFFFFFF0000000000)>>getModelMask());
	}
	long getDomainID(const double& did){
		return long((getIDfromDouble(did)&0xFFFFFF0000000000)>>getModelMask());
	}
	long getShortID(const long& lid){
		return long(lid&0x000000FFFFFFFFFF);
	}
	long getShortID(const double& did){
		return long(getIDfromDouble(did)&0x000000FFFFFFFFFF);
	}

	long getDomainID(){return getDomainID(numID);}
	long getShortID(){return getShortID(numID);}

	double getIDtoDouble(){
		return (double)(getShortID()+domainMult()*getDomainID());
	}
	long getIDfromLongs(const long& domainID, const long& shortID){
		long tmpnumID = domainID;
		tmpnumID <<=getModelMask() ;
		tmpnumID |= shortID;
		return tmpnumID;
	}
	long getIDfromDouble(const double& did){
		return getIDfromLongs((long) did/domainMult(),(long) did%domainMult());
	}
	void getNewID(const long& domainId){
		numID = getIDfromLongs(domainId,instanceNRCount++);
	}

	/*! \brief Pure virtual function for inpus */
	virtual void input() = 0;
	/*! \brief Pure virtual function for object update */
	virtual void update() = 0;
	/*! \brief Pure virtual function for advancing the object in time */
	virtual void timeAdvance() = 0;
	/*! \brief Output virtual function */
	virtual void output() = 0;

	/*! \brief print function */
	virtual string toString() = 0;

};

}

#endif /* FOREFIREATOM_H_ */
