/**
 * @file TimeTable.cpp
 * @brief Implements the methods of the TimeTable class.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "TimeTable.h"
using namespace std;

namespace libforefire {

TimeTable::TimeTable() {
	commonInitialization();

}

TimeTable::TimeTable(FFEvent* ev) {
	head = nullptr;
	commonInitialization();
	insert(ev);
}

TimeTable::~TimeTable() {
	while ( size() > 0 ) dropEvent(head);
	
}

void TimeTable::commonInitialization(){
	incr = 0;
	decr = 0; 
}

void TimeTable::setHead(FFEvent* newHead){
	head->getPrev()->setNext(newHead);
	newHead->setPrev(head->getPrev());
	head = newHead;
}

FFEvent* TimeTable::getHead(){
	return head;
}

double TimeTable::getTime(){
	if ( !head ){
		return -numeric_limits<double>::infinity();
	}
	if(size()==0 ) {
		return -numeric_limits<double>::infinity();
	}
	return head->getTime();
}

void TimeTable::increment(){
	incr++;
}

void TimeTable::decrement(){
	decr++;
}

size_t TimeTable::size(){
	return incr-decr;
}

FFEvent* TimeTable::getUpcomingEvent(){
	FFEvent* upEvent = head;
	if ( size() > 1 ) {
		setHead(head->getNext());
		decrement();
	} else if ( size() == 1 ) {
		// this is the only event left
		decrement();
	} else {
		// no events left to be treated (size=0)
		//cout << "ForeFire simulation ended with no more event to be treated" << endl;
		upEvent = nullptr;
	}
	return upEvent;
}

void TimeTable::insertBefore(FFEvent* newEv){
	// checking the event consistency
	double evTime = newEv->getTime();
	if ( evTime < 0. ){
		// deleting the event
		delete newEv;
		return;
	}
	if ( size() == 0 ) {
		// First element of the timetable
		head = newEv;
		head->setNext(newEv);
		head->setPrev(newEv);
	} else {
		// possible insertion at the head or the tail
		// if not, searching for the time of insertion
		if ( evTime < head->getTime() + EPSILONT ){
			// inserting the event at the head
			head->insertBefore(newEv);
			head = newEv;
		} else if ( evTime > head->getPrev()->getTime() ){
			// inserting the event at the tail
			head->insertBefore(newEv);
		} else {
			// searching for the time of insertion
			// starting from the head
			FFEvent* tmpEv = head;
			while ( evTime > tmpEv->getTime() + EPSILONT ){
				tmpEv = tmpEv->getNext();
			}
			tmpEv->insertBefore(newEv);
		}
	}
	increment();
}

void TimeTable::insert(FFEvent* newEv){
	// checking the event consistency
	double evTime = newEv->getTime();
	if ( evTime == numeric_limits<double>::infinity() ){
		// deleting the event
		delete newEv;
		return;
	}
	if ( size() == 0 ) {
		// First element of the timetable
		head = newEv;
		head->setNext(newEv);
		head->setPrev(newEv);
	} else {
		// possible insertion at the head or the tail
		// if not, searching for the time of insertion
		if ( evTime < head->getTime() - EPSILONT ){
			// inserting the event at the head
			head->insertBefore(newEv);
			head = newEv;
		} else if ( evTime >= head->getPrev()->getTime() - EPSILONT ){
			// inserting the event at the tail
			FFEvent* tmpEv = head->getPrev();
			tmpEv->insertAfter(newEv);
		} else {
			// searching for the time of insertion
			// starting from the head
			FFEvent* tmpEv = head;
			while ( evTime > tmpEv->getTime() - EPSILONT ){
				tmpEv = tmpEv->getNext();
			}
			tmpEv = tmpEv->getPrev();
			tmpEv->insertAfter(newEv);
		}
	}
	increment();
}

void TimeTable::dropEvent(FFEvent* ev){
	if ( !head ) return;
	if ( size() > 1 ) {
		// classical removing
		ev->getPrev()->setNext(ev->getNext());
		ev->getNext()->setPrev(ev->getPrev());
		if ( ev == head ){
			head = head->getNext();
		}
	} else {
		head = 0;
	}
	delete ev;
	decrement();
}

void TimeTable::dropAtomEvents(ForeFireAtom* atom){
	if ( !head ) return;
	FFEvent* tmpEvNext;
	// removing possible events at head
	while ( head != 0 and head->getAtom() == atom ) dropEvent(head);
	FFEvent* tmpEv = head->getNext();
	/* scanning all the events to see if they're
	 * related to the searched ForeFireAtom */
	while ( tmpEv != head ) {
		tmpEvNext = tmpEv->getNext();
		if ( tmpEv->getAtom() == atom ) dropEvent(tmpEv);
		tmpEv = tmpEvNext;
	}
}

string TimeTable::print(){
	if ( !head ) return "";
	ostringstream oss;
	oss << "TIMETABLE" << endl;
	FFEvent* tmpEv = head;
	oss << tmpEv->getAtom()->toString() << " at " << tmpEv->getTime()
			<<" at "<< tmpEv->getAtom() << endl;
	while ( tmpEv->getNext() != head ){
		tmpEv = tmpEv->getNext();
		oss << tmpEv->getAtom()->toString() << " at " << tmpEv->getTime()
				<<" at "<< tmpEv->getAtom() << endl;
	}
	oss << "END TIMETABLE" << endl;
	return oss.str();
}

void TimeTable::clear() {
    while (head != nullptr) {
        FFEvent* ev = head;
        // dropEvent sets head = nullptr if it's the last element
        dropEvent(ev);
    }
	
	commonInitialization();
	if(head ) {
		cout << "Error: head is not null after clearing the timetable." << endl;
	}
}

}
