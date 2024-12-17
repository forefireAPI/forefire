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

#ifndef PARALLELEXCEPTION_H_
#define PARALLELEXCEPTION_H_

#include <iostream>
#include <sstream>
#include <exception>

using namespace std;

namespace libforefire {

class ParallelException : public exception {
	string reason;
	string where;
	string msg;
public:
	ParallelException(string, string);
	virtual ~ParallelException() throw();

	virtual const char* what() const throw(){
	    return msg.c_str();
	}
};

class TopologicalException : public exception {
	string reason;
	string where;
	string msg;
public:
	TopologicalException(string, string);
	virtual ~TopologicalException() throw();

	virtual const char* what() const throw(){
	    return msg.c_str();
	}
};

}

#endif /* PARALLELEXCEPTION_H_ */
