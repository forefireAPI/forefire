/**
 * @file test_timetable.cpp
 * @brief The event queue once its last event has been taken out.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 */

#include "doctest/doctest.h"

#include "FFEvent.h"
#include "TimeTable.h"

#include <limits>

using libforefire::FFEvent;
using libforefire::TimeTable;

TEST_SUITE("timetable") {

TEST_CASE("clear() after the last event was dropped on reinsertion") {
    TimeTable table;
    table.insert(new FFEvent(nullptr, 1.0, "none"));

    // A node that stopped gets an infinite time, and insert() deletes its event.
    FFEvent* last = table.getUpcomingEvent();
    last->setNewTime(std::numeric_limits<double>::infinity());
    table.insert(last);

    table.clear();

    FFEvent* next = new FFEvent(nullptr, 2.0, "none");
    table.insert(next);
    CHECK(table.getUpcomingEvent() == next);
    delete next;
}

TEST_CASE("dropping an atom's events spares the event being processed") {
    TimeTable table;
    FFEvent* event = new FFEvent(nullptr, 1.0, "none");
    table.insert(event);

    FFEvent* upcoming = table.getUpcomingEvent();
    REQUIRE(upcoming == event);
    table.dropAtomEvents(nullptr);

    upcoming->setNewTime(2.0);
    table.insert(upcoming);
    CHECK(table.getUpcomingEvent() == event);
    delete event;
}

}
