/**
 * @file FFEvent.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FFEvent.h"

namespace libforefire {

FFEvent::FFEvent() {
	eventTime = 0.;
}

FFEvent::FFEvent(ForeFireAtom* m) {
	eventTime = m->getTime();
	atom = m;
	input = true;
	output = true;
}

FFEvent::FFEvent(ForeFireAtom* m, const string& type) {
	eventTime = m->getTime();
	atom = m;
	input = false;
	output = false;
	if ( type == "input" ) {
		input = true;
		return;
	} else if ( type == "output" ) {
		output = true;
		return;
	} else if ( type == "all" ) {
		input = true;
		output = true;
		return;
	} else if ( type == "none" ) {
		// nothing to do
		return;
	} else {
		cout << "event type " << type << " not recognized" << endl;
	}
}

FFEvent::FFEvent(ForeFireAtom* m, const double& time, const string& type) {
	eventTime = time;
	atom = m;
	input = false;
	output = false;
	if ( type == "input" ) {
		input = true;
		return;
	} else if ( type == "output" ) {
		output = true;
		return;
	} else if ( type == "none" ) {
		// nothing to do
		return;
	} else {
		cout << "event type " << type << " not recognized" << endl;
	}
}

FFEvent::FFEvent(const FFEvent& event) {
	eventTime = event.eventTime;
	atom = event.atom;
	next = event.next;
	prev = event.prev;
	input = event.input;
	output = event.output;
}

FFEvent::~FFEvent() {
	// nothing to do
}

// Overloading operators
int operator==(const FFEvent& left, const FFEvent& right){
	return left.eventTime == right.eventTime and left.atom == right.atom
			and left.input == right.input and left.output == right.output;
}
int operator!=(const FFEvent& left, const FFEvent& right){
	return !(left==right);
}

double FFEvent::getTime() const{
	return eventTime;
}

ForeFireAtom* FFEvent::getAtom(){
	return atom;
}

FFEvent* FFEvent::getNext(){
	return next;
}

FFEvent* FFEvent::getPrev(){
	return prev;
}

void FFEvent::setNewTime(double t){
	eventTime = t;
}

void FFEvent::setNext(FFEvent* ev){
	next = ev;
}

void FFEvent::setPrev(FFEvent* ev){
	prev = ev;
}

void FFEvent::insertBefore(FFEvent* ev){
	prev->setNext(ev);
	ev->setPrev(prev);
	setPrev(ev);
	ev->setNext(this);
}

void FFEvent::insertAfter(FFEvent* ev){
	next->setPrev(ev);
	ev->setNext(next);
	setNext(ev);
	ev->setPrev(this);
}

void FFEvent::makeInput(FFEvent* ev){
	input = true;
	output = false;
}

void FFEvent::makeOutput(FFEvent* ev){
	input = false;
	output = true;
}

string FFEvent::toString(){
	return atom->toString();
}

}
