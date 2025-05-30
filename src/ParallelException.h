/**
 * @file ParallelException.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
