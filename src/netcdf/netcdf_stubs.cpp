/**
 * @file netcdf_stubs.cpp
 * @brief Stand-ins for every NetCDF-backed entry point, compiled instead of the *_nc.cpp files when FOREFIRE_ENABLE_NETCDF is OFF (Emscripten, slim builds). Each one reports the missing support and keeps the command table unchanged.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "Command.h"
#include "DataBroker.h"
#include "FireDomain.h"

namespace libforefire
{

    void DataBroker::loadFromNCFile(std::string filename)
    {
        // The empty-filename early return matches the NetCDF path: the command
        // layer calls this with no file on every session setup.
        if (filename.empty()) return;
        cout << "loadFromNCFile: this build has no NetCDF support, ignoring "
             << filename << endl;
    }

    void FireDomain::loadArrivalTimeNC(string fname)
    {
        cout << "loadArrivalTimeNC: this build has no NetCDF support, ignoring "
             << fname << endl;
    }

    void FireDomain::saveArrivalTimeNC()
    {
        cout << "saveArrivalTimeNC: this build has no NetCDF support, "
                "nothing written" << endl;
    }

    // saveData writes a landscape file, which is NetCDF end to end; there is no
    // partial behaviour to keep when NetCDF is compiled out.
    int Command::saveData(const std::string &/*arg*/, size_t &/*numTabs*/)
    {
        std::cout << "saveData: this build has no NetCDF support" << std::endl;
        return error;
    }

    void Command::writeNetCDF(const char *filename, const string &/*varName*/, const std::vector<std::vector<double>> &/*matrix*/, const vector<double> &/*latitudes*/, const vector<double> &/*longitudes*/)
    {
        std::cerr << "writeNetCDF: this build has no NetCDF support, "
                  << filename << " not written" << std::endl;
    }

    bool Command::readPgdGrid(const string &pgdNcFile, std::vector<double> &/*xhat*/, std::vector<double> &/*yhat*/)
    {
        cout << "createDomain: reading the domain from " << pgdNcFile
             << " needs NetCDF, which this build has no support for" << endl;
        return false;
    }

    bool Command::loadDomainHeader(const string &/*name*/, const string &path)
    {
        cout << "loadData: reading a domain from " << path
             << " needs NetCDF, which this build has no support for" << endl;
        return false;
    }

}
