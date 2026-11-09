/**
 * @file Command_nc.cpp
 * @brief NetCDF-backed commands and helpers. Compiled only when FOREFIRE_ENABLE_NETCDF is ON; netcdf_stubs.cpp stands in otherwise.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "Command.h"
#include <map>
#include <sstream>

#include <netcdf>

using namespace netCDF;
using namespace netCDF::exceptions;

namespace libforefire
{

    int Command::saveData(const std::string &arg, size_t &/*numTabs*/)
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

            std::string bboxwsen = simParam->getParameter("BBoxWSEN");
            if (!bboxwsen.empty())
            {
                domVar.putAtt("BBoxWSEN", bboxwsen.c_str());
            }

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

    void Command::writeNetCDF(const char *filename, const string &varName, const std::vector<std::vector<double>> &matrix, const vector<double> &latitudes, const vector<double> &longitudes)
    {
    try
    {
        // Create (or replace) the NetCDF file using NetCDF-4 mode.
        NcFile dataFile(filename, NcFile::replace);

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

    bool Command::readPgdGrid(const string &pgdNcFile, std::vector<double> &xhatValues, std::vector<double> &yhatValues)
    {
        try
        {
            NcFile dataFile(pgdNcFile.c_str(), NcFile::read);
            if (!dataFile.isNull())
            {
                NcVar XHAT = dataFile.getVar("XHAT");
                NcVar YHAT = dataFile.getVar("YHAT");

                size_t nXElements = XHAT.getDim(0).getSize();
                xhatValues.resize(nXElements);
                size_t nYElements = YHAT.getDim(0).getSize();
                yhatValues.resize(nYElements);

                YHAT.getVar(&yhatValues[0]);
                XHAT.getVar(&xhatValues[0]);
                return true;
            }
        }
        catch (NcException &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return false;
    }

    bool Command::loadDomainHeader(const string &name, const string &path)
    {
        SimulationParameters *simParam = SimulationParameters::GetInstance();
        simParam->setParameter("NetCDFfile", name);
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
                    simParam->setParameter("atmoNX", std::to_string(ncNX));
                    simParam->setParameter("atmoNY", std::to_string(ncNY));
                }
            }
        }
        catch (NcException &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return true;
    }

}
