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

#ifndef FFPOINT_H_
#define FFPOINT_H_


#include "include/Futils.h"


using namespace std;

namespace libforefire{
/*! \class FFPoint
 * \brief 3d points for LibForeFire
 *
 *  FFPoint defines 3d points and methods associated with it.
 */

class FFPoint {


	static const double Pi;
public:
	double x; /*!< x coordinate*/
	double y; /*!< y coordinate*/
	double z; /*!< z coordinate*/

	/*! \brief Default constructor */
	FFPoint();
	/*! \brief 3d Constructor */
	FFPoint(double, double, double);
	/*! \brief Destructor */
	virtual ~FFPoint();
	/*! \brief Copy-constructor
	 *  \param[in] 'p' : point to be copied */
	FFPoint(const FFPoint& p);

	/*!  \brief overloaded operator +  */
	friend const FFPoint operator+(const FFPoint&, const FFPoint&);
	/*!  \brief overloaded operator -  */
	friend const FFPoint operator-(const FFPoint&, const FFPoint&);
	/*!  \brief overloaded operator *, multiplication by a double  */
	friend const FFPoint operator*(const double&, const FFPoint&);
	/*!  \brief overloaded operator +=  */
	friend FFPoint& operator+=(FFPoint&, const FFPoint&);
	/*!  \brief overloaded operator -=  */
	friend FFPoint& operator-=(FFPoint&, const FFPoint&);
	/*!  \brief overloaded operator *=, multiplication by a double  */
	friend FFPoint& operator*=(FFPoint&, const double&);
	/*!  \brief overloaded operator ==  */
	friend int operator==(const FFPoint&, const FFPoint&);
	/*!  \brief overloaded operator !=  */
	friend int operator!=(const FFPoint&, const FFPoint&);

	/*!  \brief mutator of the variable 'x'  */
	void setX(const double&);
	/*!  \brief mutator of the variable 'y'  */
	void setY(const double&);
	/*!  \brief mutator of the variable 'z'  */
	void setZ(const double&);
	/*!  \brief mutator of the point itself  */
	void setLoc(const double&, const double&, const double& = 0);
	void setLoc(FFPoint);

	/*!  \brief Accessor to 'x'  */
	double& getX();
	/*!  \brief Accessor to 'y'  */
	double& getY();
	/*!  \brief Accessor to 'z'  */
	double& getZ();
	/*!  \brief Accessor to the point itself  */
	FFPoint& getLoc();

	/*! \brief norm of the point
	 * \return norm of the point */
	double norm();

	/*! \brief distance with another point
	 * \param[in] 'p' : other point
	 * \return distance between the two points  */
	double distance(FFPoint);
	double distance2D(FFPoint);
	double distance2D(double&, double&);

	/*! \brief scalar product with another point
	 * \param[in] 'p' : other point
	 * \return scalar product between the two vectors */
	double scalarProduct(FFPoint);

	/*! \brief cross product with another point
	 * \param[in] 'p' : other point
	 * \return cross product of the two points  */
	FFPoint crossProduct(FFPoint);

	/*! \brief 2D angle with another point
	 * \param[in] 'p' : other point
	 * \return angle between the two vectors  */
	double angle2D(FFPoint);

	/*! \brief signed distance between a point and a polygon */
	double signedDistanceToPolygon(size_t&, double*, double*, bool);

	/*! \brief distance between a point and a segment */
	double distanceToSegment(double&, double&, double&, double&);

	/*! \brief Point in polygon algorithm */
	bool pointInPolygon(size_t&, double*, double*);

	/*! \brief printing function */
	string print();
};

}

#endif /* FFPOINT_H_ */
