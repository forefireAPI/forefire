/**
 * @file ParallelException.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "ParallelException.h"

namespace libforefire {

ParallelException::ParallelException(string how, string at)
: reason(how), where(at) {
	ostringstream oss;
	oss<<"PROBLEM in "<<where<<endl<<reason;
	msg = oss.str();
}

ParallelException::~ParallelException() throw() {
}

TopologicalException::TopologicalException(string how, string at)
: reason(how), where(at) {
	cout<<"raising topological exception"<<endl;
	ostringstream oss;
	oss<<"PROBLEM in "<<where<<" for "<<reason<<endl;
	msg = oss.str();
}

TopologicalException::~TopologicalException() throw() {
}

}
