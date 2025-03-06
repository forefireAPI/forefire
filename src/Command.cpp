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
#include "Command.h"
#include "colormap.h"
#include <sstream>
#include <dirent.h>
#include <regex>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

namespace libforefire
{

    size_t Command::currentLevel = 0;

    bool Command::init = true;
    bool Command::currentFrontCompleted = false;

    double Command::startTime = 0;
    double Command::endTime = 0;

    bool Command::firstCommand = true;
    size_t Command::refTabs = 0;

    FFPoint *Command::lastReadLoc = 0;
    FireNode *Command::previousNode = 0;
    FireNode *Command::leftLinkNode = 0;
    FireNode *Command::rightLinkNode = 0;

    double Command::bmapOutputUpdate = 0;
    int Command::numBmapOutputs = 0;
    double Command::refTime = 0;
    int Command::numAtmoIterations = 0;

    const string Command::stringError = "1234567890";
    const FFPoint Command::pointError = FFPoint(1234567890., 1234567890., 0);
    const FFVector Command::vectorError = FFVector(1234567890., 1234567890.);

    vector<string> Command::outputDirs;

    Command::Session Command::currentSession =
        {
            SimulationParameters::GetInstance(),
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            &cout,
            0,
            0,
    };

    const Command::commandMap Command::translator = Command::makeCmds();

    // Defaults constructor and destructor for the 'Command' abstract class
    Command::Command()
    {
    }

    Command::~Command()
    {
    }

    void Command::increaseLevel()
    {
        currentLevel++;
    }

    void Command::decreaseLevel()
    {
        currentLevel--;
    }

    int Command::createDomain(const string &arg, size_t &numTabs)
    {

        size_t n = argCount(arg);
        if (n >= 3)
        {
            FFPoint SW = getPoint("sw", arg);
            FFPoint NE = getPoint("ne", arg);
            double t = getFloat("t", arg);

            setStartTime(t);
            setReferenceTime(t);

            /* creating the domain */
            if (currentSession.fd != 0)
            {

                if (currentSession.fd->getDomainID() == 1)
                { // ID 1 is for the Fortran MPI rank 1
                    cout << "Creating master FD coupler" << endl;
                    currentSession.params->setParameter("runmode", "masterMNH");

                    currentSession.fdp = new FireDomain(t, SW, NE);
                    currentSession.fdp->setTimeTable(currentSession.tt);
                    currentSession.ff = currentSession.fdp->getDomainFront();

                    ostringstream ffOutputsFNAME;
                    ffOutputsFNAME << currentSession.params->getParameter("caseDirectory") << '/'
                                   << currentSession.params->getParameter("fireOutputDirectory") << '/'
                                   << currentSession.params->getParameter("outputFiles")
                                   << "." << currentSession.fdp->getDomainID();
                    currentSession.params->setParameter("PffOutputsPattern", ffOutputsFNAME.str());
                    currentSession.outStrRepp = new StringRepresentation(currentSession.fdp);
                    currentSession.outStrRepp->setOutPattern(ffOutputsFNAME.str());
                    if (currentSession.params->getInt("outputsUpdate") != 0)
                    {
                        currentSession.tt->insert(new FFEvent(currentSession.outStrRepp));
                    }
                }
            }
            else
            {
                currentSession.fd = new FireDomain(t, SW, NE);

                /* setting up the pointer to the domain */

                /* creating the related timetable and simulator */
                currentSession.tt = new TimeTable(new FFEvent(getDomain()));
                currentSession.sim = new Simulator(currentSession.tt, currentSession.fd->outputs);
                getDomain()->setTimeTable(currentSession.tt);
                /* linking to the domain front */
                currentSession.ff = getDomain()->getDomainFront();
                /* managing the outputs */
                ostringstream ffOutputsPattern;
                ffOutputsPattern << currentSession.params->getParameter("caseDirectory") << '/'
                                 << currentSession.params->getParameter("fireOutputDirectory") << '/'
                                 << currentSession.params->getParameter("outputFiles")
                                 << "." << getDomain()->getDomainID();
                currentSession.params->setParameter("ffOutputsPattern", ffOutputsPattern.str());
                currentSession.outStrRep = new StringRepresentation(currentSession.fd);
                if (currentSession.params->getInt("outputsUpdate") != 0)
                {
                    currentSession.tt->insert(new FFEvent(currentSession.outStrRep));
                }
                /* increasing the level */
                increaseLevel();
            }
            /* local copy of the reference time */
            return normal;
        }
        else
        {
            throw MissingOption(3 - n);
        }
    }

    int Command::startFire(const string &arg, size_t &numTabs)
    {
        double t = getDomain()->getTime();
        size_t n = argCount(arg);

        if (getDomain()->getDomainID() > 0)
        {

            return normal;
        }

        FireDomain *refDomain = getDomain();

        SimulationParameters *simParam = SimulationParameters::GetInstance();

        FFPoint pos = getPoint("lonlat", arg);
        
        if (pos == pointError)
        {
            pos = getPoint("loc", arg);
        }

        if ((refDomain->striclyWithinDomain(pos)))
        {
            if (currentSession.ff != 0)
            {
                completeFront(currentSession.ff);
            }

            double nt = getFloat("t", arg);

            if (nt != FLOATERROR)
            {
                t = nt;
            }
             
            string date = getString("date", arg);

            if (date != stringError)
            {
                int year, yday;
                double secs;

                simParam->ISODateDecomposition(date, secs, year, yday);

                t = simParam->SecsBetween(simParam->getDouble("refTime"), simParam->getInt("refYear"), simParam->getInt("refDay"), secs, year, yday);

                cout << endl
                     << endl
                     << "REF DATE : " << simParam->FormatISODate(simParam->getDouble("refTime"), simParam->getInt("refYear"), simParam->getInt("refDay"))
                     << endl
                     << "DATE OF FIRE : " << date << endl
                     << "SECS BETWEEN BOTH : " << t << endl
                     << endl;
            }

            double perimRes = refDomain->getPerimeterResolution() * 2;
            int fdom = getInt("domain", arg);
            if (fdom == INTERROR)
                fdom = 0;

            double fdepth = currentSession.params->getDouble("initialFrontDepth");
            double kappa = 0.;
            string state = "init";
            FireFront *contfront = currentSession.ff->getContFront();
            currentSession.ff = refDomain->addFireFront(t, contfront);

            FFVector vel1 = FFVector(0, 1);
            FFVector vel2 = FFVector(1, -1);
            FFVector vel3 = FFVector(-1, -1);
            FFVector diffP1 = perimRes * vel1;
            FFVector diffP2 = perimRes * vel2;
            FFVector diffP3 = perimRes * vel3;

            FFPoint pos1 = pos + diffP1.toPoint();
            FFPoint pos2 = pos + diffP2.toPoint();
            FFPoint pos3 = pos + diffP3.toPoint();

            vel1 *= 0.1;
            vel2 *= 0.1;
            vel3 *= 0.1;

            FireNode *lastnode = refDomain->addFireNode(pos1, vel1, t, fdepth, kappa, currentSession.ff, 0);
            lastnode = refDomain->addFireNode(pos2, vel2, t, fdepth, kappa, currentSession.ff, lastnode);
            refDomain->addFireNode(pos3, vel3, t, fdepth, kappa, currentSession.ff, lastnode);

            completeFront(currentSession.ff);
        }

        // simParam->setParameter("ISOdate", simParam->FormatISODate(simParam->getInt("refTime") + refDomain->getSimulationTime(), refDomain->getReferenceYear(), refDomain->getReferenceDay()));

        return normal;
    }

    std::vector<FFPoint> Command::getPoly(const std::string &opt, const std::string &arg)
    {
        std::vector<FFPoint> points;

        if (opt == "geojson")
        {
            // Example input: [[8.81116, 41.96981, 0], [8.81137, 41.96976, 0], ...]
            // Use a regex to extract all numbers.
            std::regex number_regex("[-+]?[0-9]*\\.?[0-9]+");
            auto numbers_begin = std::sregex_iterator(arg.begin(), arg.end(), number_regex);
            auto numbers_end = std::sregex_iterator();
            std::vector<double> coords;
            for (auto it = numbers_begin; it != numbers_end; ++it)
            {
                coords.push_back(std::stod(it->str()));
            }
            if (coords.size() % 3 != 0)
                return points; // Malformed input.

            double refLon = getDomain()->getRefLongitude();
            double refLat = getDomain()->getRefLatitude();
            double mPerDegLon = getDomain()->getMetersPerDegreesLon();
            double mPerDegLat = getDomain()->getMetersPerDegreeLat();

            for (size_t i = 0; i < coords.size(); i += 3)
            {
                double lon = coords[i];
                double lat = coords[i + 1];
                double alt = coords[i + 2];
                double x = (lon - refLon) * mPerDegLon;
                double y = (lat - refLat) * mPerDegLat;
                points.push_back(FFPoint(x, y, alt));
            }
        }
        else if (opt == "kml")
        {
            // Example input:
            // "8.810264,41.968841,0.14 8.810117,41.968757,0.03 8.810017,41.968692,0.01"
            std::istringstream iss(arg);
            double refLon = getDomain()->getRefLongitude();
            double refLat = getDomain()->getRefLatitude();
            double mPerDegLon = getDomain()->getMetersPerDegreesLon();
            double mPerDegLat = getDomain()->getMetersPerDegreeLat();
            std::string token;
            while (iss >> token)
            {
                // Replace commas with spaces.
                std::replace(token.begin(), token.end(), ',', ' ');
                std::istringstream ptStream(token);
                double lon, lat, alt;
                if (!(ptStream >> lon >> lat >> alt))
                    continue;
                double x = (lon - refLon) * mPerDegLon;
                double y = (lat - refLat) * mPerDegLat;
                points.push_back(FFPoint(x, y, alt));
            }
        }
        else if (opt == "polyencoded")
        {
            // Polyline encoded string. Standard algorithm decodes lat and lon.
            // Altitude is not provided (set to 0).
            double refLon = getDomain()->getRefLongitude();
            double refLat = getDomain()->getRefLatitude();
            double mPerDegLon = getDomain()->getMetersPerDegreesLon();
            double mPerDegLat = getDomain()->getMetersPerDegreeLat();

            int index = 0, len = static_cast<int>(arg.size());
            int lat = 0, lng = 0;
            while (index < len)
            {
                int b, shift = 0, resultInt = 0;
                do
                {
                    b = arg[index++] - 63;
                    resultInt |= (b & 0x1f) << shift;
                    shift += 5;
                } while (b >= 0x20 && index < len);
                int dlat = (resultInt & 1) ? ~(resultInt >> 1) : (resultInt >> 1);
                lat += dlat;

                shift = 0;
                resultInt = 0;
                do
                {
                    b = arg[index++] - 63;
                    resultInt |= (b & 0x1f) << shift;
                    shift += 5;
                } while (b >= 0x20 && index < len);
                int dlng = (resultInt & 1) ? ~(resultInt >> 1) : (resultInt >> 1);
                lng += dlng;

                double latd = lat * 1e-5;
                double lngd = lng * 1e-5;
                double x = (lngd - refLon) * mPerDegLon;
                double y = (latd - refLat) * mPerDegLat;
                points.push_back(FFPoint(x, y, 0));
            }
        }
        else if (opt == "points")
        {
            // Example input: (x,y,z),(x,y,z),...
            // Coordinates are assumed to be already in Cartesian space.
            size_t pos = 0;
            while (true)
            {
                pos = arg.find('(', pos);
                if (pos == std::string::npos)
                    break;
                size_t end = arg.find(')', pos);
                if (end == std::string::npos)
                    break;
                std::string pointStr = arg.substr(pos + 1, end - pos - 1);
                std::replace(pointStr.begin(), pointStr.end(), ',', ' ');
                std::istringstream issPoint(pointStr);
                double x, y, z;
                if (!(issPoint >> x >> y >> z))
                {
                    pos = end + 1;
                    continue;
                }
                points.push_back(FFPoint(x, y, z));
                pos = end + 1;
            }
        }

        return points;
    }

    int Command::createFireFront(const string &arg, size_t &numTabs)
    {

        size_t n = argCount(arg);
        if (n >= 1)
        {
            double t = getFloat("t", arg);
            if (numTabs == currentLevel)
            {

                /* first completing the previous front if needed */
                if (currentSession.ff != 0)
                {
                    completeFront(currentSession.ff);
                    currentFrontCompleted = false;
                }
                /* creating the new front */
                FireFront *contfront = currentSession.ff->getContFront();
                currentSession.ff = getDomain()->addFireFront(t, contfront);
            }
            else if (numTabs > currentLevel)
            {

                /* first completing the previous front */
                completeFront(currentSession.ff);
                currentFrontCompleted = false;
                /* creation of an inner front to the current one */
                FireFront *contfront = currentSession.ff;
                currentSession.ff = getDomain()->addFireFront(t, contfront);
                currentLevel = numTabs;
            }
            else if (numTabs < currentLevel)
            {

                /* first completing the previous front */
                completeFront(currentSession.ff);
                currentFrontCompleted = false;
                /* creation of a new front at a higher level then the current one */
                FireFront *contfront = currentSession.ff;
                for (size_t k = 0; k < currentLevel - numTabs; k++)
                {
                    contfront = contfront->getContFront();
                }
                currentSession.ff = getDomain()->addFireFront(t, contfront);
                currentLevel = numTabs;
            }
        }
        else
        {
            throw MissingTime();
        }

        return normal;
    }

    int Command::addFireNode(const string &arg, size_t &numTabs)
    {

        if (lastReadLoc == 0)
            lastReadLoc = new FFPoint(-numeric_limits<double>::infinity(), -numeric_limits<double>::infinity(), 0);

        size_t n = argCount(arg);
        double perimRes = getDomain()->getPerimeterResolution();
        if (n >= 3)
        {
            FFPoint pos = getPoint("loc", arg);
            FFVector vel = getVector("vel", arg);
            double t = getFloat("t", arg);
            int fdom = getInt("domain", arg);
            if (fdom == INTERROR)
                fdom = 0;
            int id = getInt("id", arg);
            if (id == INTERROR)
                id = 0;
            double fdepth = getFloat("fdepth", arg);
            if (fdepth == FLOATERROR)
                fdepth = currentSession.params->getDouble("initialFrontDepth");
            double kappa = getFloat("kappa", arg);
            if (kappa == FLOATERROR)
                kappa = 0.;
            string state = getString("state", arg);
            if (state == stringError or state == "moving")
                state = "init";
            if (numTabs != currentLevel + 1)
            {
                cout << getDomain()->getDomainID() << ": WARNING : asked for a FireNode "
                     << " with wrong indentation, treating it the current fire front" << endl;
            }

            if (state == "link")
            {

                /* Creating a link node */
                previousNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, previousNode, fdom, id, FireNode::link);
                if (getDomain()->commandOutputs)
                    cout << getDomain()->getDomainID() << ": INIT -> added " << previousNode->toString() << endl;
            }
            else if (state == "final")
            {

                /* Creating a link node */
                previousNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, previousNode, fdom, id, FireNode::final);
                if (getDomain()->commandOutputs)
                    cout << getDomain()->getDomainID() << ": INIT -> added " << previousNode->toString() << endl;
            }
            else if (getDomain()->striclyWithinDomain(pos) and getDomain()->striclyWithinDomain(*lastReadLoc))
            {

                /* Both nodes are within the domain and represent real firenodes */
                /* checking the distance between the two nodes */
                double distanceBetweenNodes = lastReadLoc->distance2D(pos);
                int interNodes = (int)floor(distanceBetweenNodes / (2. * perimRes));
                FFPoint posinc = (1. / (interNodes + 1)) * (pos - previousNode->getLoc());
                FFVector velinc = (1. / (interNodes + 1)) * (vel - previousNode->getVel());
                double timeinc = (1. / (interNodes + 1)) * (t - previousNode->getTime());
                FFPoint ipos;
                FFVector ivel;
                double itime;
                for (int k = 0; k < interNodes; k++)
                {
                    ipos = previousNode->getLoc() + posinc;
                    ivel = previousNode->getVel() + velinc;
                    itime = previousNode->getTime() + timeinc;
                    previousNode = getDomain()->addFireNode(ipos, ivel, itime, fdepth, kappa, currentSession.ff, previousNode);
                    if (getDomain()->commandOutputs)
                        cout << getDomain()->getDomainID() << ": INIT -> added inter-node "
                             << previousNode->toString() << endl;
                }
                // creating the firenode
                previousNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, previousNode, fdom, id);
                if (getDomain()->commandOutputs)
                    cout << getDomain()->getDomainID()
                         << ": INIT -> added " << previousNode->toString() << endl;
            }
            else if (!getDomain()->striclyWithinDomain(pos) and getDomain()->striclyWithinDomain(*lastReadLoc))
            {

                FFPoint linkPoint = getDomain()->findIntersectionWithFrontiers(
                    pos, *lastReadLoc);
                rightLinkNode = getDomain()->addLinkNode(linkPoint);
                /* checking the distance between the two nodes */
                double distanceBetweenNodes = linkPoint.distance2D(previousNode->getLoc());
                if (distanceBetweenNodes > 2. * perimRes)
                {
                    int interNodes = (int)floor(distanceBetweenNodes / (2. * perimRes));
                    FFPoint posinc = (1. / (interNodes + 1)) * (linkPoint - previousNode->getLoc());
                    FFPoint ipos;
                    for (int k = 0; k < interNodes; k++)
                    {
                        ipos = previousNode->getLoc() + posinc;
                        previousNode = getDomain()->addFireNode(ipos, vel, t, fdepth, kappa, currentSession.ff, previousNode);
                        if (getDomain()->commandOutputs)
                            cout << getDomain()->getDomainID() << ": INIT -> added inter-node "
                                 << previousNode->toString() << endl;
                    }
                }
                previousNode->insertAfter(rightLinkNode);
                if (leftLinkNode != 0)
                    getDomain()->relateLinkNodes(rightLinkNode, leftLinkNode);
            }
            else if (getDomain()->striclyWithinDomain(pos) and !getDomain()->striclyWithinDomain(*lastReadLoc))
            {

                if (lastReadLoc->getX() == -numeric_limits<double>::infinity())
                {

                    /* First node to be created */
                    previousNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, previousNode, fdom, id);
                    if (getDomain()->commandOutputs)
                        cout << getDomain()->getDomainID() << ": INIT -> added " << previousNode->toString() << endl;
                }
                else
                {

                    /* Creating a link node */
                    FFPoint linkPoint =
                        getDomain()->findIntersectionWithFrontiers(
                            pos, *lastReadLoc);
                    leftLinkNode = getDomain()->addLinkNode(linkPoint);
                    if (rightLinkNode != 0)
                    {
                        currentSession.ff->addFireNode(leftLinkNode, rightLinkNode);
                    }
                    else
                    {
                        leftLinkNode->setFront(currentSession.ff);
                        currentSession.ff->addFireNode(leftLinkNode);
                    }
                    previousNode = leftLinkNode;
                    /* checking the distance between the two nodes */
                    double distanceBetweenNodes = leftLinkNode->getLoc().distance2D(pos);
                    if (distanceBetweenNodes > 2. * perimRes)
                    {
                        int interNodes = (int)floor(distanceBetweenNodes / (2. * perimRes));
                        FFPoint posinc = (1. / (interNodes + 1)) * (pos - leftLinkNode->getLoc());
                        FFPoint ipos;
                        for (int k = 0; k < interNodes; k++)
                        {
                            ipos = previousNode->getLoc() + posinc;
                            previousNode = getDomain()->addFireNode(ipos, vel, t, fdepth, kappa, currentSession.ff, previousNode);
                            if (getDomain()->commandOutputs)
                                cout << getDomain()->getDomainID() << ": INIT -> added inter-node "
                                     << previousNode->toString() << endl;
                        }
                    }
                    // creating the firenode
                    previousNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, previousNode, fdom, id);
                    if (getDomain()->commandOutputs)
                        cout << getDomain()->getDomainID()
                             << ": INIT -> added " << previousNode->toString() << endl;
                }
            }

            lastReadLoc->setX(pos.getX());
            lastReadLoc->setY(pos.getY());

            return normal;
        }
        else
        {
            throw MissingOption(3 - n);
        }
    }

    void Command::completeFront(FireFront *ff)
    {
        if (ff->getHead() == 0)
            return;
        FireNode *firstNode = ff->getHead();
        FireNode *lastNode = firstNode->getPrev();
        if (firstNode->getState() == FireNode::init and lastNode->getState() == FireNode::init)
        {
            double perimRes = getDomain()->getPerimeterResolution();
            double distanceBetweenNodes = lastNode->distance2D(firstNode);
            double fdepth = 0.5 * (firstNode->getFrontDepth() + lastNode->getFrontDepth());
            double kappa = 0.5 * (firstNode->getCurvature() + lastNode->getCurvature());
            int numNewNodes = (int)floor(distanceBetweenNodes / (2. * perimRes));
            if (numNewNodes > 0)
            {
                FFPoint posinc = (1. / (numNewNodes + 1)) * (firstNode->getLoc() - lastNode->getLoc());
                FFVector velinc = (1. / (numNewNodes + 1)) * (firstNode->getVel() - lastNode->getVel());
                double timeinc = (1. / (numNewNodes + 1)) * (firstNode->getTime() - lastNode->getTime());
                FireNode *prevNode = lastNode;
                FFPoint pos;
                FFVector vel;
                double t;
                for (int k = 0; k < numNewNodes; k++)
                {
                    pos = lastNode->getLoc() + (k + 1) * posinc;
                    vel = lastNode->getVel() + (k + 1) * velinc;
                    t = lastNode->getTime() + (k + 1) * timeinc;
                    prevNode = getDomain()->addFireNode(pos, vel, t, fdepth, kappa, currentSession.ff, prevNode);
                    prevNode->computeNormal();
                }
            }
        }
        if (getDomain()->commandOutputs)
        {
            cout << getDomain()->getDomainID() << ": "
                 << "****************************************" << endl;
            cout << getDomain()->getDomainID() << ": "
                 << " BEFORE THE BEGINNING OF THE SIMULATION: " << endl
                 << ff->print(1);
            cout << getDomain()->getDomainID() << ": "
                 << "****************************************" << endl;
        }

        if (!currentSession.params->isValued("BMapFiles"))
        {
            /* testing the domain for burning matrix */
            double iniFrontDepth = currentSession.params->getDouble("initialFrontDepth");
            double iniBurningTime = currentSession.params->getDouble("initialBurningDuration");

            if (!currentSession.params->isValued("noInitialScan"))
            {
                getDomain()->frontInitialBurningScan(ff->getTime(), ff, iniFrontDepth, iniBurningTime);
            }
        }

        currentFrontCompleted = true;
        delete lastReadLoc;
        lastReadLoc = 0;
    }

    int Command::stepSimulation(const string &arg, size_t &numTabs)
    {
        if (getDomain() == 0)
            return normal;

        double dt = getFloat("dt", arg);
        endTime = startTime + dt;
        ostringstream etime;
        etime.precision(numeric_limits<double>::digits10);
        etime << "t=" << endTime;
        goTo(etime.str().c_str(), numTabs);

        return normal;
    }

    int Command::goTo(const string &arg, size_t &numTabs)
    {

        /* Advancing simulation to the prescribed time */
        endTime = getFloat("t", arg);

        if (init)
        {
            /* finishing the initialization process */
            bmapOutputUpdate = currentSession.params->getDouble("bmapOutputUpdate");
            if (!currentFrontCompleted)
                completeFront(currentSession.ff);
            numAtmoIterations = currentSession.params->getInt("numAtmoIterations") - 1;
            init = false;
        }

        if (endTime > startTime)
        {
            getDomain()->setTime(startTime);
            if (getDomain()->commandOutputs)
            {
                cout.precision(numeric_limits<double>::digits10);
                cout << getDomain()->getDomainID() << ": "
                     << "***************************************************" << endl;
                cout << getDomain()->getDomainID() << ": "
                     << "   ADVANCING FOREFIRE SIMULATION FROM T="
                     << startTime << " to " << endTime << endl;
                cout << getDomain()->getDomainID() << ": "
                     << "***************************************************" << endl;
            }

            try
            {

                /* ******************************************** */
                /* Advancing the simulation to the desired time */
                /* ******************************************** */
                // cout<<getDomain()->getDomainID()<<"  iteration : "<<FireDomain::atmoIterNumber<<" and "<<getDomain()->getNumIterationAtmoModel()<<endl;
                FireDomain::atmoIterNumber = FireDomain::atmoIterNumber + 1;

                currentSession.params->setInt("atmoIterNumber", FireDomain::atmoIterNumber);

//
#ifdef MPI_COUPLING
                currentSession.sim->goTo(endTime);
#else
                currentSession.fd->loadCellsInBinary();
                currentSession.sim->goTo(endTime);
                getDomain()->dumpCellsInBinary();
                getDomain()->loadWindDataInBinary(endTime);
#endif

                startTime = endTime;
                getDomain()->increaseNumIterationAtmoModel();

                /* outputs */
                /* ******* */
                if (getDomain()->commandOutputs)
                {
                    cout << getDomain()->getDomainID() << ": "
                         << "End of the step in domain "
                         << getDomain()->getDomainID() << endl
                         << currentSession.outStrRep->dumpStringRepresentation();
                }
                if (currentSession.params->getInt("debugFronts") != 0)
                    printSimulation(currentSession.params->getParameter("ffOutputsPattern"), numTabs);
                /* burning map outputs */
                if (bmapOutputUpdate > 0)
                {
                    if ((int)((endTime - refTime) / bmapOutputUpdate) > numBmapOutputs)
                    {
                        if (getDomain()->getDomainID() == 0)
                        {
                            getDomain()->saveArrivalTimeNC();
                        }
                        numBmapOutputs++;
                    }
                }

                /* backing up the state for future steps */
                /* ************************************* */
                // getDomain()->validateTopology("advance");
                // getDomain()->backupState();

                /* saving simulation if needed */
                /* *************************** */
            }
            catch (TopologicalException &e)
            {
                cout << getDomain()->getDomainID() << ": " << e.what() << endl;
                getDomain()->restoreValidState();
                getDomain()->setSafeTopologyMode(true);
                cout << getDomain()->getDomainID() << ": "
                     << "**** MAKING THIS STEP IN SAFE TOPOLOGY MODE ****" << endl;
                try
                {

                    /* ******************************************** */
                    /* Advancing the simulation to the desired time */
                    /* ******************************************** */
                    currentSession.sim->goTo(endTime);
                    // Getting out of safe topology mode
                    getDomain()->setSafeTopologyMode(false);
                    startTime = endTime;
                    getDomain()->increaseNumIterationAtmoModel();
                    if (getDomain()->commandOutputs)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "End of the step in domain "
                             << getDomain()->getDomainID() << endl
                             << currentSession.outStrRep->dumpStringRepresentation();
                    }

                    /* outputs */
                    /* ******* */
                    if (getDomain()->commandOutputs)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "End of the step in domain "
                             << getDomain()->getDomainID() << endl
                             << currentSession.outStrRep->dumpStringRepresentation();
                    }
                    if (currentSession.params->getInt("debugFronts") != 0)
                        printSimulation(currentSession.params->getParameter("ffOutputsPattern"), numTabs);
                    /* burning map outputs */
                    if (bmapOutputUpdate > 0)
                    {
                        if ((int)((endTime - refTime) / bmapOutputUpdate) > numBmapOutputs)
                        {
                            getDomain()->saveArrivalTimeNC();
                            numBmapOutputs++;
                        }
                    }

                    /* backing up the state for future steps */
                    /* ************************************* */
                    getDomain()->validateTopology("advance");
                    getDomain()->backupState();

                    /* saving simulation if needed */
                    /* *************************** */
                    if (getDomain()->getNumIterationAtmoModel() == numAtmoIterations)
                        getDomain()->saveArrivalTimeNC();
                }
                catch (...)
                {
                    if (getDomain()->commandOutputs)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "**** ERROR IN SAFE TOPOLOGY MODE, QUITING ****" << endl;
                    }
                    // TODO supersafe mode ?
                    quit(arg, numTabs);
                }
            }
        }
        SimulationParameters *simParam = SimulationParameters::GetInstance();
        simParam->setParameter("ISOdate", simParam->FormatISODate(simParam->getInt("refTime") + getDomain()->getSimulationTime(), getDomain()->getReferenceYear(), getDomain()->getReferenceDay()));

        return normal;
    }




    int Command::printSimulation(const string &arg, size_t &numTabs)
    {
        if (getDomain() == 0)
            return normal;

        SimulationParameters *simParam = SimulationParameters::GetInstance();
        string finalStr = "";

        if (arg.size() > 0)
        {
            vector<string> parts;
            tokenize(arg, parts, "*");

            unsigned int partToEval = (arg.at(0) == '*') ? 0 : 1;

            for (auto i = 0u; i < parts.size(); i++)
            {
                finalStr += (i % 2 == partToEval) ? simParam->getParameter(parts[i]) : parts[i];
            }
        }

        replace(finalStr.begin(), finalStr.end(), ':', '-');

        ofstream outputfile(finalStr.c_str());
        if (outputfile)
        {
            cout<<" putting into file "<<finalStr<<endl;
            simParam->setInt("count", simParam->getInt("count") + 1);
            if (currentSession.fdp != 0)
            {
                outputfile << currentSession.outStrRepp->dumpStringRepresentation();
            }
            else
            {
                outputfile << currentSession.outStrRep->dumpStringRepresentation();
            }
        }
        else
        {
            if (currentSession.fdp != 0)
            {
                *currentSession.outStream << currentSession.outStrRepp->dumpStringRepresentation();
            }
            else
            {
                *currentSession.outStream << currentSession.outStrRep->dumpStringRepresentation();
            }
        }
        return normal;
    }
    int Command::systemExec(const string &arg, size_t &numTabs)
    {
        SimulationParameters *simParam = SimulationParameters::GetInstance();
        string finalStr;

        if (!arg.empty())
        {
            vector<string> parts;
            tokenize(arg, parts, "*");

            unsigned int partToEval = (arg.at(0) == '*') ? 0 : 1;

            for (size_t i = 0; i < parts.size(); i++)
            {
                finalStr += (i % 2 == partToEval) ? simParam->getParameter(parts[i]) : parts[i];
            }
        }

        replace(finalStr.begin(), finalStr.end(), ':', '-');

        // Use popen to execute the command and capture its output.
        FILE *pipe = popen(finalStr.c_str(), "r");
        if (!pipe)
        {
            *currentSession.outStream << "popen() failed!" << endl;
            return -1;
        }

        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            string line(buffer);
            // Optionally process line if needed
            *currentSession.outStream << line;
        }
        int status = pclose(pipe);
        return status;
    }

    std::vector<double> getMinMax(const std::vector<std::vector<double>> &matrix) {
        // Initialize min and max with extreme values.
        double minVal = std::numeric_limits<double>::max();
        double maxVal = std::numeric_limits<double>::lowest();
    
        // Iterate over each row and each value in the row.
        for (const auto &row : matrix) {
            for (double value : row) {
                if (value < minVal) {
                    minVal = value;
                }
                if (value > maxVal) {
                    maxVal = value;
                }
            }
        }
        
        return std::vector<double>{minVal, maxVal};
    }
    std::vector<double> getMinMaxFromUV(const std::vector<std::vector<double>> &matrixU,
                                        const std::vector<std::vector<double>> &matrixV) {
        // Ensure matrices have the same dimensions
        size_t numRows = matrixU.size();
        if (numRows != matrixV.size()) {
            // Handle the error as needed, here we simply return an empty vector.
            return std::vector<double>{};
        }

        double minVal = std::numeric_limits<double>::max();
        double maxVal = std::numeric_limits<double>::lowest();

        for (size_t i = 0; i < numRows; ++i) {
            size_t numCols = matrixU[i].size();
            if (matrixV[i].size() != numCols) {
                // Handle dimension mismatch, here we simply return an empty vector.
                return std::vector<double>{};
            }
            for (size_t j = 0; j < numCols; ++j) {
                double u = matrixU[i][j];
                double v = matrixV[i][j];
                double magnitude = std::sqrt(u * u + v * v);

                if (magnitude < minVal) {
                    minVal = magnitude;
                }
                if (magnitude > maxVal) {
                    maxVal = magnitude;
                }
            }
        }
        
        return std::vector<double>{minVal, maxVal};
    }
    int Command::plotSimulation(const std::string &arg, size_t &numTabs)
    {
        if (getDomain() == nullptr)
            return normal;
    
        SimulationParameters *simParam = SimulationParameters::GetInstance();
    
        if (!arg.empty())
        {
            std::map<std::string, std::string> argMap;
            std::string temp = arg;
            size_t pos = 0;
            std::string token;
            const char delim = ';';
            const char assign = '=';
    
            // Parse the argument string into a key-value map.
            while ((pos = temp.find(delim)) != std::string::npos)
            {
                token = temp.substr(0, pos);
                size_t eq_pos = token.find(assign);
                if (eq_pos != std::string::npos)
                {
                    std::string key = token.substr(0, eq_pos);
                    std::string value = token.substr(eq_pos + 1);
                    argMap[key] = value;
                }
                temp.erase(0, pos + 1);
            }
            // Handle the last part after the final semicolon.
            size_t eq_pos = temp.find(assign);
            if (eq_pos != std::string::npos)
            {
                std::string key = temp.substr(0, eq_pos);
                std::string value = temp.substr(eq_pos + 1);
                argMap[key] = value;
            }
    
            // Extract arguments of interest.
            std::string filename = argMap["filename"];
            std::string parameter = argMap["parameter"];
            std::string colormap = argMap["cmap"];
    
            // Parse optional matrix size.
            size_t eni = 0, enj = 0;
            if (argMap.find("size") != argMap.end())
            {
                std::string matrixShape = argMap.at("size");
                size_t comma = matrixShape.find(',');
                if (comma != std::string::npos)
                {
                    eni = std::stoul(matrixShape.substr(0, comma));
                    enj = std::stoul(matrixShape.substr(comma + 1));
                }
            }
    
            // Determine bounding box from arguments (BBoxWSEN or BBoxLBRT) or use domain corners.
            double SWX, SWY, NEX, NEY;
            if (argMap.find("BBoxWSEN") != argMap.end())
            {
                // BBoxWSEN: WestLongitude, SouthLatitude, EastLongitude, NorthLatitude.
                std::string areaStr = argMap.at("BBoxWSEN");
                if (!areaStr.empty() && areaStr.front() == '(' && areaStr.back() == ')')
                {
                    areaStr = areaStr.substr(1, areaStr.size() - 2); // remove parentheses
                }
                std::istringstream iss(areaStr);
                double v[4] = {0.0, 0.0, 0.0, 0.0};
                int idx = 0;
                while (std::getline(iss, token, ',') && idx < 4)
                {
                    v[idx++] = std::stod(token);
                }
                // Convert geographic -> cartesian
                SWX = getDomain()->getXFromLon(v[0]); // west
                SWY = getDomain()->getYFromLat(v[1]); // south
                NEX = getDomain()->getXFromLon(v[2]); // east
                NEY = getDomain()->getYFromLat(v[3]); // north
            }
            else if (argMap.find("BBoxLBRT") != argMap.end())
            {
                // BBoxLBRT: LeftX, BottomY, RightX, TopY.
                std::string areaStr = argMap.at("BBoxLBRT");
                double v[4] = {0.0, 0.0, 0.0, 0.0};
                if (areaStr == "active")
                {
                    // Domain's active bounding box
                    std::vector<double> activeBBox = getDomain()->getActiveBBoxLBRT();
                    if (activeBBox.size() == 4)
                    {
                        v[0] = activeBBox[0];
                        v[1] = activeBBox[1];
                        v[2] = activeBBox[2];
                        v[3] = activeBBox[3];
                    }
                }
                else if (!areaStr.empty() && areaStr.front() == '(' && areaStr.back() == ')')
                {
                    areaStr = areaStr.substr(1, areaStr.size() - 2);
                    std::istringstream iss(areaStr);
                    int idx = 0;
                    while (std::getline(iss, token, ',') && idx < 4)
                    {
                        v[idx++] = std::stod(token);
                    }
                }
                SWX = v[0];
                SWY = v[1];
                NEX = v[2];
                NEY = v[3];
            }
            else
            {
                // Default corners
                SWX = getDomain()->SWCornerX();
                SWY = getDomain()->SWCornerY();
                NEX = getDomain()->NECornerX();
                NEY = getDomain()->NECornerY();
            }
    
            // Create corner points
            FFPoint SWB(SWX, SWY, 0);
            FFPoint NEB(NEX, NEY, 0);
    
            // Default range
            double minVal = std::numeric_limits<double>::infinity();
            double maxVal = -std::numeric_limits<double>::infinity();
    
            // If range is specified, parse it
            if (argMap.find("range") != argMap.end())
            {
                std::string range = argMap["range"];
                size_t mid = range.find(',');
                if (mid != std::string::npos)
                {
                    // e.g. "(0,100)" => min=0, max=100
                    minVal = std::stod(range.substr(1, mid - 1));
                    maxVal = std::stod(range.substr(mid + 1, range.length() - mid - 2));
                }
            }
    
            // Check if filename is provided
            if (!filename.empty())
            {
                // --------------------------------------------------------------
                // NEW BLOCK: handle "wind" with JSON output
                // --------------------------------------------------------------
                std::string lowerFilename = filename;
                std::transform(lowerFilename.begin(), lowerFilename.end(),
                               lowerFilename.begin(), ::tolower);
    
                if (parameter == "wind" &&
                    lowerFilename.size() >= 5 &&
                    lowerFilename.substr(lowerFilename.size() - 5) == ".json")
                {
                        // We want two data matrices: "windU" and "windV".
                        // Suppose these are each std::vector<std::vector<double>>.
                        auto matrixU = getDomain()->getDataMatrix("windU", SWB, NEB, eni, enj);
             
                        auto matrixV = getDomain()->getDataMatrix("windV", SWB, NEB, eni, enj);
                        auto minMax = getMinMaxFromUV(matrixU, matrixV);

                        if (matrixU.empty() || matrixV.empty())
                        {
                            std::cerr << "Error: No data available for 'windU' or 'windV'." << std::endl;
                        }
                        else
                        {
                            // Convert bounding box corners back to lat/lon, etc.
                            double westLon = getDomain()->getLonFromX(SWX);
                            double eastLon = getDomain()->getLonFromX(NEX);
                            double southLat = getDomain()->getLatFromY(SWY);
                            double northLat = getDomain()->getLatFromY(NEY);

                            // nrows = matrixU.size(), ncols = matrixU[0].size() (assuming consistent sizing)
                            size_t ncols = matrixU.size();
                            size_t nrows = (ncols > 0) ? matrixU[0].size() : 0;

                            double dx = (ncols > 1) ? (eastLon - westLon) / (ncols - 1) : 0.0;
                            double dy = (nrows > 1) ? (northLat - southLat) / (nrows - 1) : 0.0;

                            std::string referenceTime = simParam->FormatISODate(
                                simParam->getDouble("refTime"),
                                simParam->getInt("refYear"),
                                simParam->getInt("refDay"));

                            // Open JSON file
                            std::ofstream outFile(filename);
                            if (!outFile.is_open())
                            {
                                std::cerr << "Error: Cannot open output file " << filename << std::endl;
                            }
                            else
                            {
                                // Write JSON array with two objects
                                outFile << "[";

                                // ------------------
                                // 1) Eastward wind (windU)
                                // ------------------
                                outFile << R"({"header": {)"
                                        << R"("parameterUnit": "m.s-1",)"
                                        << R"("parameterNumber": 2,)"
                                        << R"("dx": )" << dx << ","
                                        << R"("dy": )" << dy << ","
                                        << R"("parameterNumberName": "eastward_wind",)"
                                        << R"("la1": )" << northLat << ","
                                        << R"("la2": )" << southLat << ","
                                        << R"("parameterCategory": 2,)"
                                        << R"("lo2": )" << eastLon << ","
                                        << R"("nx": )" << ncols << ","
                                        << R"("ny": )" << nrows << ","
                                        << R"("refTime": ")" << referenceTime << R"(",)"
                                        << R"("lo1": )" << westLon
                                        << R"(},"data":[)";

                                {
                                    // Flatten matrixU to a 1D comma-separated list
                                    bool firstVal = true;
                                    for (size_t r = 0; r < nrows; r++)
                                    {
                                        for (size_t c = 0; c < ncols; c++)
                                        {
                                            if (!firstVal)
                                                outFile << ",";
                                            double val = matrixU[c][nrows-r-1];
                                            if (std::isnan(val))
                                                outFile << 0.0;
                                            else
                                                outFile << val;
                                        
                                            firstVal = false;
                                        }
                                    }
                                }

                                outFile << "]},"; // close first object

                                // ------------------
                                // 2) Northward wind (windV)
                                // ------------------
                                outFile << R"({"header": {)"
                                        << R"("parameterUnit": "m.s-1",)"
                                        << R"("parameterNumber": 3,)"
                                        << R"("dx": )" << dx << ","
                                        << R"("dy": )" << dy << ","
                                        << R"("parameterNumberName": "northward_wind",)"
                                        << R"("la1": )" << northLat << ","
                                        << R"("la2": )" << southLat << ","
                                        << R"("parameterCategory": 2,)"
                                        << R"("lo2": )" << eastLon << ","
                                        << R"("nx": )" << ncols << ","
                                        << R"("ny": )" << nrows << ","
                                        << R"("refTime": ")" << referenceTime << R"(",)"
                                        << R"("lo1": )" << westLon
                                        << R"(},"data":[)";

                                {
                                    // Flatten matrixV to a 1D comma-separated list
                                    bool firstVal = true;
                                    for (size_t r = 0; r < nrows; r++)
                                    {
                                        for (size_t c = 0; c < ncols; c++)
                                        {
                                            if (!firstVal)
                                                outFile << ",";
                                            outFile << matrixV[r][c];
                                            firstVal = false;
                                        }
                                    }
                                }

                                outFile << "]}]"; // close second object + JSON array
                                outFile.close();
                            }
                    }
    
                    // Still handle optional projectionOut if present
                    if (argMap.find("projectionOut") != argMap.end())
                    {
                        double westLon = getDomain()->getLonFromX(SWX);
                        double eastLon = getDomain()->getLonFromX(NEX);
                        double southLat = getDomain()->getLatFromY(SWY);
                        double northLat = getDomain()->getLatFromY(NEY);
    
                        std::string projectionPath = argMap["projectionOut"];
                        if (projectionPath == "json")
                        {
                            double minVal = minMax[0] ;
                            double maxVal = minMax[1];
                            *currentSession.outStream << "{\"SWlon\":" << westLon
                                                      << ",\"SWlat\":" << southLat
                                                      << ",\"NElon\":" << eastLon
                                                      << ",\"NElat\":" << northLat 
                                                      << ",\"minVal\":" << minVal
                                                      << ",\"maxVal\":" << maxVal
                                                      << "}" << std::endl;
                        }

                    }
                }
                // --------------------------------------------------------------
                // ELSE: original logic for other parameters / file extensions
                // --------------------------------------------------------------
                else
                {
                    // Standard approach: fetch single matrix for the requested "parameter"
                    auto matrix = getDomain()->getDataMatrix(parameter, SWB, NEB, eni, enj);
                    auto minMax = getMinMax(matrix);
                    if (!matrix.empty())
                    {
                        std::string lowerFilename = filename;
                        std::transform(lowerFilename.begin(), lowerFilename.end(),
                                       lowerFilename.begin(), ::tolower);
    
                        // e.g. PNG/JPG
                        if (lowerFilename.size() >= 4 &&
                           (lowerFilename.substr(lowerFilename.size() - 4) == ".png" ||
                            lowerFilename.substr(lowerFilename.size() - 4) == ".jpg"))
                        {
                            writeImage(filename.c_str(), matrix, minVal, maxVal, colormap);
                            if (argMap.find("histbins") != argMap.end())
                            {
                                int numbins = std::stoi(argMap["histbins"]);
                                writeHistogram(filename.c_str(), matrix, numbins, minVal, maxVal, colormap);
                            }
                        }
                        // e.g. NetCDF
                        else if (lowerFilename.size() >= 3 &&
                                 lowerFilename.substr(lowerFilename.size() - 3) == ".nc")
                        {
                            std::vector<double> latitudes, longitudes;
                            double westLon = getDomain()->getLonFromX(SWX);
                            double eastLon = getDomain()->getLonFromX(NEX);
                            double southLat = getDomain()->getLatFromY(SWY);
                            double northLat = getDomain()->getLatFromY(NEY);
                            size_t nlat = (enj > 0) ? enj : 100;
                            size_t nlon = (eni > 0) ? eni : 100;
                            double dLat = (northLat - southLat) / (nlat - 1);
                            double dLon = (eastLon - westLon) / (nlon - 1);
    
                            for (size_t j = 0; j < nlat; j++)
                                latitudes.push_back(southLat + j * dLat);
                            for (size_t i = 0; i < nlon; i++)
                                longitudes.push_back(westLon + i * dLon);
    
                            writeNetCDF(filename.c_str(), parameter, matrix, latitudes, longitudes);
                        }
                        // e.g. ASCII
                        else if (lowerFilename.size() >= 4 &&
                                 lowerFilename.substr(lowerFilename.size() - 4) == ".asc")
                        {
                            writeASCII(filename.c_str(), matrix, SWX, SWY, NEX, NEY);
                        }
                        else
                        {
                            std::cerr << "Unsupported file extension: " << filename << std::endl;
                        }
    
                        // Optional projectionOut
                        if (argMap.find("projectionOut") != argMap.end())
                        {
                            double westLon = getDomain()->getLonFromX(SWX);
                            double eastLon = getDomain()->getLonFromX(NEX);
                            double southLat = getDomain()->getLatFromY(SWY);
                            double northLat = getDomain()->getLatFromY(NEY);
    
                            std::string projectionPath = argMap["projectionOut"];
                            if (projectionPath == "json")
                            {
                                double minVal = minMax[0];
                                double maxVal = minMax[1];
                                *currentSession.outStream << "{\"SWlon\":" << westLon
                                                          << ",\"SWlat\":" << southLat
                                                          << ",\"NElon\":" << eastLon
                                                          << ",\"NElat\":" << northLat 
                                                          << ",\"minVal\":" << minVal
                                                          << ",\"maxVal\":" << maxVal
                                                          << "}" << std::endl;
                            }
                            else if (projectionPath.size() >= 4 &&
                                     projectionPath.substr(projectionPath.size() - 4) == ".kml")
                            {
                                std::ofstream kmlFile(projectionPath);
                                if (kmlFile.is_open())
                                {
                                    kmlFile << "<Document>\n"
                                            << "  <TimeStamp><when>"
                                            << simParam->FormatISODate(simParam->getDouble("refTime"),
                                                                       simParam->getInt("refYear"),
                                                                       simParam->getInt("refDay"))
                                            << "</when></TimeStamp>\n"
                                            << "  <GroundOverlay>\n"
                                            << "    <name>" << parameter << "</name>\n"
                                            << "    <Icon>\n"
                                            << "      <href>" << filename << "</href>\n"
                                            << "      <viewBoundScale>1</viewBoundScale>\n"
                                            << "    </Icon>\n"
                                            << "    <LatLonBox>\n"
                                            << "      <north>" << northLat << "</north>\n"
                                            << "      <south>" << southLat << "</south>\n"
                                            << "      <east>" << eastLon << "</east>\n"
                                            << "      <west>" << westLon << "</west>\n"
                                            << "    </LatLonBox>\n"
                                            << "  </GroundOverlay>\n"
                                            << "</Document>\n";
                                    kmlFile.close();
                                }
                                else
                                {
                                    std::cerr << "Error writing KML file to " << projectionPath << std::endl;
                                }
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "Error: No data available for parameter '" << parameter << "'." << std::endl;
                    }
                }
            }
            else
            {
                std::cerr << "Error: Filename not provided in the argument." << std::endl;
            }
        }
    
        return normal;
    }
    

    int Command::loadSimulation(const std::string &arg, size_t &numTabs)
    {
        if (getDomain() == nullptr)
            return normal;
        string ncfilePath = getString("arrival_time_of_front", arg);
        cout << getDomain()->getDomainID() << ": " << "loading " << ncfilePath << endl;
        getDomain()->loadArrivalTimeNC(ncfilePath); // Default save operation if no arguments provided
        return normal;
    }

    int Command::saveSimulation(const std::string &arg, size_t &numTabs)
    {
        if (!arg.empty())
        {
            return saveData(arg, numTabs);
        }
        else
        {
            getDomain()->saveArrivalTimeNC(); // Default save operation if no arguments provided
        }

        return normal;
    }

    int Command::setParameters(const string &arg, size_t &numTabs)
    {
        // Getting all the arguments, using the 'tokenize' function
        // Returns strings of the form 'arg=...'
        vector<string> tmpArgs;
        string delimiter = ";";
        tokenize(arg, tmpArgs, delimiter);
        for (size_t i = 0, size = tmpArgs.size(); i < size; ++i)
        {
            setParameter(tmpArgs[i], numTabs);
        }
        return normal;
    }

    int Command::computeModelSpeed(const std::string &cmd, size_t &numTabs)
    {
        // Expected cmd format: "ModelKey;value1;value2;value3;..."

        // Step 1: Tokenize the cmd string by ";"
        std::vector<std::string> tokens;
        std::string delimiter = ";";
        size_t start = 0;
        size_t end = cmd.find(delimiter);

        while (end != std::string::npos)
        {
            tokens.emplace_back(cmd.substr(start, end - start));
            start = end + delimiter.length();
            end = cmd.find(delimiter, start);
        }
        tokens.emplace_back(cmd.substr(start, end - start)); // Add the last token

        if (tokens.empty())
        {
            std::cerr << "Error: No command arguments provided." << std::endl;
            return error;
        }

        // Step 2: The first token is the model key
        std::string modelKey = currentSession.params->getParameter("propagationModel");

        // Step 3: Retrieve the propagation model
        PropagationModel *model = getDomain()->getPropagationModel(modelKey);
        if (model == nullptr)
        {
            std::cerr << "Error: Propagation model with key '" << modelKey << "' not found." << std::endl;
            return error;
        }

        // Step 4: Convert the remaining tokens to double values
        std::vector<double> values;
        values.reserve(tokens.size());
        for (size_t i = 0; i < tokens.size(); ++i)
        {
            try
            {
                double val = std::stod(tokens[i]);
                values.push_back(val);
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Error: Invalid number format '" << tokens[i] << "'" << std::endl;
                return error;
            }
            catch (const std::out_of_range &e)
            {
                std::cerr << "Error: Number out of range '" << tokens[i] << "'" << std::endl;
                return error;
            }
        }

        // Step 5: Prepare the double array
        double *test_values = values.data();

        // Step 6: Call the getSpeed function of the selected model
        double speedV = model->getSpeed(test_values);

        ostringstream oss;
        oss << speedV;
        // Step 7: Output the result
        *currentSession.outStream << oss.str();
        // *currentSession.outStream<< speed << std::endl;

        return normal;
    }
    int Command::setParameter(const string &arg, size_t &numTabs)
    {
        // Getting all the arguments, using the 'tokenize' function
        // Returns strings of the form 'arg=...'
        vector<string> tmpArgs;
        string delimiter = "=";
        tokenize(arg, tmpArgs, delimiter);
        if (tmpArgs.size() > 2)
        {
            cout << "problem in the number of arguments when setting " << arg << endl;
            return error;
        }

        // Check if the parameter value begins with '!'
        if (!tmpArgs[1].empty() && tmpArgs[1][0] == '!')
        {
            // Remove the leading '!' to get the actual value.
            std::string value = tmpArgs[1].substr(1);
            // Set the parameter with a protection flag (true).
            currentSession.params->setParameter(tmpArgs[0], value, true);
            std::cout << "Parameter " << tmpArgs[0]
                      << " set to fixed protected value " << value << std::endl;
        }
        else
        {
            // Set the parameter normally (without protection).
            currentSession.params->setParameter(tmpArgs[0], tmpArgs[1], false);
        }

        currentSession.params->setParameter(tmpArgs[0], tmpArgs[1]);
        return normal;
    }

    int Command::getParameter(const string &arg, size_t &numTabs)
    {
        if (arg.empty())
        {
            cout << "You have to specify one argument" << endl;
            return error;
        }
    
        // If the argument is "parameterNames", output a comma-separated list of parameter keys.
        if (arg == "parameterNames")
        {
            // Assuming currentSession.params provides a way to retrieve all parameter names.
            // For example, if params is a std::map<string, string>, we can iterate through it.
            vector<string> names = currentSession.params->getAllKeys();
            std::string result;
            for (size_t i = 0; i < names.size(); i++)
            {
                result += names[i];
                if (i != names.size() - 1)
                    result += ", ";
            }
            *currentSession.outStream << result;
            return normal;
        }
        // If the argument is "layerNames", output a comma-separated list of layer names.
        else if (arg == "layerNames")
        {
            vector<string> names = getDomain()->getDataBroker()->getAllLayerNames();
            std::string result;
            for (size_t i = 0; i < names.size(); i++)
            {
                result += names[i];
                if (i != names.size() - 1)
                    result += ", ";
            }
            *currentSession.outStream << result;
            return normal;
        }
        else
        {
            // Otherwise, retrieve the requested parameter.
            string param = currentSession.params->getParameter(arg);
            if (param == "1234567890")
            {
                cout << "Parameter doesn't exist : " << arg << endl;
                return error;
            }
            *currentSession.outStream << param;
            return normal;
        }
    }

    int Command::triggerValue(const string &arg, size_t &numTabs)
    {

        vector<string> tmpArgs;
        string delimiter = ";";
        tokenize(arg, tmpArgs, delimiter);
        if (tmpArgs.size() < 2)
        {
            return error;
        }

        if (tmpArgs[0] == "wind")
        {
            FFVector a = getVector("vel", arg);
            FFPoint b = pointError; //("loc", arg);

            if (getDomain() != 0)
            {
                if (getDomain()->getDataBroker() != 0)
                {
                    if (getDomain()->getDataBroker()->PwindULayer != 0)
                    {
                        getDomain()->getDataBroker()->PwindULayer->setProjectionDirVector(a, b);
                    }
                    if (getDomain()->getDataBroker()->PwindVLayer != 0)
                    {
                        getDomain()->getDataBroker()->PwindVLayer->setProjectionDirVector(a, b);
                    }
                    return normal;
                }
            }
        }

        if (tmpArgs[0] == "fuel")
        {

            if (getDomain()->getDataBroker() != 0)
            {

                vector<string> tmpVal;
                string delimiter = "=";
                double val = 0;
                tokenize(tmpArgs[1], tmpVal, delimiter);
                istringstream iss(tmpVal[1]);
                if (iss >> val)
                {
                    getDomain()->updateFuelTable(tmpVal[0], val);

                    return normal;
                }
                return error;
            }
        }
        if (tmpArgs[0] == "fuelIndice")
        {
            if (getDomain()->getDataBroker() != 0)
            {
                FFPoint loc = getPoint("loc", arg);
                int fvalue = getInt("fuelType", arg);
                getDomain()->getDataBroker()->getLayer("fuel")->setValueAt(loc, 0.0, fvalue);
                return normal;
            }
        }
        return error;
    }
    int Command::include(const string &arg, size_t &numTabs)
    {
        /* the commands are read from a defined file
         *  testing for the presence of the file */
        ifstream instream(arg.c_str());
        if (instream)
        {
            // reading all the commands (one command per line) of the input file
            string line;
            // size_t numLine = 0;
            while (getline(instream, line))
            {
                // while ( getline( cin, line ) ) {
                // numLine++;
                // checking for comments or newline
                if ((line[0] == '#') || (line[0] == '*') || (line[0] == '\n'))
                    continue;
                // treating the command of the current line
                ExecuteCommand(line);
            }
        }
        else
        {
            cout << "wrong input file, check your settings..." << endl;
        }
        return normal;
    }

    int Command::saveData(const std::string &arg, size_t &numTabs)
    {
        // Check for an argument
        if (arg.empty())
        {
            std::cout << "You have to specify the filename for saving the data" << std::endl;
            return error;
        }

        // Parse the argument string (key=value pairs separated by ';')
        std::map<std::string, std::string> argMap;
        std::string temp = arg;
        size_t pos = 0;
        std::string token;
        const char delim = ';';
        const char assign = '=';

        while ((pos = temp.find(delim)) != std::string::npos)
        {
            token = temp.substr(0, pos);
            size_t eq_pos = token.find(assign);
            if (eq_pos != std::string::npos)
            {
                std::string key = token.substr(0, eq_pos);
                std::string value = token.substr(eq_pos + 1);
                argMap[key] = value;
            }
            temp.erase(0, pos + 1);
        }
        // Process the last token (if any)
        size_t eq_pos = temp.find(assign);
        if (eq_pos != std::string::npos)
        {
            std::string key = temp.substr(0, eq_pos);
            std::string value = temp.substr(eq_pos + 1);
            argMap[key] = value;
        }

        // Extract the filename
        std::string filename = argMap["filename"];
        if (filename.empty())
        {
            std::cout << "Filename argument is missing" << std::endl;
            return error;
        }

        // --- Parse optional "fields" option ---
        // Default: altitude, fuel, wind
        std::vector<std::string> fieldsToSave;
        if (argMap.find("fields") != argMap.end())
        {
            std::istringstream iss(argMap["fields"]);
            std::string field;
            while (std::getline(iss, field, ','))
            {
                if (!field.empty())
                {
                    fieldsToSave.push_back(field);
                    std::cout << "saving " << field << std::endl;
                }
            }
        }
        else
        {
            fieldsToSave = {"altitude", "fuel"};
        }

        // --- Parse optional "compression" option ---
        // Default compression level is 0 (no compression)
        int compressionLevel = 0;
        if (argMap.find("compression") != argMap.end())
        {
            try
            {
                compressionLevel = std::stoi(argMap["compression"]);
            }
            catch (...)
            {
                compressionLevel = 0;
            }
            if (compressionLevel < 0)
                compressionLevel = 0;
            if (compressionLevel > 10)
                compressionLevel = 10;
        }

        try
        {
            // Create (or replace) the NetCDF file.
            // Note: Compression requires NetCDF-4. You might need to specify a mode
            // such as NcFile(filename.c_str(), NcFile::replace | NcFile::nc4) depending on your library.
            NcFile dataFile(filename.c_str(), NcFile::replace);

            // --- Write Domain information ---
            auto domain = getDomain();
            if (!domain)
            {
                std::cout << "No domain available to save." << std::endl;
                return error;
            }
            SimulationParameters *simParam = SimulationParameters::GetInstance();

            NcDim stringDim = dataFile.addDim("string1", 1);
            NcVar domVar = dataFile.addVar("domain", ncChar, stringDim);
            const char *domStr = "domain";
            domVar.putVar(domStr);

            FFPoint SWCorner = domain->getSWCorner();
            domVar.putAtt("SWx", NC_FLOAT, static_cast<float>(SWCorner.getX()));
            domVar.putAtt("SWy", NC_FLOAT, static_cast<float>(SWCorner.getY()));
            domVar.putAtt("SWz", NC_FLOAT, static_cast<float>(SWCorner.getZ()));
            double Lx = simParam->getDouble("Lx");
            double Ly = simParam->getDouble("Ly");
            double Lz = simParam->getDouble("Lz");
            domVar.putAtt("Lx", NC_FLOAT, static_cast<float>(Lx));
            domVar.putAtt("Ly", NC_FLOAT, static_cast<float>(Ly));
            domVar.putAtt("Lz", NC_FLOAT, static_cast<float>(Lz));
            double t0 = simParam->getDouble("t0");
            double Lt = simParam->getDouble("Lt");
            domVar.putAtt("t0", NC_FLOAT, static_cast<float>(t0));
            domVar.putAtt("Lt", NC_FLOAT, static_cast<float>(Lt));

            domVar.putAtt("type", "domain");

            std::string wsenlbrt = simParam->getParameter("WSENLBRT");
            if (!wsenlbrt.empty())
            {
                domVar.putAtt("WSENLBRT", wsenlbrt.c_str());
            }

            // --- Save Selected Data Layers ---

            // 1) Fuel Layer
            if (std::find(fieldsToSave.begin(), fieldsToSave.end(), "fuel") != fieldsToSave.end())
            {
                FuelDataLayer<double> *fuelLayer = dynamic_cast<FuelDataLayer<double> *>(domain->getDataLayer("fuel"));
                if (fuelLayer)
                {
                    int *fuelMap = fuelLayer->getFuelMap();
                    int ft = fuelLayer->getDim("t"); // expected to be 1
                    int fz = fuelLayer->getDim("z"); // expected to be 1
                    int fy = fuelLayer->getDim("y"); // e.g., 6400
                    int fx = fuelLayer->getDim("x"); // e.g., 6400
                    size_t total_size = static_cast<size_t>(ft) * fz * fy * fx;
                    std::vector<short> reshaped_fuel(total_size);
                    for (int ti = 0; ti < ft; ++ti)
                    {
                        for (int z = 0; z < fz; ++z)
                        {
                            for (int y = 0; y < fy; ++y)
                            {
                                for (int x = 0; x < fx; ++x)
                                {
                                    size_t c_index = x + fx * (y + fy * (z + fz * ti));
                                    size_t fortran_index = ti + ft * (z + fz * (y + fy * x));
                                    reshaped_fuel[c_index] = static_cast<short>(fuelMap[fortran_index]);
                                }
                            }
                        }
                    }
                    NcDim dim_ft = dataFile.addDim("ft", ft);
                    NcDim dim_fz = dataFile.addDim("fz", fz);
                    NcDim dim_fy = dataFile.addDim("fy", fy);
                    NcDim dim_fx = dataFile.addDim("fx", fx);
                    std::vector<NcDim> fuelDims = {dim_ft, dim_fz, dim_fy, dim_fx};
                    NcVar fuelVar = dataFile.addVar("fuel", ncShort, fuelDims);
                    fuelVar.putAtt("type", "fuel");
                    if (compressionLevel > 0)
                    {
                        // Use NetCDF-4 compression (no shuffle, deflate enabled)
                        fuelVar.setCompression(false, true, compressionLevel);
                    }
                    fuelVar.putVar(reshaped_fuel.data());
                }
            }

            // 2) Wind: combine windU and windV into one variable "wind"
            // The expected netCDF variable "wind" has dimensions:
            //    wind_dimensions (size 2), wind_directions, wind_rows, wind_columns
            if (std::find(fieldsToSave.begin(), fieldsToSave.end(), "wind") != fieldsToSave.end())
            {
                DataLayer<double> *windULayer = domain->getDataLayer("windU");
                DataLayer<double> *windVLayer = domain->getDataLayer("windV");
                if (windULayer && windVLayer)
                {
                    FFArray<double> *srcWindU = nullptr;
                    FFArray<double> *srcWindV = nullptr;
                    windULayer->getMatrix(&srcWindU, 0);
                    windVLayer->getMatrix(&srcWindV, 0);
                    if (srcWindU && srcWindV)
                    {
                        // For saving wind, we force each wind component's "time" dimension to 1.
                        // We then set the combined wind's first dimension to 2.
                        int compDim = 1;                       // each component (windU or windV) is 1 in its "t" dimension
                        int wind_dir = srcWindU->getDim("z");  // maps to wind_directions
                        int wind_rows = srcWindU->getDim("y"); // maps to wind_rows
                        int wind_cols = srcWindU->getDim("x"); // maps to wind_columns
                        size_t comp_size = static_cast<size_t>(compDim) * wind_dir * wind_rows * wind_cols;

                        // Reshape windU (using a loop similar to your existing reshaping code)
                        double *dataU = srcWindU->getData();
                        std::vector<float> reshaped_windU(comp_size);
                        for (int d = 0; d < compDim; ++d)
                        {
                            for (int dir = 0; dir < wind_dir; ++dir)
                            {
                                for (int r = 0; r < wind_rows; ++r)
                                {
                                    for (int c = 0; c < wind_cols; ++c)
                                    {
                                        size_t c_index = c + wind_cols * (r + wind_rows * (dir + wind_dir * d));
                                        size_t fortran_index = d + compDim * (dir + wind_dir * (r + wind_rows * c));
                                        reshaped_windU[c_index] = static_cast<float>(dataU[fortran_index]);
                                    }
                                }
                            }
                        }

                        // Reshape windV similarly
                        double *dataV = srcWindV->getData();
                        std::vector<float> reshaped_windV(comp_size);
                        for (int d = 0; d < compDim; ++d)
                        {
                            for (int dir = 0; dir < wind_dir; ++dir)
                            {
                                for (int r = 0; r < wind_rows; ++r)
                                {
                                    for (int c = 0; c < wind_cols; ++c)
                                    {
                                        size_t c_index = c + wind_cols * (r + wind_rows * (dir + wind_dir * d));
                                        size_t fortran_index = d + compDim * (dir + wind_dir * (r + wind_rows * c));
                                        reshaped_windV[c_index] = static_cast<float>(dataV[fortran_index]);
                                    }
                                }
                            }
                        }

                        // Combine the two components into one array.
                        size_t total_size = 2 * comp_size;
                        std::vector<float> combined_wind(total_size);
                        for (size_t i = 0; i < comp_size; i++)
                        {
                            combined_wind[i] = reshaped_windU[i];
                            combined_wind[i + comp_size] = reshaped_windV[i];
                        }

                        // Create wind variable dimensions:
                        NcDim dim_wind_dimensions = dataFile.addDim("wind_dimensions", 2);
                        NcDim dim_wind_directions = dataFile.addDim("wind_directions", wind_dir);
                        NcDim dim_wind_rows = dataFile.addDim("wind_rows", wind_rows);
                        NcDim dim_wind_columns = dataFile.addDim("wind_columns", wind_cols);
                        std::vector<NcDim> windDims = {dim_wind_dimensions, dim_wind_directions, dim_wind_rows, dim_wind_columns};

                        NcVar windVar = dataFile.addVar("wind", ncFloat, windDims);
                        windVar.putAtt("type", "wind");
                        float fillValue = NAN;
                        windVar.putAtt("_FillValue", ncFloat, fillValue);
                        if (compressionLevel > 0)
                        {
                            windVar.setCompression(false, true, compressionLevel);
                        }
                        windVar.putVar(combined_wind.data());
                    }
                }
                else
                {
                    std::cout << "Wind layers (windU/windV) not available to save." << std::endl;
                }
            }

            // 3) Altitude Layer
            if (std::find(fieldsToSave.begin(), fieldsToSave.end(), "altitude") != fieldsToSave.end())
            {
                DataLayer<double> *altLayer = domain->getDataLayer("altitude");
                if (altLayer)
                {
                    FFArray<double> *srcAlt = nullptr;
                    altLayer->getMatrix(&srcAlt, 0);
                    if (srcAlt)
                    {
                        double *data = srcAlt->getData();
                        int nt = srcAlt->getDim("t");
                        int nz = srcAlt->getDim("z");
                        int ny = srcAlt->getDim("y");
                        int nx = srcAlt->getDim("x");
                        size_t total_size = static_cast<size_t>(nt) * nz * ny * nx;
                        std::vector<short> reshaped_alt(total_size);
                        for (int ti = 0; ti < nt; ++ti)
                        {
                            for (int z = 0; z < nz; ++z)
                            {
                                for (int y = 0; y < ny; ++y)
                                {
                                    for (int x = 0; x < nx; ++x)
                                    {
                                        size_t c_index = x + nx * (y + ny * (z + nz * ti));
                                        size_t fortran_index = ti + nt * (z + nz * (y + ny * x));
                                        reshaped_alt[c_index] = static_cast<short>(data[fortran_index]);
                                    }
                                }
                            }
                        }
                        NcDim dim_nt = dataFile.addDim("nt", nt);
                        NcDim dim_nz = dataFile.addDim("nz", nz);
                        NcDim dim_ny = dataFile.addDim("ny", ny);
                        NcDim dim_nx = dataFile.addDim("nx", nx);
                        std::vector<NcDim> altDims = {dim_nt, dim_nz, dim_ny, dim_nx};
                        NcVar altVar = dataFile.addVar("altitude", ncShort, altDims);
                        altVar.putAtt("type", "data");
                        if (compressionLevel > 0)
                        {
                            altVar.setCompression(false, true, compressionLevel);
                        }
                        altVar.putVar(reshaped_alt.data());
                    }
                }
            }

            std::cout << "Data successfully saved to " << filename << std::endl;
            return 0;
        }
        catch (NcException &e)
        {
            std::cout << "NetCDF error: " << e.what() << std::endl;
            return error;
        }
    }

    int Command::loadData(const string &arg, size_t &numTabs)
    {

        if (arg.size() == 0)
        {
            cout << "You have to specify the path to the data" << endl;
            return error;
        }

        vector<string> args;
        tokenize(arg, args, ";");

        if (args.size() > 2)
        {
            cout << "LoadData Warning : expecting 1 or 2 arguments" << endl;
            return error;
        }

        SimulationParameters *simParam = SimulationParameters::GetInstance();
        string path = simParam->GetPath(args[0]);

        if (std::ifstream(path.c_str()).fail())
        {
            cout << "File " << path << " doesn't exist or no longer available" << endl;
            return error;
        }
        if (args.size() == 2)
        {

            simParam->setParameter("NetCDFfile", args[0]);
            try
            {
                NcFile dataFile(path.c_str(), NcFile::read);
                if (!dataFile.isNull())
                {
                    // Retrieve and process the "domain" variable attributes.
                    NcVar domVar = dataFile.getVar("domain");
                    if (!domVar.isNull())
                    {
                        map<string, NcVarAtt> attributeList = domVar.getAtts();
                        for (auto myIter = attributeList.begin(); myIter != attributeList.end(); ++myIter)
                        {
                            NcVarAtt att = myIter->second;
                            NcType attValType = att.getType();
                            string attsVal;
                            int attiVal;
                            float attfVal;
                            switch ((int)attValType.getTypeClass())
                            {
                            case NC_CHAR:
                                att.getValues(attsVal);
                                simParam->setParameter(myIter->first, attsVal);
                                break;
                            case NC_INT:
                                att.getValues(&attiVal);
                                simParam->setParameter(myIter->first, std::to_string(attiVal));
                                break;
                            case NC_FLOAT:
                                att.getValues(&attfVal);
                                simParam->setParameter(myIter->first, std::to_string(attfVal));
                                break;
                            case NC_DOUBLE:
                                att.getValues(&attfVal);
                                simParam->setParameter(myIter->first, std::to_string(attfVal));
                                break;
                            default:
                                std::cout << myIter->first << " attribute of unhandled type " << attValType.getName() << std::endl;
                                break;
                            }
                        }
                    }

                    // Check for dimensions "nx" and "ny" and set parameters accordingly.
                    NcDim nxDim = dataFile.getDim("nx");
                    NcDim nyDim = dataFile.getDim("ny");

                    if (!nxDim.isNull() && !nyDim.isNull())
                    {
                        size_t ncNX = nxDim.getSize();
                        size_t ncNY = nyDim.getSize();
                        simParam->setParameter("AtmoNX", std::to_string(ncNX));
                        simParam->setParameter("AtmoNY", std::to_string(ncNY));
                    }
                }
            }
            catch (NcException &e)
            {
                std::cerr << e.what() << std::endl;
            }

            double secs;
            int year, yday;
            if (simParam->ISODateDecomposition(args[1], secs, year, yday))
            {
                simParam->setInt("refYear", year);
                simParam->setInt("refDay", yday);
                simParam->setInt("refTime", secs);
                simParam->setParameter("ISOdate", args[1]);
                // cout<<"loading at time "<<args[1]<<" " <<"yday"<<endl;
            }

            string com = "FireDomain[sw=(" + simParam->getParameter("SWx") + ".," + simParam->getParameter("SWy") + ".," + simParam->getParameter("SWz") + ".);ne=(";
            {
                std::ostringstream oss;
                oss << (simParam->getDouble("Lx") + simParam->getDouble("SWx"));
                oss << ".,";
                oss << (simParam->getDouble("Ly") + simParam->getDouble("SWy"));
                oss << ".,";
                oss << (simParam->getDouble("Lz") + simParam->getDouble("SWz"));
                oss << ".);t=" + simParam->getParameter("t0") + ".]";
                com += oss.str();
            }

            ExecuteCommand(com);
        }
        if (args.size() == 1)
        {
            // this is the case to load layers but the domain is already loaded and time set
            if (currentSession.fd != 0)
            {
                currentSession.fd->getDataBroker()->loadFromNCFile(path.c_str());
            }
        }
        return normal;
    }

    int Command::clear(const string &arg, size_t &numTabs)
    {
        delete currentSession.fd;
        delete currentSession.outStrRep;
        delete currentSession.sim;
        delete currentSession.params;
        return normal;
    }

    int Command::quit(const string &arg, size_t &numTabs)
    {
        cout << "*** QUITTING FOREFIRE, GOODBYE ***" << endl;
        delete currentSession.fd;
        delete currentSession.outStrRep;
        delete currentSession.sim;
        delete currentSession.params;
        exit(0);
        return normal;
    }

    void Command::setOstringstream(ostringstream *oss)
    {
        currentSession.outStream = oss;
    }

    void Command::tokenize(const string &str, vector<string> &tokens,
                           const string &delimiter = " ")
    {
        // Skip delimiters at beginning.
        string::size_type lastPos = str.find_first_not_of(delimiter, 0);
        // Find first "non-delimiter".
        string::size_type pos = str.find_first_of(delimiter, lastPos);

        while (string::npos != pos || string::npos != lastPos)
        {
            // Found a token, add it to the vector.
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            // Skip delimiters.  Note the "not_of"
            lastPos = str.find_first_not_of(delimiter, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiter, lastPos);
        }
    }

    string Command::getString(string opt, string arg)
    {
        vector<string> tmpArgs;
        string tmparg;
        // Getting all the arguments, using the 'tokenize' function
        // Returns strings of the form 'arg=...'
        string delimiter1 = ";";
        tokenize(arg, tmpArgs, delimiter1);
        // Checking every 'tmpArgs' to see if it contains 'opt'
        // on the right-hand-side of 'arg=...' (i.e. arg=opt)
        vector<string> inArg;
        for (size_t i = 0, size = tmpArgs.size(); i < size; ++i)
        {
            string delimiter2 = "=";
            tokenize(tmpArgs[i], inArg, delimiter2);
            if (opt.compare(inArg[0]) == 0)
            {
                return inArg[1];
            }
            else
            {
                inArg.clear();
            }
        }
        return stringError;
    }

    int Command::getInt(string opt, string arg)
    {
        /* The 'getFloat()' method simply casts the results
         * of the search for the option 'opt' given by 'getString()' */
        string tmpstr = getString(opt, arg);
        if (tmpstr == stringError)
            return INTERROR;
        string s;
        string::reverse_iterator it = tmpstr.rbegin();
        if (*it == 's')
        {
            // removing it
            size_t len = tmpstr.length();
            s = tmpstr.substr(0, len - 1);
        }
        else
        {
            s = tmpstr;
        }
        istringstream iss(s);
        int d;
        if (iss >> d)
        {
            return d;
        }
        else
        {
            return INTERROR;
        }
    }

    double Command::getFloat(string opt, string arg)
    {
        /* The 'getFloat()' method simply casts the results
         * of the search for the option 'opt' given by 'getString()' */
        string tmpstr = getString(opt, arg);
        if (tmpstr == stringError)
            return FLOATERROR;
        string s;
        string::reverse_iterator it = tmpstr.rbegin();
        if (*it == 's')
        {
            // removing it
            size_t len = tmpstr.length();
            s = tmpstr.substr(0, len - 1);
        }
        else
        {
            s = tmpstr;
        }
        istringstream iss(s);
        double d;
        if (iss >> d)
        {
            return d;
        }
        else
        {
            return FLOATERROR;
        }
    }

    FFPoint Command::getPoint(string opt, string arg)
    {
        // Get the point string in the form "(...,...,...)"
        string tmpstr = getString(opt, arg);
        if (tmpstr == stringError)
        {
            return pointError;
        }

        // Remove the surrounding parentheses.
        if (tmpstr.size() < 2)
        {
            return pointError;
        }
        string point = tmpstr.substr(1, tmpstr.size() - 2);

        // Tokenize the string using a comma delimiter.
        vector<string> tokens;
        tokenize(point, tokens, ",");
        if (tokens.size() != 3)
        {
            return pointError;
        }

        // Convert tokens to double.
        double v1, v2, v3;
        istringstream iss1(tokens[0]);
        istringstream iss2(tokens[1]);
        istringstream iss3(tokens[2]);
        if (!(iss1 >> v1 && iss2 >> v2 && iss3 >> v3))
        {
            return pointError;
        }

        // Check the option to determine how to interpret the values.
      
        if (opt == "lonlat")
        {
            // Convert spherical (lon, lat) to Cartesian coordinates (x, y).
            double x = getDomain()->getXFromLon(v1);
            double y = getDomain()->getYFromLat(v2);
   
            return FFPoint(x, y, v3);
        }
        else
        {
            // Otherwise assume the provided values are already Cartesian.
            return FFPoint(v1, v2, v3);
        }
    }

    FFVector Command::getVector(string opt, string arg)
    {
        /* The 'getPoint()' method casts the results of the
         * search for the option 'opt' given by 'getString()';
         * as this result is in the form "(...,...,...)"
         * this string has to be tokenized */

        string tmpstr = getString(opt, arg);
        if (tmpstr == stringError)
            return vectorError;
        // droping the parenthesis
        string vec = tmpstr.substr(1, getString(opt, arg).size() - 2);

        // Getting all the arguments, using the 'tokenize' function
        vector<string> v;
        string delimiter = ",";
        tokenize(vec, v, delimiter);
        // Passing the 'strings' into 'istringstream' for cast into 'double'
        double vx, vy, vz;
        istringstream vsx(v[0]);
        istringstream vsy(v[1]);
        istringstream vsz(v[2]);
        if ((vsx >> vx) && (vsy >> vy) && (vsz >> vz))
        {
            return FFVector(vx, vy, vz);
        }
        else
        {
            return vectorError;
        }
    }

    size_t Command::argCount(string arg)
    {
        vector<string> tmpArgs;
        string tmparg;
        // Getting all the arguments, using the 'tokenize' function
        // Returns strings of the form 'arg=...'
        string delimiter = ";";
        tokenize(arg, tmpArgs, delimiter);
        return tmpArgs.size();
    }

    void Command::setStartTime(const double &t)
    {
        startTime = t;
    }

    void Command::setReferenceTime(const double &t)
    {
        refTime = t;
    }

    double Command::getTime()
    {
        return startTime;
    }

    size_t Command::tabsCount(string cmdLine)
    {
        size_t count = 0;
        size_t pos = 0;
        while (pos < cmdLine.size())
        {
            if (cmdLine[pos] == '\t')
            {
                count++;
                pos++;
            }
            else if (pos + 3 < cmdLine.size() && cmdLine.substr(pos, 4) == "    ")
            {
                count++;
                pos += 4;
            }
            else
            {
                break;
            }
        }
        if (firstCommand)
        {
            refTabs = count;
            firstCommand = false;
        }
        return count - refTabs;
    }

    string Command::removeTabs(string arg)
    {
        size_t pos = 0;
        while (pos < arg.size())
        {
            if (arg[pos] == '\t')
            {
                arg.erase(pos, 1);
            }
            else if (pos + 3 < arg.size() && arg.substr(pos, 4) == "    ")
            {
                arg.erase(pos, 4);
            }
            else
            {
                break;
            }
        }
        return arg;
    }

    void Command::ExecuteCommand(string &line)
    {
        /* The method 'ExecuteCommands' uses the 'tokenize' class
         * to execute the command given by the user
         * in the 'line'. If first looks at the possible options
         * (listed in the tables 'short_options' and 'long_options')
         * and then call the corresponding function */

        // Getting all the arguments, using the 'tokenize' function
        if (line.size() == 0)
            return;
        vector<string> command;
        size_t numTabs;
        string scmd;
        string postcmd;
        const string delimiter = "[";
        tokenize(line, command, delimiter);
        if (command.size() > 1)
        {
            // counting and removing the tabs for the level of the command
            numTabs = tabsCount(command[0]);
            scmd = removeTabs(command[0]);
            if (command[1].size() > 1)
            {
                // dropping the last parenthesis ']'
                postcmd = command[1].substr(command[1].rfind("]") + 1, -1);
                command[1] = command[1].substr(0, command[1].rfind("]"));
            }
            else
            {
                command[1] = "";
            }
        }
        else
        {
            scmd = removeTabs(line);
        }

        // calling the right method using the 'translator'
        if (((scmd)[0] == '#') || ((scmd)[0] == '*') || ((scmd)[0] == '\n') || ((scmd)[0] == '\r'))
        {
            // this line is commented, nothing to do
        }
        else
        {
            if (postcmd.size() > 1)
            {
                string atCommand = line.substr(0, line.rfind("]") + 1);
                string whenCommand = line.substr(line.rfind("]") + 1, -1);
                double whenDouble = getFloat("@t", whenCommand);
                if ((whenCommand)[0] == '@' && whenDouble != FLOATERROR)
                {
                    currentSession.tt->insert(new FFEvent(new EventCommand(atCommand, whenDouble)));
                }
                else
                {
                    cout << whenDouble << "unknown post operator  >" << whenCommand << "< , try 'help[]'." << endl;
                }
            }
            else
            {
                commandMap::const_iterator curcmd = translator.find(scmd);
                if (curcmd == translator.end())
                {
                    if (scmd.at(0) != '!')
                        cout << "unknown command  >" << scmd << "< , try 'help[]'." << endl;
                }
                else
                {
                    try
                    {
                        // calling the right function
                        (curcmd->second)(command[1], numTabs);
                    }
                    catch (BadOption &)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "argument(s) '" << command[1] << "' is (are) not fit for command '"
                             << command[0] << "'" << endl;
                        cout << "type 'man[" << command[0] << "]' for more information" << endl;
                    }
                    catch (MissingOption &mo)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "command '" << command[0] << "' misses "
                             << mo.num << " options." << endl;
                        cout << "type 'man[" << command[0] << "]' for more information" << endl;
                    }
                    catch (MissingTime &)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << "you have to specify a time for that command (as in 't=0.')" << endl;
                    }
                    catch (...)
                    {
                        cout << getDomain()->getDomainID() << ": "
                             << " PROBLEM: Command associated to " << line << " ended in error" << endl;
                    }
                }
            }
        }
        command.clear();
    }

    std::string Command::executeCommandAndCaptureOutput(const std::string &cmd)
    {
        // 'cmd' is assumed to be the command text after the "ff:" prefix.
        std::streambuf *oldbuf = std::cout.rdbuf();
        std::ostringstream captured;
        std::cout.rdbuf(captured.rdbuf());
        std::string fireCommand = cmd;
        // Execute the command. (Assuming ExecuteCommand is defined elsewhere.)
        ExecuteCommand(fireCommand);

        std::cout.rdbuf(oldbuf);
        return captured.str();
    }
    /*
    std::string Command::executeCommandAndCaptureOutput(const std::string &cmd) {
        // Normalize the request URI. Remove a leading '/' if present.
        std::string path = cmd;
        if (!path.empty() && path[0] == '/') {
            path = path.substr(1);
        }
        std::cout<<">"<<cmd<<"<     >"<<path<<std::endl;
        std::string body;
        std::string contentType;

        // CASE 1: Index request.
        if (path.empty() || path == "index.html") {
            std::ifstream file("index.html", std::ios::in | std::ios::binary);
            if (file.good()) {
                std::ostringstream contents;
                contents << file.rdbuf();
                body = contents.str();
                contentType = "text/html; charset=UTF-8";
            } else {
                // Build an HTML directory listing.
                DIR *dir = opendir(".");
                if (!dir) {
                    body = "<html><head><title>ForeFire Directory Listing</title></head>"
                           "<body><h1>ForeFire Directory Listing</h1><p>Error opening directory.</p></body></html>";
                } else {
                    std::ostringstream listing;
                    listing << "<!DOCTYPE html>\n<html>\n<head>\n"
                            << "<meta charset=\"utf-8\">\n"
                            << "<title>ForeFire Directory Listing</title>\n"
                            << "<style>\n"
                            << "body { font-family: sans-serif; background-color: #f0f0f0; padding: 20px; }\n"
                            << "h1 { color: #d9534f; }\n"
                            << "table { width: 100%; border-collapse: collapse; }\n"
                            << "th, td { padding: 8px; border-bottom: 1px solid #ccc; text-align: left; }\n"
                            << "a { text-decoration: none; color: #337ab7; }\n"
                            << "a:hover { text-decoration: underline; }\n"
                            << "</style>\n"
                            << "</head>\n<body>\n"
                            << "<h1>ForeFire Directory Listing</h1>\n"
                            << "<table>\n"
                            << "<tr><th>Filename</th></tr>\n";
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != NULL) {
                        std::string name(entry->d_name);
                        if (name == "." || name == "..")
                            continue;
                        listing << "<tr><td><a href=\"/" << name << "\">" << name << "</a></td></tr>\n";
                    }
                    listing << "</table>\n</body>\n</html>\n";
                    closedir(dir);
                    body = listing.str();
                }
                contentType = "text/html; charset=UTF-8";
            }
        }
        // CASE 2: ForeFire command.
        else if (path.compare(0, 3, "ff:") == 0) {
            std::string fireCommand = path.substr(3);
            std::streambuf* oldbuf = std::cout.rdbuf();
            std::ostringstream captured;
            std::cout.rdbuf(captured.rdbuf());
            Command::ExecuteCommand(fireCommand);
            std::cout.rdbuf(oldbuf);
            body = captured.str();
            contentType = "text/plain; charset=UTF-8";
        }
        // CASE 3: File request.
        else {
            std::ifstream file(path, std::ios::in | std::ios::binary);
            if (!file.good()) {
                body = "File not found: " + path;
                contentType = "text/plain; charset=UTF-8";
            } else {
                std::ostringstream contents;
                contents << file.rdbuf();
                body = contents.str();
                size_t dotPos = path.find_last_of('.');
                if (dotPos != std::string::npos) {
                    std::string ext = path.substr(dotPos);
                    if (ext == ".html" || ext == ".htm")
                        contentType = "text/html; charset=UTF-8";
                    else if (ext == ".png" || ext == ".PNG")
                        contentType = "image/png";
                    else if (ext == ".jpg" || ext == ".jpeg" || ext == ".JPG" || ext == ".JPEG")
                        contentType = "image/jpeg";
                    else
                        contentType = "text/plain; charset=UTF-8";
                } else {
                    contentType = "text/plain; charset=UTF-8";
                }
            }
        }

        // Build Date header in RFC1123 format.
        std::time_t now = std::time(nullptr);
        char dateBuf[100];
        std::tm *gmt = std::gmtime(&now);
        std::strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmt);

        // Assemble complete HTTP response using HTTP/1.0.
        std::ostringstream responseStream;
        responseStream << "HTTP/1.0 200 OK\r\n"
                       << "Host: localhost\r\n"
                       << "Date: " << dateBuf << "\r\n"
                       << "Connection: close\r\n"
                       << "Content-Type: " << contentType << "\r\n"
                       << "Content-Length: " << body.size() << "\r\n"
                       << "\r\n"
                       << body;

        return responseStream.str();
    }
    */

    int Command::listenHTTP(const std::string &arg, size_t &numTabs)
    {

        // ID 1 is for the Fortran MPI rank 1
        int MPIMODE = 1;
        int STANDALONEMODE = 0;
        int listenMode = STANDALONEMODE;
        size_t defaultport = 8000;
        size_t port = defaultport;
        std::string defaultHostname = "localhost";

        if (currentSession.fd != 0)
        {
            if (getDomain()->getDomainID() != 0)
            {
                return normal;
            }
            else
            {
                if (currentSession.fdp != 0)
                {
                    listenMode = MPIMODE;
                    cout << "HTTP MPI rank >>>>" << getDomain()->getDomainID() << "<<" << endl;
                }
            }
        }

        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) != 0)
        {
            cout << "Cannot get hostname info " << endl;
            return error;
        }

        std::string address = arg.empty() ? std::string(hostname) + ":" + std::to_string(port) : arg;

        // Allocate a new HTTP server if none exists.
        if (currentSession.server == 0)
        {
            currentSession.server = new http_command::HttpCommandServer();
        }

        currentSession.server->setCallback(executeCommandAndCaptureOutput);

        if (!currentSession.server->listenOn(address))
        {
            std::cerr << "Failed to start HTTP command server on " << address << std::endl;
            return error;
        }
        else
        {
            std::ofstream infoFile("httpConnectInfo.txt");
            if (infoFile)
            {
                infoFile << hostname << ":" << port << std::endl;
                infoFile.close();
            }
            else
            {
                std::cerr << "Unable to open file for writing server info." << std::endl;
            }
        }

        return normal;
    }

    FireDomain *Command::getDomain()
    {
        if (currentSession.fdp != 0)
        {
            return currentSession.fdp;
        }
        return currentSession.fd;
    }

    string Command::dumpString()
    {
        return currentSession.outStrRep->outputstr.str();
        return 0;
    }

    void Command::parseColorMap(const std::string &map, std::vector<std::array<unsigned char, 4>> &colorMap)
    {
        std::istringstream iss(map);
        std::string segment;
        double value;
        unsigned r, g, b, a;

        while (std::getline(iss, segment, '['))
        {
            std::istringstream segmentStream(segment);
            if (std::sscanf(segment.c_str(), "%lf/%u,%u,%u,%u]", &value, &r, &g, &b, &a) == 5)
            {
                int index = std::min(255, std::max(0, static_cast<int>(value * 255)));
                colorMap[index] = {static_cast<unsigned char>(r), static_cast<unsigned char>(g),
                                   static_cast<unsigned char>(b), static_cast<unsigned char>(a)};
            }
        }
    }

    void Command::writeImage(const char *filename, const std::vector<std::vector<double>> &matrix,
                             double forced_min_val, double forced_max_val,
                             const std::string &colormapName)
    {
        int width = matrix.size();                               // Transposed width (original matrix height)
        int height = matrix[0].size();                           // Transposed height (original matrix width)
        std::vector<unsigned char> image(width * height * 4, 0); // *4 for RGBA

        // double minVal = std::isnan(forced_min_val) ? std::numeric_limits<double>::infinity() : forced_min_val;
        // double maxVal = std::isnan(forced_max_val) ? -std::numeric_limits<double>::infinity() : forced_max_val;

        double minVal = forced_min_val;
        double maxVal = forced_max_val;

        // Calculate dynamic min/max if needed
        if (std::isinf(minVal) && std::isinf(maxVal))
        {
            minVal = std::numeric_limits<double>::max();
            maxVal = std::numeric_limits<double>::lowest();
            for (const auto &row : matrix)
            {
                for (double val : row)
                {
                    if (val != std::numeric_limits<double>::infinity())
                    {
                        if (val < minVal)
                            minVal = val;
                        if (val > maxVal)
                            maxVal = val;
                    }
                }
            }
        }
        // cout<<"image minVal "<<minVal<<" maxVal "<<maxVal<<endl;
        //  Use the predefined colormaps or default to grayscale
        //  Retrieve colormap from map
        auto it = colormapMap.find(colormapName);
        if (it == colormapMap.end()) {
            it = colormapMap.find(currentSession.params->getParameter("defaultColormap"));
        }
        
        const ColorEntry *colorMap = it != colormapMap.end() ? it->second : greyColormap;
        
        int mapSize = colormapSize; // Assuming all colormaps have the same size

        // Apply color mapping with corrected indices and flipping vertically
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                // Flip vertically by accessing matrix from the bottom up
                double val = matrix[x][height - 1 - y]; // Adjusted for vertical flip
                int index = (y * width + x) * 4;
                if (val == std::numeric_limits<double>::infinity())
                {
                    image[index + 3] = 0; // Transparent
                }
                else
                {
                    int colorIndex = static_cast<int>((val - minVal) / (maxVal - minVal) * (mapSize - 1));
                    colorIndex = std::max(0, std::min(colorIndex, mapSize - 1));
                    /*  if ((x % 1000 == 0) && (y % 1000 == 0)) {
                          std::cout << val << " : " << colorIndex << std::endl;
                      }*/
                    const auto &color = colorMap[colorIndex];
                    image[index] = color[0];
                    image[index + 1] = color[1];
                    image[index + 2] = color[2];
                    image[index + 3] = color[3]; // Assume last byte is alpha
                }
            }
        }
        std::string filenameStr(filename);
        if (filenameStr.substr(filenameStr.length() - 4) == ".jpg")
        {
            // Save as JPEG
            stbi_write_jpg(filename, width, height, 4, image.data(), 95); // Quality 95
        }
        else
        {
            // Save as PNG by default
            stbi_write_png(filename, width, height, 4, image.data(), width * 4);
        }
    }
    void Command::writeHistogram(const char *filename, const std::vector<std::vector<double>> &matrix, int bins,
                                 double forced_min_val, double forced_max_val,
                                 const std::string &colormapName)
    {
        int width = 600;
        int height = 600;
        std::vector<unsigned char> histogram(width * height * 4, 255); // Initialize to white

        double minVal = forced_min_val;
        double maxVal = forced_max_val;

        // Calculate dynamic min/max if needed
        if (std::isinf(minVal) && std::isinf(maxVal))
        {
            minVal = std::numeric_limits<double>::max();
            maxVal = std::numeric_limits<double>::lowest();
            for (const auto &row : matrix)
            {
                for (double val : row)
                {
                    if (val != std::numeric_limits<double>::infinity())
                    {
                        if (val < minVal)
                            minVal = val;
                        if (val > maxVal)
                            maxVal = val;
                    }
                }
            }
        }

        std::vector<int> bin_counts(bins, 0);
        double bin_width = (maxVal - minVal) / bins;

        // Populate bins
        for (const auto &row : matrix)
        {
            for (double val : row)
            {
                if (val != std::numeric_limits<double>::infinity() && val < maxVal)
                {
                    int bin = std::min(bins - 1, static_cast<int>((val - minVal) / bin_width));
                    bin_counts[bin]++;
                }
            }
        }

        // Find max count for normalization
        int max_count = *std::max_element(bin_counts.begin(), bin_counts.end());

        // Retrieve colormap from map
        auto it = colormapMap.find(colormapName);
        const ColorEntry *colorMap = it != colormapMap.end() ? it->second : greyColormap;
        int mapSize = colormapSize; // Assuming all colormaps have the same size

        // Draw histogram using colormap
        for (int i = 0; i < bins; ++i)
        {
            int bar_height = static_cast<int>((static_cast<double>(bin_counts[i]) / max_count) * height);
            int colorIndex = static_cast<int>((static_cast<double>(i) / bins) * (mapSize));
            colorIndex = std::max(0, std::min(colorIndex, mapSize - 1)); // Ensure color index is within bounds
            const auto &color = colorMap[colorIndex];
            // cout<<((static_cast<double>(i))/(static_cast<double>(bins-1)))* maxVal - minVal<< " : "<<bin_counts[i]<<"   "<<i<<" : "<<bins<<endl;
            for (int y = height - 1; y > height - bar_height - 1; --y)
            {
                for (int x = i * width / bins; x < (i + 1) * width / bins; ++x)
                {
                    int index = (y * width + x) * 4;
                    histogram[index] = color[0];     // Red component
                    histogram[index + 1] = color[1]; // Green component
                    histogram[index + 2] = color[2]; // Blue component
                    histogram[index + 3] = 255;      // Alpha component (full opacity)
                }
            }
        }

        // Append 'spectrum' to filename
        std::string hist_filename = "spectrum";
        hist_filename += filename;
        stbi_write_png(hist_filename.c_str(), width, height, 4, histogram.data(), width * 4);
    }

    void Command::writeNetCDF(const char *filename, const string& varName, const std::vector<std::vector<double>> &matrix, const vector<double> &latitudes, const vector<double> &longitudes)
    {
        try
        {
            // Create (or replace) the NetCDF file using NetCDF-4 mode.
            NcFile dataFile(filename, NcFile::replace );

            // Dimensions.
            size_t nlat = latitudes.size();
            size_t nlon = longitudes.size();
            NcDim latDim = dataFile.addDim("latitude", nlat);
            NcDim lonDim = dataFile.addDim("longitude", nlon);

            // Create variables for latitude and longitude.
            NcVar latVar = dataFile.addVar("latitude", ncDouble, latDim);
            NcVar lonVar = dataFile.addVar("longitude", ncDouble, lonDim);

            // Write latitude and longitude values.
            latVar.putVar(latitudes.data());
            lonVar.putVar(longitudes.data());

            // Create the data variable with dimensions (latitude, longitude).
            vector<NcDim> dims = {latDim, lonDim};
            NcVar dataVar = dataFile.addVar(varName, ncDouble, dims);

            // Enable compression with a compression level of 6.
            dataVar.setCompression(false, true, 6);

            // Flatten the matrix into a one-dimensional array.
            vector<double> flat;
            for (size_t i = 0; i < matrix[0].size(); i++)
            {
                for (size_t j = 0; j < matrix.size(); j++)
                {
                    flat.push_back(matrix[j][i]);
                }
            }

            // Write the flattened data.
            dataVar.putVar(flat.data());
        }
        catch (NcException &e)
        {
            std::cerr << "Error writing NetCDF file: " << e.what() << std::endl;
        }
    }
    void Command::writeASCII(const char *filename, const std::vector<std::vector<double>>& matrix, double SWX, double SWY, double NEX, double NEY)
    {
        std::ofstream out(filename);
        if (!out)
        {
            std::cerr << "Error opening ASCII file: " << filename << std::endl;
            return;
        }

        // Determine the number of rows and columns.
        size_t nrows = matrix.size();
        size_t ncols = (nrows > 0) ? matrix[0].size() : 0;

        // Compute the cell size.
        // Here we assume that (NEX - SWX) corresponds to the total width of the matrix.
        double cellsize = (ncols > 0) ? (NEX - SWX) / static_cast<double>(ncols) : 0.0;

        // Write the ASCII header.
        out << "ncols         " << ncols << "\n";
        out << "nrows         " << nrows << "\n";
        out << "xllcorner     " << SWX << "\n";
        out << "yllcorner     " << SWY << "\n";
        out << "cellsize      " << cellsize << "\n";
        out << "NODATA_value  -9999\n";

        // Write the matrix data row by row.
        for (size_t i = 0; i < nrows; i++)
        {
            for (size_t j = 0; j < ncols; j++)
            {
                out << matrix[i][j];
                if (j < ncols - 1)
                    out << " ";
            }
            out << "\n";
        }

        out.close();
    }

}
