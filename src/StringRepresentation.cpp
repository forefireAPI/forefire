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

#include "StringRepresentation.h"
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#define GEOJSON_MODE  2
#define JSON_MODE     1
#define FF_MODE       0
#define KML_MODE      3

using namespace std;

namespace libforefire {

// static variables initialization
size_t StringRepresentation::currentLevel = 0;
ostringstream StringRepresentation::outputstr("");
bool writedOnce = false;
string outPattern;
FireDomain* domain = 0;

// --- GEOJSON Aggregation Variables ---
// For GEOJSON mode we aggregate rings per top-level front.
// Each top-level feature (currentLevel==1) will be represented as a vector of rings,
// where each ring is a vector of coordinate strings.
static bool firstGeoFeature = true;  // used when outputting the feature list
static std::vector< std::vector<std::string> > geojson_current_feature;

StringRepresentation::StringRepresentation(FireDomain* fdom) : Visitor() {
    lastLevel = -1;
    domain = fdom;
    
    outPattern = SimulationParameters::GetInstance()->getParameter("ffOutputsPattern");
    setTime(domain->getTime());
    updateStep = SimulationParameters::GetInstance()->getDouble("outputsUpdate");
    setUpdateTime(domain->getTime());
    dumpMode = FF_MODE;  // default; will be set by dumpStringRepresentation()
}

StringRepresentation::~StringRepresentation() { }

void StringRepresentation::input() { }

void StringRepresentation::setOutPattern(string s) {
    outPattern = s;
}

void StringRepresentation::update() {
    setTime(getUpdateTime());
}

void StringRepresentation::timeAdvance() {
    if ( updateStep < EPSILONT ) {
        setUpdateTime(numeric_limits<double>::infinity());
    } else {
        setUpdateTime(getTime() + updateStep);
    }
}

void StringRepresentation::output() {
    if (writedOnce && (domain->getNumFF() == 0))
        return;
    writedOnce = true;

    ostringstream oss;
    oss << outPattern << "." << getTime();
    ofstream outputfile(oss.str().c_str());
    if (outputfile) {
        outputfile << dumpStringRepresentation();
    } else {
        cout << "could not open file " << oss.str() << " for writing domain file " << endl;
    }
}

//
// Visitor methods
//

// Visit the domain: outputs the header and resets GEOJSON flags.
void StringRepresentation::visit(FireDomain* fd) {
    if (dumpMode == JSON_MODE) {
        outputstr << '{' << endl << "\t\"fronts\": [";
        lastLevel = 0;
    }
    else if (dumpMode == GEOJSON_MODE) {
        firstGeoFeature = true;  // reset before outputting features
        outputstr << "{" << endl;
        outputstr << "\t\"type\": \"FeatureCollection\"," << endl;
        outputstr << "\t\"features\": [" << endl;
    }
    else if (dumpMode == KML_MODE) {
        outputstr << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
        outputstr << "<kml xmlns=\"http://www.opengis.net/kml/2.2\" "
                  << "xmlns:gx=\"http://www.google.com/kml/ext/2.2\" "
                  << "xmlns:kml=\"http://www.opengis.net/kml/2.2\" "
                  << "xmlns:atom=\"http://www.w3.org/2005/Atom\">" << endl;
        outputstr << "<Document>" << endl;
    }
    else if (dumpMode == FF_MODE) {
        outputstr << fd->toString() << endl;
    }
}

// Visit a fire front.
void StringRepresentation::visit(FireFront* ff) {
    if (dumpMode == FF_MODE) {
        for (size_t k = 0; k < currentLevel; k++)
            outputstr << "    ";
        outputstr << ff->toString() << endl;
        return;
    }
    else if (dumpMode == JSON_MODE) {
        // (JSON mode unchanged)
        SimulationParameters *simParam = SimulationParameters::GetInstance();
        if (ff->getDomain()->getSimulationTime() >= ff->getTime()) {
            if (lastLevel >= 2)
                outputstr << '"';
            if (lastLevel >= 1)
                outputstr << endl << "\t" << "},";
            double t = simParam->getInt("refTime") + ff->getDomain()->getSimulationTime();
            int d = simParam->getInt("refDay");
            int y = simParam->getInt("refYear");

            outputstr.precision(3);
            outputstr << endl << "\t{" << endl;
            outputstr << "\t\t\"area\": \"" << fixed << (ff->getArea() / 10000.0) << "ha\"," << endl;
            outputstr << "\t\t\"date\": \"" << SimulationParameters::FormatISODate(t, y, d) << "\"," << endl;
            outputstr << "\t\t\"projection\": \"" 
                      << SimulationParameters::GetInstance()->getParameter("projection") << "\"," << endl;
            outputstr << "\t\t\"coordinates\": \"";
            lastLevel = 1;
        }
    }
    else if (dumpMode == GEOJSON_MODE) {
        // In GEOJSON mode we aggregate rings from nodes.
        // For a top–level fire front (currentLevel==1), clear any previous data and start a new feature.
        if (currentLevel == 1) {
            geojson_current_feature.clear();
            geojson_current_feature.push_back(std::vector<std::string>());  // main (outer) ring
        }
        else if (currentLevel == 2) {
            // For an inner boundary (hole), add a new ring.
            geojson_current_feature.push_back(std::vector<std::string>());
        }
        // (No immediate output; nodes will be collected.)
    }
    else if (dumpMode == KML_MODE) {
        // KML mode unchanged from your adjustments.
        if (currentLevel == 1) {
            outputstr << "<Placemark>" << endl;
            outputstr << "<Style>" << endl;
            outputstr << " <LineStyle>" << endl;
            outputstr << "  <color>ff0000ff</color>" << endl;
            outputstr << "  <width>2</width>" << endl;
            outputstr << "</LineStyle>" << endl;
            outputstr << "<PolyStyle>" << endl;
            outputstr << "  <color>22000000</color>" << endl;
            outputstr << "</PolyStyle>" << endl;
            outputstr << "</Style>" << endl;
        }
        if (currentLevel % 2 == 1) { // For top-level front (outer boundary)
            outputstr << "<Polygon>" << endl;
            outputstr << "  <outerBoundaryIs>" << endl;
            outputstr << "    <LinearRing>" << endl;
            outputstr << "      <coordinates>" << endl;
        } else { // For inner boundaries
            outputstr << "<innerBoundaryIs>" << endl;
            outputstr << "  <LinearRing>" << endl;
            outputstr << "      <coordinates>" << endl;
        }
    }
}

// Visit a fire node. In GEOJSON mode, add its coordinate to the current ring.
void StringRepresentation::visit(FireNode* fn) {
    if (dumpMode == JSON_MODE) {
        if (fn->getFront()->getDomain()->getSimulationTime() >= fn->getFront()->getTime()) {
            if (lastLevel == 2)
                outputstr << ' ';
            outputstr.precision(3);
            outputstr << fixed << fn->getX() << ',' << fn->getY() << ',' << fn->getSpeed();
            lastLevel = 2;
        }
        return;
    }
    
    if (dumpMode == FF_MODE) {
        for (size_t k = 0; k < currentLevel; k++)
            outputstr << "    ";
        outputstr << fn->toString() << endl;
        return;
    }
    
    if (dumpMode == GEOJSON_MODE) {
        if (fn->getFront()->getDomain()->getSimulationTime() >= fn->getFront()->getTime()) {
            ostringstream coord;
          //  std::cout<<domain->getRefLongitude()<<"   "<< domain->getMetersPerDegreesLon()<<std::endl;
            coord << "[" 
                  << fixed << setprecision(5)
                  << fn->getLoc().projectLon(domain->getRefLongitude(), domain->getMetersPerDegreesLon()) << ", "
                  << fn->getLoc().projectLat(domain->getRefLatitude(), domain->getMetersPerDegreeLat())
                  << ", 0]";
            if (!geojson_current_feature.empty()) {
                // Append coordinate to the current (last) ring.
                geojson_current_feature.back().push_back(coord.str());
            }
        }
        return;
    }
    
    if (dumpMode == KML_MODE) {
        if (fn->getFront()->getDomain()->getSimulationTime() >= fn->getFront()->getTime()) {
            outputstr.precision(5);
            outputstr << fixed 
                      << fn->getLoc().projectLon(domain->getRefLongitude(), domain->getMetersPerDegreesLon()) << ","
                      << fn->getLoc().projectLat(domain->getRefLatitude(), domain->getMetersPerDegreeLat())
                      << ",0 ";
        }
        return;
    }
}

// postVisitInner: called after a FireFront's nodes but before processing its internals.
// For GEOJSON mode, reverse the current ring to mimic the Python [::-1] behavior.
void StringRepresentation::postVisitInner(FireFront* ff) {
    if (dumpMode == JSON_MODE) {
       // outputstr << "\"" << endl << "\t}";
    }
    else if (dumpMode == GEOJSON_MODE) {
        if (!geojson_current_feature.empty() && !geojson_current_feature.back().empty()) {
            std::reverse(geojson_current_feature.back().begin(), geojson_current_feature.back().end());
        }
    }
    else if (dumpMode == KML_MODE) {
        if (currentLevel % 2 == 1) { // outer boundary
            outputstr << endl << "      </coordinates>" << endl;
            outputstr << "    </LinearRing>" << endl;
            outputstr << "  </outerBoundaryIs>" << endl;
        } else { // inner boundary
            outputstr << endl << "      </coordinates>" << endl;
            outputstr << "    </LinearRing>" << endl;
            outputstr << "  </innerBoundaryIs>" << endl;
        }
    }
}

// postVisitAll: called at the very end after processing internals.
// For GEOJSON mode, if finishing a top–level FireFront (currentLevel==1), build and output its Feature.
void StringRepresentation::postVisitAll(FireFront* ff) {
    if (dumpMode == JSON_MODE) {
        // (JSON mode unchanged)
    }
    else if (dumpMode == GEOJSON_MODE) {
        if (currentLevel == 1) {
            // Build the GeoJSON Feature from geojson_current_feature.
            ostringstream feature;
            feature << "\t{" << "\n";
            feature << "\t\t\"type\": \"Feature\",\n";
            feature << "\t\t\"properties\": { \"numberOfPolygons\": " << geojson_current_feature.size() << " },\n";
            feature << "\t\t\"geometry\": {\n";
            feature << "\t\t\t\"type\": \"MultiPolygon\",\n";
            feature << "\t\t\t\"coordinates\": [ [ ";
            bool firstRing = true;
            for (const auto &ring : geojson_current_feature) {
                if (!firstRing)
                    feature << ", ";
                firstRing = false;
                feature << "[";
                bool firstCoord = true;
                std::string firstCoordinate;
                for (const auto &coord : ring) {
                    if (firstCoord) {
                        firstCoordinate = coord; // Store the very first coordinate.
                        firstCoord = false;
                    } else {
                        feature << ", "; // Add comma *before* subsequent coordinates
                    }
    
                    feature << coord;
                }
                // Close the ring by adding the first coordinate again.
                if (!firstCoordinate.empty()) { // Make sure the ring wasn't empty.
                    feature << ", " << firstCoordinate;
                    }
                feature << "]";
            }
            feature << " ] ]\n";
            feature << "\t\t}\n";
            feature << "\t}";
            if (!firstGeoFeature) {
                outputstr << ",\n" << feature.str();
            } else {
                firstGeoFeature = false;
                outputstr << feature.str();
            }
        }
    }
    else if (dumpMode == KML_MODE) {
        if (currentLevel == 1) { // finishing top-level front
            outputstr << endl << "       </Polygon>" << endl;
            outputstr << "    </Placemark>" << endl;
        }
    }
}

void StringRepresentation::postVisitInner(FireDomain* fd) { }
void StringRepresentation::postVisitAll(FireDomain* fd) { }

string StringRepresentation::dumpStringRepresentation() {
    // Set dump mode based on simulation parameters.
    string mode = SimulationParameters::GetInstance()->getParameter("dumpMode");
    if (mode == "json")
        dumpMode = JSON_MODE;
    else if (mode == "ff")
        dumpMode = FF_MODE;
    else if (mode == "geojson")
        dumpMode = GEOJSON_MODE;
    else if (mode == "kml")
        dumpMode = KML_MODE;
    
    currentLevel = 0;
    lastLevel = -1;
    
    outputstr.str("");
    
    if (domain != 0)
        domain->accept(this);
    
    // Append closing parts for each mode.
    if (dumpMode == JSON_MODE) {
        if (lastLevel >= 2)
            outputstr << '"';
        if (lastLevel >= 1)
            outputstr << endl << "\t}" << endl;
        if (lastLevel >= 0)
            outputstr << "\t]" << endl << "}" << endl;
    }
    else if (dumpMode == GEOJSON_MODE) {
        outputstr << "\n\t]" << endl;
        outputstr << "}" << endl;
    }
    else if (dumpMode == KML_MODE) {
        outputstr << "</Document>" << endl;
        outputstr << "</kml>" << endl;
    }
    
    return outputstr.str();
}

void StringRepresentation::increaseLevel() {
    currentLevel++;
}

void StringRepresentation::decreaseLevel() {
    currentLevel--;
}

size_t StringRepresentation::getLevel() {
    return currentLevel;
}

string StringRepresentation::toString() {
    ostringstream oss;
    oss << "string representation";
    return oss.str();
}

} // namespace libforefire
