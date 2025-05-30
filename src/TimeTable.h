/**
 * @file TimeTable.h
 * @brief Defines the TimeTable class for managing the chronological succession of simulation events (FFEvents).
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef TIMETABLE_H_
#define TIMETABLE_H_

#include <iostream>
#include <set>
#include <limits>
#include <sstream>
#include <stdexcept>
#include "ForeFireAtom.h"
#include "FFEvent.h"
#include "include/FFConstants.h"

using namespace std;

namespace libforefire {

// Comparator for FFEvent pointers: sorts by event time, with pointer as tie-breaker.
struct FFEventComparator {
    bool operator()(const FFEvent* lhs, const FFEvent* rhs) const {
        // Compare based on time; if equal, compare pointer addresses
        if (lhs->getTime() != rhs->getTime())
            return lhs->getTime() < rhs->getTime();
        return lhs < rhs;
    }
};

/*! \class TimeTable
 * \brief Class providing control over the succession of 'FFEvents'
 *
 *  The 'TimeTable' class provides an efficient way to deal
 *  with the succession of events occurring in the simulation,
 *  i.e. 'FFEvents'. The timetable points to the 'head' event,
 *  which in turn points to the next one, etc... A 'rbin'
 *  is provided to trash events.
 */
class TimeTable {

    FFEvent* head; /*!< Next event to happen */
    FFEvent* rbin; /*!< Bin for thrashing events */
    multiset<FFEvent*, FFEventComparator> events;

    size_t size(); /*!< Number of events in the timetable */
    size_t incr; /*!< Total number of inserted events */
    void increment();
    size_t decr; /*!< Total number of deleted events */
    void decrement();

    void commonInitialization();
public:
    /*! \brief Default constructor */
    TimeTable();
    /*! \brief Constructor providing one 'FFEvent' */
    TimeTable(FFEvent*);
    /*! \brief Default destructor */
    virtual ~TimeTable();

    /*! \brief Mutator of the 'head' of the timetable */
    void setHead(FFEvent*);
    friend void FFEvent::setNext(FFEvent*);
    friend void FFEvent::setPrev(FFEvent*);
    /*! \brief Accessor to the next event */
    FFEvent* getUpcomingEvent();
    /*! \brief Accessor to the head */
    FFEvent* getHead();
    /*! \brief Inserting a new event before the head */
    void insertBefore(FFEvent*);
    /*! \brief Inserting a new event according to its time of activation */
    void insert(FFEvent*);
    /*! \brief Removing an event */
    void dropEvent(FFEvent*);
    /*! \brief Removing all the events associated to a ForeFireAtom */
    void clear();
    /*! \brief Removing all the events associated to a ForeFireAtom */

    void dropAtomEvents(ForeFireAtom*);

    /*! \brief Getting the current time of the timetable */
    double getTime();

    /*! \brief Printing the timetable */
    string print();
};

} // namespace libforefire

#endif /* TIMETABLE_H_ */
