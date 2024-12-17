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

#ifndef FFVECTOR_H_
#define FFVECTOR_H_

#include "include/Futils.h"
#include "FFPoint.h"

using namespace std;

namespace libforefire{

/*! \class FFVector
 * \brief 3d vectors for LibForeFire
 *
 *  FFPoint defines 3d vectors and methods associated with it.
 */
class FFVector {
	double vx; /*!< component along x axis */
	double vy; /*!< component along y axis */
	double vz; /*!< component along z axis */

	static const double epsilonv;
	static const double Pi;
public:
	/*! \brief Default constructor */
	FFVector();
	/*! \brief 2d Constructor (vz=0.) */
	FFVector(double, double);
	/*! \brief 3d Constructor */
	FFVector(double, double, double);
	/*! \brief Constructor from two FFPoints */
	FFVector(FFPoint, FFPoint);
	/*! \brief Destructor */
	virtual ~FFVector();
	/*! \brief Copy-constructor */
	FFVector(const FFVector&);

	/*!  \brief overloaded operator +  */
	friend const FFVector operator+(const FFVector&, const FFVector&);
	/*!  \brief overloaded operator -  */
	friend const FFVector operator-(const FFVector&, const FFVector&);
	/*!  \brief overloaded operator *, multiplication by a double  */
	friend const FFVector operator*(const double&, const FFVector&);
	/*!  \brief overloaded operator +=  */
	friend FFVector& operator+=(FFVector&, const FFVector&);
	/*!  \brief overloaded operator -=  */
	friend FFVector& operator-=(FFVector&, const FFVector&);
	/*!  \brief overloaded operator *=, multiplication by a double  */
	friend FFVector& operator*=(FFVector&, const double&);
	/*!  \brief overloaded operator ==  */
	friend int operator==(const FFVector&, const FFVector&);
	/*!  \brief overloaded operator !=  */
	friend int operator!=(const FFVector&, const FFVector&);

	/*!  \brief accessor to 'vx' */
	double getVx();
	/*!  \brief accessor to 'vy' */
	double getVy();
	/*!  \brief accessor to 'vz' */
	double getVz();
	/*!  \brief accessor to vector itself */
	FFVector& getVec();

	/*!  \brief mutator of the variable 'vx'  */
	void setVx(const double&);
	/*!  \brief mutator of the variable 'vy'  */
	void setVy(const double&);
	/*!  \brief mutator of the variable 'vz'  */
	void setVz(const double&);
	/*!  \brief mutator of the vector itself  */
	void setVec(const double&, const double&, const double&);

	/*! \brief Cast into a 'FFpoint' */
	FFPoint toPoint();

	/*! \brief computing the angle in a spherical representation */
	double toAngle();

	/*! \brief norm of the vector
	 * \return norm of the vector */
	double norm();
	/*! \brief normalization of the vector */
	void normalize();
	/*! \brief normalized version of the vector
	 * \return a normalized vector with the same direction */
	FFVector normed();
	/*! \brief scalar product with another vector
	 * \param[in] 'vec' : other vector
	 * \return scalar product between the two vectors */
	double scalarProduct(const FFVector& vec);
	/*! \brief vector product with another vector
	 * \param[in] 'vec' : other vector
	 * \return vector product between the two vectors */
	FFVector crossProduct(const FFVector& vec);

	/*! \brief printing function */
	string print();

};

}

#endif /* FFVECTOR_H_ */
