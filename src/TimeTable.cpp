/*
   Copyright (C) 2012 ForeFire Team, SPE, CNRS/Universita di Corsica.
   Licensed under the terms of the GNU Lesser General Public License.
*/

#include <iostream>
#include <set>
#include <limits>
#include <sstream>
#include <stdexcept>
#include "TimeTable.h"

using namespace std;

namespace libforefire {




//////////////////////////
// TimeTable functions
//////////////////////////

TimeTable::TimeTable() : incr(0), decr(0) { }

TimeTable::TimeTable(FFEvent* ev) : incr(0), decr(0) {
    insert(ev);
}

TimeTable::~TimeTable() {
    // Delete all remaining events.
    for (auto ev : events) {
        delete ev;
    }
    events.clear();
}

void TimeTable::setHead(FFEvent* newHead) {
    // In the original code, setHead could reset the head pointer arbitrarily.
    // In a multiset-based implementation, the ordering is fixed by the event times.
    // We only check that newHead would be the smallest element.
    if (!newHead)
        return;
    if (!events.empty()) {
        FFEvent* currentHead = *events.begin();
        if (currentHead != newHead) {
            // If newHead's time does not match the minimum, we cannot reorder.
            // For compatibility, we throw an exception.
            throw runtime_error("setHead: Cannot override multiset ordering");
        }
    }
    // Otherwise, do nothing.
}

FFEvent* TimeTable::getHead() {
    if (events.empty())
        return nullptr;
    return *events.begin();
}

double TimeTable::getTime() {
    if (events.empty())
        return -numeric_limits<double>::infinity();
    return (*events.begin())->getTime();
}

void TimeTable::increment() {
    ++incr;
}

void TimeTable::decrement() {
    ++decr;
}

size_t TimeTable::size() {
    // The counter-based size in the original is simply incr - decr.
    // It should correspond to events.size() here.
    return events.size();
}

FFEvent* TimeTable::getUpcomingEvent() {
    if (events.empty()) {
        cout << "ForeFire simulation ended with no more event to be treated" << endl;
        return nullptr;
    }
    // Retrieve the event with the smallest time.
    auto it = events.begin();
    FFEvent* nextEv = *it;
    events.erase(it);
    decrement();
    return nextEv;
}

void TimeTable::insertBefore(FFEvent* newEv) {
    // In the original implementation, insertBefore would insert an event
    // before the head if its time qualifies.
    // Here, we simply check the event's time and insert it.
    double evTime = newEv->getTime();
    if (evTime < 0.0) {
        // Delete the event if its time is negative.
        delete newEv;
        return;
    }
    // Insert into the multiset.
    events.insert(newEv);
    increment();
    // If the new event qualifies as the new head, there is no need to alter ordering.
    // The multiset automatically orders elements by time.
}

void TimeTable::insert(FFEvent* newEv) {
    // Check for event consistency.
    double evTime = newEv->getTime();
    if (evTime == numeric_limits<double>::infinity()) {
        delete newEv;
        return;
    }
    events.insert(newEv);
    increment();
}

void TimeTable::dropEvent(FFEvent* ev) {
    // Erase one matching event.
    auto it = events.find(ev);
    if (it != events.end()) {
        events.erase(it);
        delete ev;
        decrement();
    }
}

void TimeTable::dropAtomEvents(ForeFireAtom* atom) {
    // Remove all events associated with the given atom.
    for (auto it = events.begin(); it != events.end(); ) {
        if ((*it)->getAtom() == atom) {
            FFEvent* tmp = *it;
            it = events.erase(it);
            delete tmp;
            decrement();
        } else {
            ++it;
        }
    }
}

string TimeTable::print() {
    ostringstream oss;
    oss << "TIMETABLE" << endl;
    for (auto ev : events) {
        oss << ev->toString() << " at " << ev->getTime() 
            << " at " << ev->getAtom() << endl;
    }
    oss << "END TIMETABLE" << endl;
    return oss.str();
}

} // namespace libforefire