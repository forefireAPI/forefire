/**
 * @file DataBroker_nc.cpp
 * @brief NetCDF landscape loading for DataBroker. Compiled only when FOREFIRE_ENABLE_NETCDF is ON; netcdf_stubs.cpp stands in otherwise.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "DataBroker.h"
#include "MultiplicativeLayer.h"
#include "FireDomain.h"

#include <netcdf>

using namespace netCDF;
using namespace netCDF::exceptions;

namespace libforefire
{

	void DataBroker::loadFromNCFile(std::string filename)
	{
		if(filename.empty())
		{
			return;
		}
		try
		{
			NcFile dataFile(filename.c_str(), NcFile::read);
			if (dataFile.isNull())
			{
				cout << "Error: Unable to open file " << filename << endl;
				return;
			}

			NcVar domvar = dataFile.getVar("domain");
			if (domvar.isNull())
			{
				cout << "Error: Domain variable not found in file " << filename << endl;
				return;
			}

			// Helper lambda to check if an attribute exists
			auto hasAttribute = [&](const NcVar &var, const std::string &attName) -> bool
			{
				std::map<std::string, NcVarAtt> attMap = var.getAtts();
				return (attMap.find(attName) != attMap.end());
			};

			// Read version attribute
			double version = 1;
			{
				auto attributes = domvar.getAtts();
				auto it = attributes.find("version");
				if (it != attributes.end() && !it->second.isNull())
				{
					it->second.getValues(&version);
				}
			}

			// Read simulation origin coordinates
			double Xorigin = 0, Yorigin = 0, Zorigin = 0;
			double Lx = 0, Ly = 0, Lz = 0;
			{
				auto attributes = domvar.getAtts();
				auto getAttributeValue = [&](const std::string &attName, double &value)
				{
					auto it = attributes.find(attName);
					if (it != attributes.end() && !it->second.isNull())
					{
						it->second.getValues(&value);
					}
				};
				getAttributeValue("SWx", Xorigin);
				getAttributeValue("SWy", Yorigin);
				getAttributeValue("SWz", Zorigin);
				getAttributeValue("Lx", Lx);
				getAttributeValue("Ly", Ly);
				getAttributeValue("Lz", Lz);
			}

			// Process BBoxWSEN attribute if available

			if (hasAttribute(domvar, "BBoxWSEN"))
			{
				NcVarAtt bboxAtt = domvar.getAtt("BBoxWSEN");
				size_t len = bboxAtt.getAttLength();
				if (len == 4)
				{
					std::vector<double> bbox(4);
					bboxAtt.getValues(&bbox[0]);

					std::ostringstream oss;
					oss << bbox[0] << "," << bbox[1] << "," << bbox[2] << "," << bbox[3];
				//	cout << "WARNING got BBoxWSEN as a double array in NC data, should be a string parameter, not using it: " << oss.str() << endl;
					// params->setParameter("dataTileBBoxWSEN", oss.str());
				}
				else
				{
					std::vector<char> buffer(len + 1, '\0'); // extra space for null terminator
					bboxAtt.getValues(&buffer[0]);
					std::string bboxwsen(buffer.data());
					params->setParameter("dataTileBBoxWSEN", bboxwsen);
				}

				vector<double> bbox = params->getDoubleArray("dataTileBBoxWSEN");
				if (bbox.size() >= 4 && (bbox[3] - bbox[1]) != 0 && (bbox[2] - bbox[0]) != 0)
				{
					double metersPerDegreeLat = Ly / (bbox[3] - bbox[1]);
					double metersPerDegreeLon = Lx / (bbox[2] - bbox[0]);
					params->setDouble("metersPerDegreeLat", metersPerDegreeLat);
					params->setDouble("metersPerDegreesLon", metersPerDegreeLon);

					double simSWLat = 9999, simSXLon = 9999;
					std::string runmode = params->getParameter("runmode");
					if ((runmode == "coupled" || runmode == "masterMNH") && params->isValued("SWLngLat"))
					{
						vector<double> swLngLat = params->getDoubleArray("SWLngLat");
						if (swLngLat.size() >= 2)
						{
							simSXLon = swLngLat[0];
							simSWLat = swLngLat[1];
						}
					}
					if (simSWLat < 9999)
					{
						double dx = (bbox[0] - simSXLon) * metersPerDegreeLon;
						double dy = (bbox[1] - simSWLat) * metersPerDegreeLat;
						cout << "WARNING: Shifting origin. Simulation domain SWCorner ("
							 << domain->getSWCorner().getX() << ", " << domain->getSWCorner().getY()
							 << ") corresponds to (" << simSXLon << ", " << simSWLat
							 << ") while data gives offset (" << dx << ", " << dy << ")" << endl;
						Xorigin += dx;
						Yorigin += dy;
					}
				}
			}

			FFPoint spatialExtent(Lx, Ly, Lz);
			FFPoint SWCorner(Xorigin, Yorigin, Zorigin);

			// Read time attributes from "t0" (and optionally "Lt")
			double timeOrigin = 0;
			{
				auto attributes = domvar.getAtts();
				auto it = attributes.find("t0");
				if (it != attributes.end() && !it->second.isNull())
				{
					it->second.getValues(&timeOrigin);
				}
			}
			double Lt = 0;
			{
				auto attributes = domvar.getAtts();
				auto it = attributes.find("Lt");
				if (it != attributes.end() && !it->second.isNull())
				{
					it->second.getValues(&Lt);
				}
				else
				{
					Lt = timeOrigin;
				}
			}

			std::multimap<std::string, NcVar> allVariables = dataFile.getVars();

			// Loop through all variables in the file
			for (auto it = allVariables.begin(); it != allVariables.end(); ++it)
			{
				string varName = it->first;
				NcVar currentVar = it->second;
				string layerType;

				// Retrieve the "type" attribute if available
				{
					auto attributes = currentVar.getAtts();
					auto attIt = attributes.find("type");
					if (attIt != attributes.end() && !attIt->second.isNull())
					{
						attIt->second.getValues(layerType);
					}
				}
				if (layerType.empty())
				{
					cout << "Skipping variable " << varName << " because 'type' attribute is missing." << endl;
					continue;
				}
				if (varName == "wind")
				{
					if (!domain->atmosphericCoupling)
					{
						XYZTDataLayer<double> *wul = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, 0);
						registerLayer("windU", wul);
						XYZTDataLayer<double> *wvl = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, 1);
						registerLayer("windV", wvl);
						PwindULayer = wul;
						PwindVLayer = wvl;
					}
				}
				// Process known property layers
				auto pg = propPropertiesGetters.find(varName);
				if (pg != propPropertiesGetters.end())
				{

					if (varName == "altitude" && domain->getDomainID() == 0)
					{
				
						auto altLayer = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, altLayer);
					}
					else if (varName == "windU")
					{
						auto windULayerLocal = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						windULayer = windULayerLocal;
						registerLayer(varName, windULayerLocal);
					}
					else if (varName == "windV")
					{
						auto windVLayerLocal = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, windVLayerLocal);
					}
					else if (varName == "temperature")
					{
						auto tempLayer = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, tempLayer);
					}
					else if (varName == "moisture")
					{
						auto moistLayer = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, moistLayer);
					}
					else if (varName == "fuel")
					{
						fuelLayer = constructFuelLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer("fuel", fuelLayer);
					}
					else if (varName == "fieldSpeed")
					{
						dummyLayer = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, dummyLayer);
					}
				}
				// Process additional layer types
				else if (layerType.find("data") != std::string::npos)
				{
					if (std::find(neededProperties.begin(), neededProperties.end(), varName) != neededProperties.end())
					{
						auto newLayer = constructXYZTLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerLayer(varName, newLayer);
					}
				}
				else if (layerType.find("flux") != std::string::npos)
				{
					auto newLayer = constructFluxLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
					registerFluxLayer(varName, newLayer);
				}
				else if (layerType.find("propagative") != std::string::npos)
				{
					auto propLayer = constructPropagativeLayer(currentVar, SWCorner, spatialExtent, timeOrigin, Lt, -1);
					domain->setPropagativeLayer(propLayer);
				}
				else if (layerType.find("parameter") != std::string::npos)
				{
					// Set parameters from variable attributes
					auto paramAttributes = currentVar.getAtts();
					for (auto iter = paramAttributes.begin(); iter != paramAttributes.end(); ++iter)
					{
						NcVarAtt att = iter->second;
						NcType attValType = att.getType();
						string attsVal;
						int attiVal;
						float attfVal;
						double attdVal;
						switch ((int)attValType.getTypeClass())
						{
						case NC_CHAR:
							att.getValues(attsVal);
							params->setParameter(iter->first, attsVal);
							break;
						case NC_INT:
							att.getValues(&attiVal);
							params->setParameter(iter->first, std::to_string(attiVal));
							break;
						case NC_FLOAT:
							att.getValues(&attfVal);
							params->setParameter(iter->first, std::to_string(attfVal));
							break;
						case NC_DOUBLE:
							att.getValues(&attdVal);
							params->setParameter(iter->first, std::to_string(attdVal));
							break;
						default:
							cout << iter->first << " attribute of unhandled type "
								 << attValType.getName() << endl;
							break;
						}
					}
				}
			} // End of variable loop

			// Ensure propagative layer is present
			if (domain->getPropagativeLayer() == 0)
			{
				std::string propagationModel = params->getParameter("propagationModel");
				if (!domain->addPropagativeLayer(propagationModel))
				{
					cout << "PROBLEM: Unable to retrieve propagation model " << propagationModel << endl;
				}
			}
			if (domain->getPropagativeLayer() != 0)
			{
				registerLayer(domain->getPropagativeLayer()->getKey(), domain->getPropagativeLayer());
			}
		}
		catch (NcException &e)
		{
			cout << "Problem Loading data from NetCDF file " << filename << ": " << e.what() << endl;
		}
	}

	XYZTDataLayer<double> *DataBroker::constructXYZTLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
														  double timeOrigin, double Lt, int dimTSelected /* = -1 */)
	{
		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount())
		{
		case 2:
			ny = values.getDim(0).getSize(), nx = values.getDim(1).getSize();
			break;
		case 3:
			nz = values.getDim(0).getSize(), ny = values.getDim(1).getSize(), nx = values.getDim(2).getSize();
			break;
		case 4:
			nt = values.getDim(0).getSize(), nz = values.getDim(1).getSize(), ny = values.getDim(2).getSize(), nx = values.getDim(3).getSize();
			break;
		}
		if (dimTSelected > -1)
			nt = 1;

		double *data = readAndTransposeFortranProjectedField(&values, nt, nz, ny, nx, true, dimTSelected);

		std::string property = values.getName();

		if (isRelevantData(SWCorner, spatialExtent))
		{
			XYZTDataLayer<double> *newlayer = new XYZTDataLayer<double>(property, SWCorner, timeOrigin, spatialExtent, Lt,
																		nx, ny, nz, nt, data);
			delete[] data;
			return newlayer;
		}
		else
		{
			cout << "WARNING, NC variable " << property << " is not in domain, Failsafe Reprojection to whole domain extent" << endl;
			FFPoint SWC = FFPoint(domain->SWCornerX(), domain->SWCornerY(), 0.);
			FFPoint ext = FFPoint(domain->NECornerX() - domain->SWCornerX(),
								  domain->NECornerY() - domain->SWCornerY(), 10000.);
			double t0 = 0;
			double Dt = 10000000.;
			XYZTDataLayer<double> *newlayer = new XYZTDataLayer<double>(property, SWC, t0, ext, Dt,
																		nx, ny, nz, nt, data);
			delete[] data;
			return newlayer;
		}
	}

	PropagativeLayer<double> *DataBroker::constructPropagativeLayer(NcVar &/*values*/, FFPoint &/*SWCorner*/, FFPoint &/*spatialExtent*/,
																	double /*timeOrigin*/, double /*Lt*/, int /*dimTSelected*/ /* = -1 */)
	{
		cout << "DataBroker::constructPropagativeLayerFromFile " << " from netCDF Not Implemented" << endl;
		return NULL;
	}

	FuelDataLayer<double> *DataBroker::constructFuelLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
														  double timeOrigin, double Lt, int /*dimTSelected*/)
	{

		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount())
		{
		case 2:
			ny = values.getDim(0).getSize(), nx = values.getDim(1).getSize();
			break;
		case 3:
			nz = values.getDim(0).getSize(), ny = values.getDim(1).getSize(), nx = values.getDim(2).getSize();
			break;
		case 4:
			nt = values.getDim(0).getSize(), nz = values.getDim(1).getSize(), ny = values.getDim(2).getSize(), nx = values.getDim(3).getSize();
			break;
		}
		std::string property = values.getName();

		int *fuelMap = readAndTransposeIntFortranProjectedField(&values, nt, nz, ny, nx, true, -1);

		if (isRelevantData(SWCorner, spatialExtent))
		{
			FuelDataLayer<double> *newlayer = new FuelDataLayer<double>(property, SWCorner, timeOrigin, spatialExtent, Lt, nx, ny, nz, nt, fuelMap);
			delete[] fuelMap;
			return newlayer;
		}
		else
		{
			cout << "WARNING, NC variable " << property << " is not in domain, Failsafe Reprojection to whole domain extent" << endl;
			FFPoint SWC = FFPoint(domain->SWCornerX(), domain->SWCornerY(), 0.);
			FFPoint ext = FFPoint(domain->NECornerX() - domain->SWCornerX(),
								  domain->NECornerY() - domain->SWCornerY(), 10000.);
			double t0 = 0;
			double Dt = 10000000.;
			FuelDataLayer<double> *newlayer = new FuelDataLayer<double>(property, SWC, t0, ext, Dt, nx, ny, nz, nt, fuelMap);
			delete[] fuelMap;
			return newlayer;
		}
	}

	FluxLayer<double> *DataBroker::constructFluxLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
													  double timeOrigin, double Lt, int /*dimTSelected*/ /* = -1 */)
	{

		/* Sending the information on the models to the domain */
		/*-----------------------------------------------------*/
		std::string property = values.getName();

		NcVarAtt indices = values.getAtt("indices");
		int indModel = 0;
		if (!indices.isNull())
			indices.getValues(&indModel);
		else
			cout << "DataBroker::constructFluxLayer can't get number of indices of model property" << endl;
		/*
		for (int i = indModel; i < indModel + 5; i++) {
			stringstream mname;
			mname << "model" << i << "name";

			NcVarAtt tmpName = values.getAtt(mname.str());
			if (!tmpName.isNull()) {
				string modelName;
				tmpName.getValues(modelName);
				domain->fluxModelInstanciation(i, modelName);
				cout << mname.str() << " DataBroker::Instanciated " <<modelName<< endl;
			}
		}*/

		stringstream mname;
		string modelName;
		mname << "model" << indModel << "name";
		NcVarAtt tmpName = values.getAtt(mname.str()); // mname.str());
		if (!tmpName.isNull())
			tmpName.getValues(modelName);
		domain->fluxModelInstanciation(indModel, modelName);

		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount())
		{
		case 2:
			ny = values.getDim(0).getSize(), nx = values.getDim(1).getSize();
			break;
		case 3:
			nz = values.getDim(0).getSize(), ny = values.getDim(1).getSize(), nx = values.getDim(2).getSize();
			break;
		case 4:
			nt = values.getDim(0).getSize(), nz = values.getDim(1).getSize(), ny = values.getDim(2).getSize(), nx = values.getDim(3).getSize();
			break;
		}

		/* Getting the data */
		/*------------------*/
		int *data = readAndTransposeIntFortranProjectedField(&values, nt, nz, ny, nx, true, -1);

		if (isRelevantData(SWCorner, spatialExtent))
		{

			/* Instanciating the data layer */
			/*------------------------------*/

			FluxLayer<double> *newlayer = new FluxLayer<double>(property,
																atmoSWCorner, atmoNECorner, atmosphericNx, atmosphericNy,
																domain->getCells(), data, SWCorner, timeOrigin, spatialExtent,
																Lt, nx, ny, nz, nt);
			delete[] data;
			return newlayer;
		}
		else
		{
			cout << "WARNING, NC variable " << property << " is not in domain, Failsafe Reprojection to whole domain extent" << endl;
			FFPoint SWC = FFPoint(domain->SWCornerX(), domain->SWCornerY(), 0.);
			FFPoint ext = FFPoint(domain->NECornerX() - domain->SWCornerX(),
								  domain->NECornerY() - domain->SWCornerY(), 10000.);
			double t0 = 0;
			double Dt = 10000000.;
			FluxLayer<double> *newlayer = new FluxLayer<double>(property,
																atmoSWCorner, atmoNECorner, atmosphericNx, atmosphericNy,
																domain->getCells(), data, SWC, t0, ext, Dt, nx, ny, nz, nt);
			delete[] data;
			return newlayer;
		}
	}

	int *DataBroker::readAndTransposeIntFortranProjectedField(NcVar *val, const size_t &nt, const size_t &nz, const size_t &ny, const size_t &nx, bool transpose, int selectedT)
	{
		size_t nnt = nt;
		size_t SelectedTdim = (selectedT < 0 ? 0 : selectedT);
		if (selectedT > -1)
			nnt = 1;
		vector<size_t> mySlab{nnt, nz, ny, nx};
		vector<size_t> myStart{SelectedTdim, 0, 0, 0};
		int *tmp = new int[nnt * nz * ny * nx];
		val->getVar(myStart, mySlab, tmp);
		if (!transpose)
		{
			std::cout << "not transposing field" << std::endl;
			return tmp;
		}
		int *data = new int[nnt * nz * ny * nx];
		size_t sizeV = nnt * nz * ny * nx;
		size_t indC, indF;
		size_t ii, jj, kk, ll, rest;
		if (nnt > 1)
		{
			size_t restz;
			for (indF = 0; indF < sizeV; indF++)
			{
				ll = indF / (nx * ny * nz);
				restz = indF - (ll * nx * ny * nz);
				kk = restz / (nx * ny);
				rest = restz - kk * nx * ny;
				jj = rest / nx;
				ii = rest % nx;
				indC = ii * ny * nz * nnt + jj * nz * nnt + kk * nnt + ll;
				data[indC] = tmp[indF];
			}
		}
		else
		{
			for (indF = 0; indF < sizeV; indF++)
			{
				kk = indF / (nx * ny);
				rest = indF - kk * nx * ny;
				jj = rest / nx;
				ii = rest % nx;
				indC = ii * ny * nz + jj * nz + kk;
				data[indC] = tmp[indF];
			}
		}
		delete[] tmp;
		return data;
	}

	double *DataBroker::readAndTransposeFortranProjectedField(NcVar *val, const size_t &nt, const size_t &nz, const size_t &ny, const size_t &nx, bool transpose, int selectedT)
	{
		size_t nnt = nt;
		size_t SelectedTdim = (selectedT < 0 ? 0 : selectedT);
		if (selectedT > -1)
			nnt = 1;
		vector<size_t> mySlab{nnt, nz, ny, nx};
		vector<size_t> myStart{SelectedTdim, 0, 0, 0};
		double *tmp = new double[nnt * nz * ny * nx];
		val->getVar(myStart, mySlab, tmp);
		if (!transpose)
		{
			std::cout << "not transposing field" << std::endl;
			return tmp;
		}
		double *data = new double[nnt * nz * ny * nx];
		size_t sizeV = nnt * nz * ny * nx;
		size_t indC, indF;
		size_t ii, jj, kk, ll, rest;
		if (nnt > 1)
		{
			size_t restz;
			for (indF = 0; indF < sizeV; indF++)
			{
				ll = indF / (nx * ny * nz);
				restz = indF - (ll * nx * ny * nz);
				kk = restz / (nx * ny);
				rest = restz - kk * nx * ny;
				jj = rest / nx;
				ii = rest % nx;
				indC = ii * ny * nz * nnt + jj * nz * nnt + kk * nnt + ll;
				data[indC] = tmp[indF];
			}
		}
		else
		{
			for (indF = 0; indF < sizeV; indF++)
			{
				kk = indF / (nx * ny);
				rest = indF - kk * nx * ny;
				jj = rest / nx;
				ii = rest % nx;
				indC = ii * ny * nz + jj * nz + kk;
				data[indC] = tmp[indF];
			}
		}
		delete[] tmp;
		return data;
	}

}
