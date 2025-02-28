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

#include "DataBroker.h"
#include "MultiplicativeLayer.h"
#include "FireDomain.h"

namespace libforefire
{

	const DataBroker::propGetterMap DataBroker::propPropertiesGetters =
		DataBroker::makePGmap();
	const DataBroker::fluxGetterMap DataBroker::fluxPropertiesGetters =
		DataBroker::makeFGmap();

	DataLayer<double> *DataBroker::fuelLayer = 0;
	DataLayer<double> *DataBroker::moistureLayer = 0;
	DataLayer<double> *DataBroker::temperatureLayer = 0;
	DataLayer<double> *DataBroker::dummyLayer = 0;
	DataLayer<double> *DataBroker::altitudeLayer = 0;
	DataLayer<double> *DataBroker::forcedArrivalTimeLayer = 0;
	DataLayer<double> *DataBroker::slopeLayer = 0;
	DataLayer<double> *DataBroker::windULayer = 0;
	DataLayer<double> *DataBroker::windVLayer = 0;

	XYZTDataLayer<double> *DataBroker::PwindULayer = 0;
	XYZTDataLayer<double> *DataBroker::PwindVLayer = 0;
	FluxLayer<double> *DataBroker::heatFluxLayer = 0;

	SimulationParameters *DataBroker::params = SimulationParameters::GetInstance();
	double frontScanDistance = 1000;

	DataBroker::DataBroker(FireDomain *fd) : domain(fd)
	{
		commonInitialization();
	}

	DataBroker::~DataBroker()
	{
		// Deleting layers
		while (!layers.empty())
		{
			delete layers.back();
			layers.pop_back();
		}
		// Deleting atmospheric and parallel data
		if (atmosphericData)
			delete atmosphericData;

		// Deleting optimized data brokers
		delete[] optimizedPropDataBroker;
		delete[] optimizedFluxDataBroker;
	}

	void DataBroker::commonInitialization()
	{
		/* atmospheric data */
		atmosphericData = new AtmosphericData();
		/* parallel data */
	
		/* data brokers for propagation models */
		propDataGetters.resize(FireDomain::NUM_MAX_PROPMODELS);
		numPropDataGetters.resize(FireDomain::NUM_MAX_PROPMODELS);
		optimizedPropDataBroker = new bool[FireDomain::NUM_MAX_PROPMODELS];
		for (size_t i = 0; i < FireDomain::NUM_MAX_PROPMODELS; i++)
		{
			numPropDataGetters[i] = 0;
			optimizedPropDataBroker[i] = true;
		}
		frontScanDistance = params->getDouble("frontScanDistance");
		/* data brokers for flux models */
		fluxDataGetters.resize(FireDomain::NUM_MAX_FLUXMODELS);
		numFluxDataGetters.resize(FireDomain::NUM_MAX_FLUXMODELS);
		optimizedFluxDataBroker = new bool[FireDomain::NUM_MAX_FLUXMODELS];
		for (size_t i = 0; i < FireDomain::NUM_MAX_FLUXMODELS; i++)
		{
			numFluxDataGetters[i] = 0;
			optimizedFluxDataBroker[i] = true;
		}

		/* Getting the fuel parameters' table */
		/*------------------------------------*/
		ostringstream infile;
		/*
		infile << params->getParameter("caseDirectory") << '/'
				<< params->getParameter("ForeFireDataDirectory") << '/'
				<< params->getParameter("fuelsTableFile");
		*/
		std::string paramTable = params->getParameter("fuelsTable");
		if (paramTable != "1234567890")
		{
			readTableFromString(paramTable, fuelPropertiesTable);
		}
		else
		{
			std::ostringstream infile;
			infile << params->GetPath(params->getParameter("fuelsTableFile"));
			readTableFromAsciiFile(infile.str(), fuelPropertiesTable);
		}
	}

	void DataBroker::updateFuelValues(PropagationModel *model, string key, double value)
	{

		size_t curfuel = 0;
		map<string, double>::iterator iindex;
		map<string, double>::iterator iparam;
		// second dimension
		for (curfuel = 0; curfuel < fuelPropertiesTable.size(); curfuel++)
		{
			// Getting the index of the fuel
			iindex = fuelPropertiesTable[curfuel].find(key);
			if (iindex != fuelPropertiesTable[curfuel].end())
			{
				iindex->second = value;
			}
		}

		extractFuelProperties(fuelPropertiesTable, model);
	}

	void DataBroker::registerPropagationModel(PropagationModel *model)
	{
		/* Constructing the vector of property getters */

		propGetterMap::const_iterator pg;
		bool fuelAsked = false;

		for (size_t prop = 0; prop < model->numProperties; prop++)
		{
			try
			{
				if ((model->wantedProperties)[prop].substr(0, 4) == "fuel")
				{
					if (!fuelAsked)
					{
						propDataGetters[model->index].push_back(&getFuelProperties);
						numPropDataGetters[model->index]++;
						neededProperties.push_back("fuel");
						fuelAsked = true;
					}
				}
				else
				{
					pg = propPropertiesGetters.find(
						(model->wantedProperties)[prop]);
					if (pg == propPropertiesGetters.end())
					{
						optimizedPropDataBroker[model->index] = false;
						cout << "WARNING: could not find an optimized property getter for "
							 << (model->wantedProperties)[prop] << endl;
						cout << "databroker is switched to an un-optimized mode"
							 << endl;
					}
					else
					{
						propDataGetters[model->index].push_back(pg->second);
						numPropDataGetters[model->index]++;
					}
					if (find(neededProperties.begin(), neededProperties.end(),
							 (model->wantedProperties)[prop]) == neededProperties.end())
						neededProperties.push_back((model->wantedProperties)[prop]);
				}
			}
			catch (...)
			{
				cout
					<< "Encountered an error while constructing the function to compute "
					<< (model->wantedProperties)[prop] << endl;
			}
		}

		/* registering the prop model in the fire domain */
		domain->registerPropagationModel(model->index, model);

		/* constructing the table of fuel parameters values */
		if (model->numFuelProperties > 0)
			extractFuelProperties(fuelPropertiesTable, model);
	}

	void DataBroker::registerFluxModel(FluxModel *model)
	{

		/* Constructing the vector of property getters */
		fluxGetterMap::const_iterator pg;
		bool fuelAsked = false;
		for (size_t prop = 0; prop < model->numProperties; prop++)
		{
			try
			{
				if ((model->wantedProperties)[prop].substr(0, 4) == "fuel")
				{
					if (!fuelAsked)
					{
						fluxDataGetters[model->index].push_back(&getFuelProperties);
						numFluxDataGetters[model->index]++;
						if (find(neededProperties.begin(), neededProperties.end(),
								 "fuel") == neededProperties.end())
							neededProperties.push_back("fuel");
						fuelAsked = true;
					}
				}
				else
				{
					pg = fluxPropertiesGetters.find(
						(model->wantedProperties)[prop]);
					if (pg == fluxPropertiesGetters.end())
					{
						cout << "could not find an optimized property getter for "
							 << (model->wantedProperties)[prop] << endl;
						cout << "databroker is switched to an un-optimized mode"
							 << endl;
						optimizedFluxDataBroker[model->index] = false;
					}
					else
					{
						fluxDataGetters[model->index].push_back(pg->second);
						numFluxDataGetters[model->index]++;
					}
					if (find(neededProperties.begin(), neededProperties.end(),
							 (model->wantedProperties)[prop]) == neededProperties.end())
						neededProperties.push_back((model->wantedProperties)[prop]);
				}
			}
			catch (...)
			{
				cout
					<< "Encountered an error while constructing the function to compute "
					<< (model->wantedProperties)[prop] << endl;
			}
		}

		/* registering the flux model in the fire domain */
		domain->registerFluxModel(model->index, model);

		/* constructing the table of fuel parameters values */
		if (model->numFuelProperties > 0)
			extractFuelProperties(fuelPropertiesTable, model);
	}

	void DataBroker::extractFuelProperties(vector<map<string, double>> propsTable,
										   ForeFireModel *model)
	{

		try
		{
			delete (model->fuelPropertiesTable);
			model->fuelPropertiesTable = new FFArray<double>("fuelProperties", 0.,
															 FuelDataLayer<double>::MAXNUMFUELS, model->numFuelProperties);
			size_t curfuel = 0;
			map<string, double>::iterator iindex;
			map<string, double>::iterator iparam;
			// second dimension
			for (curfuel = 0; curfuel < propsTable.size(); curfuel++)
			{
				// Getting the index of the fuel
				iindex = propsTable[curfuel].find("Index");
				size_t ind = (size_t)iindex->second;
				// Getting the values of the wanted parameters
				for (size_t param = 0; param < model->numFuelProperties; param++)
				{
					iparam = propsTable[curfuel].find(
						(model->fuelPropertiesNames)[param]);
					if (iparam != propsTable[curfuel].end())
					{
						(*(model->fuelPropertiesTable))(ind, param) =
							iparam->second;
					}
					else
					{
						cout << "Parameter " << (model->fuelPropertiesNames)[param]
							 << " could not be found " << "for fuel " << ind
							 << " in the fuel table" << endl;
					}
				}
			}
		}
		catch (const bad_alloc &)
		{
			// deleting what has been allocated
			cout << "ERROR : while retrieving the table of wanted fuel parameters"
				 << " in FuelDataLayer<T>::extractParameters" << endl;
		}
	}

	void DataBroker::registerLayer(string name, DataLayer<double> *layer)
	{

		/* inserting the layer into the map of layers */
		 
		ilayer = layersMap.find(name);
		if (ilayer != layersMap.end())
		{

			if (params->getParameter("runmode") == "standalone")
			{
				// cout << "Redefining layer for variable " << name << " !" << endl;
				DataLayer<double> *oldlayer = ilayer->second;
				layersMap.erase(name);
				layers.remove(oldlayer);
				delete oldlayer;
			}
		}

		layersMap.insert(make_pair(name, layer));
		layers.push_back(layer);

		/* looking for possible match with predefined layers */
		if (name.find("altitude") != string::npos)
		{
			altitudeLayer = layer;
			// along with the topography comes the slope
			slopeLayer = new GradientDataLayer<double>("slope", altitudeLayer,
													   params->getDouble("spatialIncrement"));
			registerLayer("slope", slopeLayer);
		}
		if (name.find("forced_arrival_time_of_front") != string::npos)
		{
			// if added
			// ForcedROSLayer = ;
			forcedArrivalTimeLayer = new TimeGradientDataLayer<double>("arrival_time_gradient", layer,
																	   params->getDouble("LookAheadDistanceForeTimeGradientDataLayer"));

			registerLayer("arrival_time_gradient", forcedArrivalTimeLayer);
		}
		if (name.find("moisture") != string::npos)
		{
			moistureLayer = layer;
		}
		if (name.find("temperature") != string::npos)
		{
			temperatureLayer = layer;
		}
		if (name.find("windU") != string::npos)
		{

			windULayer = layer;
		}
		if (name.find("windV") != string::npos)
		{
			windVLayer = layer;
		}
		if (name.find("fuel") != string::npos)
		{
			fuelLayer = layer;
		}
	}

	void DataBroker::registerFluxLayer(string name, FluxLayer<double> *layer)
	{

		flayer = fluxLayersMap.find(name);
		if (flayer != fluxLayersMap.end())
		{
			FluxLayer<double> *oldlayer = flayer->second;
			fluxLayersMap.erase(name);
			fluxLayers.remove(oldlayer);
			delete oldlayer;
		}
		fluxLayersMap.insert(make_pair(name, layer));
		fluxLayers.push_back(layer);
		registerLayer(name, layer);

		/* looking for possible match with predefined layers */
		if (name.find("heatFlux") != string::npos)
		{

			heatFluxLayer = layer;
		}
	}

	void DataBroker::setAtmosphericDomain(const FFPoint &SWCorner,
										  const FFPoint &NECorner, const size_t &nx, const size_t &ny)
	{
		atmoSWCorner = SWCorner;
		atmoNECorner = NECorner;
		atmosphericNx = nx;
		atmosphericNy = ny;
	}

	void DataBroker::initializeAtmosphericLayers(const double &time,
												 const size_t &bnx, const size_t &bny)
	{

		atmosphericData->setSize(atmosphericNx, atmosphericNy);

		double dx = (atmoNECorner.getX() - atmoSWCorner.getX()) / atmosphericNx;
		double dy = (atmoNECorner.getY() - atmoSWCorner.getY()) / atmosphericNy;

		// Loading the atmospheric layers
		FFPoint windUOrigin = FFPoint(atmoSWCorner.getX() - dx,
									  atmoSWCorner.getY() - 0.5 * dy, 0);
		TwoTimeArrayLayer<double> *wul = new TwoTimeArrayLayer<double>("windU",
																	   atmosphericData->windU, time, atmosphericData->oldWindU, time,
																	   windUOrigin, dx, dy);
		registerLayer("windU", wul);
		registerLayer("outerWindU", wul);
		FFPoint windVOrigin = FFPoint(atmoSWCorner.getX() - 0.5 * dx,
									  atmoSWCorner.getY() - dy, 0);
		TwoTimeArrayLayer<double> *wvl = new TwoTimeArrayLayer<double>("windV",
																	   atmosphericData->windV, time, atmosphericData->oldWindV, time,
																	   windVOrigin, dx, dy);
		registerLayer("windV", wvl);
		registerLayer("outerWindV", wvl);

		// Loading the topography
		Array2DdataLayer<double> *alt = new Array2DdataLayer<double>("altitude", atmosphericData->topography, atmoSWCorner, atmoNECorner);

		registerLayer("altitude", alt);
	}

	void DataBroker::loadMultiWindBin(double refTime, size_t numberOfDomains, size_t *startI, size_t *startJ)
	{
		string windFileName(params->getParameter("caseDirectory") + '/' + params->getParameter("PPath") + '/' + to_string(FireDomain::atmoIterNumber % 2) + "/");

		if (windULayer != 0)
		{
			((TwoTimeArrayLayer<double> *)windULayer)->loadMultiWindBin(windFileName, refTime, numberOfDomains, startI, startJ);
		}
		if (windVLayer != 0)
		{
			((TwoTimeArrayLayer<double> *)windVLayer)->loadMultiWindBin(windFileName, refTime, numberOfDomains, startI, startJ);
		}
	}

	

	void DataBroker::addConstantLayer(string name, const double &val)
	{
		/* Adding a simple constant layer for variable name */
		XYZTDataLayer<double> *newLayer = new XYZTDataLayer<double>(name, val);
		registerLayer(name, newLayer);
	}

	void DataBroker::dropProperty(string name)
	{
		vector<string>::iterator it = find(neededProperties.begin(),
										   neededProperties.end(), name);
		if (it != neededProperties.end())
			neededProperties.erase(it);
	}

	void DataBroker::insureLayersExistence()
	{
		/* checking the layers needed for the propagation and flux models */
		while (!neededProperties.empty())
		{

			if (getLayer(neededProperties.back()) == 0)
			{
				if (neededProperties.back().find("normalWind") != string::npos)
				{
					/*
					if (domain->getDomainID()==0) {
						cout<<"looking for  "<<neededProperties.back()<<endl;
						if (windULayer != 0)
							cout<<"windULayer is  "<<windULayer->getKey()<<endl;
						}
					if (windULayer == 0 or windVLayer == 0)
						cout<<windULayer << "layer for normal wind doesn't rely on existing"
								<< " windU and windV layers, creating them if needed"
								<< endl;*/
					/* special treatment for normal wind */
					if (windULayer == 0)
					{
						double val = 0.;
						if (params->isValued("windU"))
							val = params->getDouble("windU");
						addConstantLayer("windU", val);
					}
					if (windVLayer == 0)
					{
						double val = 0.;
						if (params->isValued("windV"))
							val = params->getDouble("windV");
						addConstantLayer("windV", val);
					}
				}
				else if (neededProperties.back().find("uel") != string::npos)
				{
					cout
						<< "layer for fuel doesn't rely on existing fuel layer, creating it"
						<< endl;
					int fuelIndex = 0;
					if (params->isValued("fuel"))
						fuelIndex = params->getDouble("fuel");
					fuelLayer = new FuelDataLayer<double>("fuel", fuelIndex);
				}
				else if (neededProperties.back().find("slope") != string::npos)
				{
					cout << "DOMAIN " << domain->getDomainID()
						 << "layer for slope doesn't rely on existing altitude layer, creating it"
						 << endl;
					double val = 0.;
					if (params->isValued("altitude"))
						val = params->getDouble("altitude");
					addConstantLayer("altitude", val);
				}
				else if (neededProperties.back().find("epth") != string::npos)
				{
					int fdepth = 1;
					FireNode::setFrontDepthComputation(fdepth);
					/* checking that a heat flux layer is present for burn checks.
					 * If not, instantiating a HeatFluxBasicModel */
					if (heatFluxLayer == 0 and !domain->addFluxLayer("heatFlux"))
					{
						cout
							<< "WARNING: heat flux layer could not be found within ForeFire framework, this should"
							<< " cause serious problems for the simulation as front depth is required"
							<< endl;
					}
				}
				else if (neededProperties.back().find("urvature") != string::npos)
				{
					int curv = 1;
					FireNode::setCurvatureComputation(curv);
				}
				else
				{
					/* else instantiating a constant layer for the property */
					double val = 0.;
					if (params->isValued(neededProperties.back()))
						val = params->getDouble(neededProperties.back());
					addConstantLayer(neededProperties.back(), val);
				}
			}
			/* erasing the property from the list of properties to be treated */
			dropProperty(neededProperties.back());
		}

		/* checking the layers that are always needed */
		if (altitudeLayer == 0)
		{
			double val = 0.;
			if (params->isValued("altitude"))
				val = params->getDouble("altitude");
			addConstantLayer("altitude", val);
		}
	}

	void DataBroker::initFluxLayers(const double &t)
	{
		list<FluxLayer<double> *>::iterator flay;
		for (flay = fluxLayers.begin(); flay != fluxLayers.end(); ++flay)
			(*flay)->setFirstCall(t);
	}

	bool DataBroker::isRelevantData(FFPoint &SW, FFPoint &ext)
	{
		if (SW.getX() > domain->NECornerX() or SW.getX() + ext.getX() < domain->SWCornerX() or SW.getY() > domain->NECornerY() or SW.getY() + ext.getY() < domain->SWCornerY())
			return false;
		return true;
	}

	void DataBroker::readTableFromString(const std::string &data,
										 std::vector<std::map<std::string, double>> &table)
	{
		std::istringstream dataStream(data);
		std::string line;
		const std::string delimiter = ";";
		std::vector<std::string> paramNames;

		// Get the first line to extract parameter names
		if (!std::getline(dataStream, line))
		{
			throw std::runtime_error("ERROR: could not read the header line from data.");
		}
		tokenize(line, paramNames, delimiter);

		// Process the rest of the data
		std::vector<std::string> vals;
		double dval;
		while (std::getline(dataStream, line))
		{
			vals.clear();
			tokenize(line, vals, delimiter);
			if (vals.size() != paramNames.size())
			{
				throw std::runtime_error("ERROR: Number of values does not match number of parameters for fuel " + vals[0]);
			}

			std::map<std::string, double> currentMap;
			for (size_t pos = 0; pos < vals.size(); ++pos)
			{
				std::istringstream iss(vals[pos]);
				if (!(iss >> dval))
				{
					throw std::runtime_error("ERROR: could not cast " + vals[pos] +
											 " into a suitable value for parameter " + paramNames[pos] +
											 " of fuel " + vals[0]);
				}
				currentMap[paramNames[pos]] = dval;
			}
			table.push_back(std::move(currentMap));
		}
	}

	void DataBroker::readTableFromAsciiFile(string filename,
											vector<map<string, double>> &table)
	{

		vector<string> paramNames;

		/* Opening the file */
		ifstream file(filename.c_str());

		if (!file)
		{
			cout << "WARNING: could not load file for fuel properties " << filename
				 << endl;
			return;
		}

		string line;
		const string delimiter = ";";

		/* getting lines of parameter one after each other.
		 * First line corresponds to the keys of the parameters. */
		getline(file, line);
		tokenize(line, paramNames, delimiter);

		/* Retrieving the data in the other lines */
		size_t fuelNum = 0;
		vector<string> vals;
		double dval;
		while (getline(file, line))
		{
			/* Affecting the map */
			table.push_back(map<string, double>());
			/* cutting the line according to delimiter */
			tokenize(line, vals, delimiter);
			if (vals.size() != paramNames.size())
			{
				cout << "Number of parameters for fuel " << vals[0]
					 << " isn't in accordance with the number of parameters in the table"
					 << " (" << vals.size() << " vs " << paramNames.size() << ")"
					 << endl;
				;
			}
			else
			{
				for (size_t pos = 0; pos < vals.size(); pos++)
				{
					istringstream iss(vals[pos]);
					if (iss >> dval)
					{
						table[fuelNum].insert(make_pair(paramNames[pos], dval));
						// cout << "at "<<fuelNum<< "  param "<<paramNames[pos] <<" = "<<dval<<endl;
					}
					else
					{
						cout << "could not cast " << vals[pos]
							 << " into a suitable value " << "for parameter "
							 << paramNames[pos] << " of fuel " << vals[0]
							 << endl;
					}
				}
				fuelNum++;
			}
			vals.clear();
		}
		file.close();
	}

	void DataBroker::tokenize(const string &str, vector<string> &tokens,
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

	void DataBroker::getPropagationData(PropagationModel *model, FireNode *fn)
	{
		size_t nfilled = 0;
		if (optimizedPropDataBroker[model->index])
		{

			for (size_t prop = 0; prop < numPropDataGetters[model->index]; prop++)
			{

				nfilled += (propDataGetters[model->index][prop])(fn, model,
																 nfilled);
			}
		}
		else
		{

			for (size_t prop = 0; prop < model->numProperties; prop++)
			{

				nfilled += getLayer(model->wantedProperties[prop])->getValuesAt(fn, model, nfilled);
			}
		}
	}

	void DataBroker::getFluxData(FluxModel *model, FFPoint &loc, const double &t)
	{
		size_t nfilled = 0;
		if (optimizedFluxDataBroker[model->index])
		{
			for (size_t prop = 0; prop < numFluxDataGetters[model->index]; prop++)
			{
				nfilled += (fluxDataGetters[model->index][prop])(loc, t, model,
																 nfilled);
			}
		}
		else
		{
			for (size_t prop = 0; prop < model->numProperties; prop++)
			{
				nfilled += getLayer(model->wantedProperties[prop])->getValuesAt(loc, t, model, nfilled);
			}
		}
	}

	double DataBroker::getProperty(FireNode *fn, string property)
	{
		return getLayer(property)->getValueAt(fn);
	}

	vector<string> DataBroker::getAllLayerNames() {
		vector<string> names;
		
		// Iterate over regular layers and add their names.
		for (const auto &entry : layersMap) {
			names.push_back(entry.first);
		}
		
		// Iterate over flux layers and add their names.
		for (const auto &entry : fluxLayersMap) {
			names.push_back(entry.first);
		}
		
		return names;
	}

	DataLayer<double> *DataBroker::getLayer(const string &property)
	{
		// Scanning the scalar layers

		ilayer = layersMap.find(property);
		if (ilayer != layersMap.end())
			return ilayer->second;

		
		return 0;
	}

	bool DataBroker::hasLayer(const string &property)
	{
		// Scanning the scalar layers

		ilayer = layersMap.find(property);
		if (ilayer != layersMap.end())
			return true;
		return false;
	}

	FluxLayer<double> *DataBroker::getFluxLayer(const string &property)
	{
		// Scanning the scalar layers

		flayer = fluxLayersMap.find(property);
		if (flayer != fluxLayersMap.end())
			return flayer->second;

		return 0;
	}
	void DataBroker::computeActiveSurfacesFlux(const double &t)
	{
		// Scanning the scalar layers
		map<string, FluxLayer<double> *>::iterator iter = fluxLayersMap.begin();
		int numFluxModelsMax = 10;
		for (; iter != fluxLayersMap.end(); ++iter)
		{

			FluxLayer<double> *flayer = iter->second;
			int modelCount[numFluxModelsMax];
			for (int i = 0; i < numFluxModelsMax; i++)
				modelCount[i] = 0;
			flayer->computeActiveMatrix(t, modelCount);
		}
	}

	/* *************************************** */
	/* Property getters for propagation models */
	/* *************************************** */

	int DataBroker::getDummy(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = dummyLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getFuelProperties(FireNode *fn, PropagationModel *model,
									  int start)
	{
		int numberOfValuesFilled = fuelLayer->getValuesAt(fn, model, start);
		return numberOfValuesFilled;
	}

	int DataBroker::getMoisture(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = moistureLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getTemperature(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = temperatureLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getAltitude(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = altitudeLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getFirenodeLocX(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = fn->getLoc().getX();
		return 1;
	}

	int DataBroker::getFirenodeLocY(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = fn->getLoc().getY();
		return 1;
	}

	int DataBroker::getFirenodeID(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = fn->getID();
		return 1;
	}

	int DataBroker::getFirenodeTime(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = fn->getTime();
		return 1;
	}
	int DataBroker::getFirenodeState(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = fn->getState();
		return 1;
	}

	int DataBroker::getArrival_time_gradient(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = forcedArrivalTimeLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getSlope(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = slopeLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getWindU(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = windULayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getWindV(FireNode *fn, PropagationModel *model, int keynum)
	{
		(model->properties)[keynum] = windVLayer->getValueAt(fn);
		return 1;
	}

	int DataBroker::getNormalWind(FireNode *fn, PropagationModel *model,
								  int keynum)
	{
		double u = windULayer->getValueAt(fn);
		double v = windVLayer->getValueAt(fn);
		FFVector wind = FFVector(u, v);
		(model->properties)[keynum] = wind.scalarProduct(fn->getNormal());
		return 1;
	}

	int DataBroker::getFrontDepth(FireNode *fn, PropagationModel *model,
								  int keynum)
	{
		(model->properties)[keynum] = fn->getFrontDepth();
		return 1;
	}

	int DataBroker::getFrontCurvature(FireNode *fn, PropagationModel *model,
									  int keynum)
	{
		(model->properties)[keynum] = fn->getCurvature();
		return 1;
	}
	int DataBroker::getFrontFastestInSection(FireNode *fn, PropagationModel *model,
											 int keynum)
	{
		(model->properties)[keynum] = fn->getLowestNearby(frontScanDistance);
		return 1;
	}

	/* ******************************** */
	/* Property getters for flux models */
	/* ******************************** */

	int DataBroker::getFuelProperties(FFPoint loc, const double &t, FluxModel *model,
									  int start)
	{
		int numberOfValuesFilled = fuelLayer->getValuesAt(loc, t, model, start);
		return numberOfValuesFilled;
	}

	int DataBroker::getMoisture(FFPoint loc, const double &t, FluxModel *model,
								int keynum)
	{
		(model->properties)[keynum] = moistureLayer->getValueAt(loc, t);
		return 1;
	}

	int DataBroker::getAltitude(FFPoint loc, const double &t, FluxModel *model,
								int keynum)
	{
		(model->properties)[keynum] = altitudeLayer->getValueAt(loc, t);
		return 1;
	}

	int DataBroker::getWindU(FFPoint loc, const double &t, FluxModel *model,
							 int keynum)
	{
		(model->properties)[keynum] = windULayer->getValueAt(loc, t);
		return 1;
	}

	int DataBroker::getWindV(FFPoint loc, const double &t, FluxModel *model,
							 int keynum)
	{
		(model->properties)[keynum] = windVLayer->getValueAt(loc, t);
		return 1;
	}

	void DataBroker::getMatrix(string matrixName, const FFPoint &SWCorner,
							   const FFPoint &NECorner, const double &t, FFArray<double> **matrix)
	{
		ilayer = layersMap.find(matrixName);
		if (ilayer != layersMap.end())
		{
			try
			{
				ilayer->second->getMatrix(matrix, t);
			}
			catch (...)
			{
				cout << "Encountered an error while retrieving matrix "
					 << matrixName << " with the data broker" << endl;
			}
		}
		else
		{

			cout << "Encountered an error while retrieving matrix "
				 << matrixName << " with the data broker: unknown matrix " << matrixName << endl;
		}
	}

	string DataBroker::printLayers()
	{
		ostringstream oss;
		oss << "Layers referenced in the data broker are:" << endl;
		for (ilayer = layersMap.begin(); ilayer != layersMap.end(); ++ilayer)
		{
			if (ilayer->second == 0)
			{
				oss << '\t' << "problem with a de-referenced layer" << endl;
				return oss.str();
			}
			oss << '\t' << "layer for property " << ilayer->second->getKey()
				<< " is at " << ilayer->second << endl;
		}
		return oss.str();
	}

	void DataBroker::loadFromNCFile(string filename)
	{
		try
		{
			NcFile dataFile(filename.c_str(), NcFile::read);
			propGetterMap::const_iterator pg;
			if (!dataFile.isNull())
			{
				NcVar domvar = dataFile.getVar("domain");
				double version = 1;
				map<string,NcVarAtt> attributeList = domvar.getAtts();
				map<string,NcVarAtt>::iterator myIter;
				myIter = attributeList.find("version");
				if(myIter != attributeList.end()){
					myIter->second.getValues(&version);
				}

				double Xorigin = 0;
				double Yorigin = 0;
				double Zorigin = 0;
				NcVarAtt att = domvar.getAtt("SWx");
				if(!att.isNull()){
					   att.getValues(&Xorigin);
				} 
				att = domvar.getAtt("SWy");
				if(!att.isNull()){
					   att.getValues(&Yorigin);
				} 
				att = domvar.getAtt("SWz");
				if(!att.isNull()){
					   att.getValues(&Zorigin);
				} 
				FFPoint SWCorner = FFPoint(Xorigin, Yorigin, Zorigin);


				double Lx = 0;
				double Ly = 0; 
				double Lz = 0; 
				 att = domvar.getAtt("Lx");
				if(!att.isNull()) att.getValues(&Lx);
				att = domvar.getAtt("Ly");
				if(!att.isNull()) att.getValues(&Ly);
				att = domvar.getAtt("Lz");
				if(!att.isNull()) att.getValues(&Lz);
				FFPoint spatialExtent = FFPoint(Lx, Ly, Lz);

				double timeOrigin =0;
				att = domvar.getAtt("t0");
			    if(!att.isNull()) att.getValues(&timeOrigin);
			
				double Lt = 0;
				att = domvar.getAtt("t0");
			    if(!att.isNull()) att.getValues(&timeOrigin);

				std::multimap<std::string, NcVar> allVariables = dataFile.getVars();
				NcVarAtt layerTypeNC;
				string layerType;
				for (auto it = allVariables.begin(); it != allVariables.end(); ++it)
				{

					try
					{
						layerTypeNC = it->second.getAtt("type");
						if (layerTypeNC.isNull())
						{
							layerType = "";
						}
						else
						{
							layerTypeNC.getValues(layerType);
						}
					}
					catch (const NcException &e)
					{
						std::cout << "Warning: Variable " << it->first
								  << " does not have a 'type' attribute or it could not be read. Skipping..."
								  << std::endl;
						continue; // Skip this variable and move to the next one
					}

					if (layerType.empty())
					{
						std::cout << "Skipping variable " << it->first
								  << " because 'type' attribute is missing."
								  << std::endl;
						continue;
					}

					string varName = it->first;

					if (varName == "wind")
					{ 
					 
						XYZTDataLayer<double> *wul = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, 0);
						registerLayer("windU", wul);
						XYZTDataLayer<double> *wvl = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, 1);
						registerLayer("windV", wvl);
						PwindULayer = wul;
						PwindVLayer = wvl;
					}

					pg = propPropertiesGetters.find(varName);
					if (pg != propPropertiesGetters.end())
					{
						if (varName == "altitude")
						{
							if (domain->getDomainID() == 0)
							{
								// altitude is given by the NetCDF file
								XYZTDataLayer<double> *altus = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
								registerLayer(varName, altus);
							}
						}
						else if (varName == "windU")
						{
							// wind is given by the NetCDF file
							XYZTDataLayer<double> *wul = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							windULayer = wul;
							registerLayer(varName, wul);
						}
						else if (varName == "windV")
						{
							// wind is given by the NetCDF file
							XYZTDataLayer<double> *wvl = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer(varName, wvl);
						}
						else if (varName == "temperature")
						{
							// wind is given by the NetCDF file
							XYZTDataLayer<double> *temp = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer(varName, temp);
						}
						else if (varName == "moisture")
						{
							// moinsture is given by the NetCDF file
							XYZTDataLayer<double> *moist = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer(varName, moist);
						}
						else if (varName == "fuel")
						{
							fuelLayer = constructFuelLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer("fuel", fuelLayer);
						}
						else if (varName == "fieldSpeed")
						{
							dummyLayer = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer(varName, dummyLayer);
						}

					}

					else if (layerType.find("data") != string::npos)
					{
						if (find(neededProperties.begin(), neededProperties.end(), varName) != neededProperties.end())
						{
							XYZTDataLayer<double> *newlayer = constructXYZTLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
							registerLayer(varName, newlayer);
						}
					}
					else if (layerType.find("flux") != string::npos)	
					{
						FluxLayer<double> *newlayer = constructFluxLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						registerFluxLayer(it->first, newlayer);
					}
					else if (layerType.find("propagative") != string::npos)
					{
						cout<<varName<<" in propagation layers"<<endl;
						PropagativeLayer<double> *propLayer = constructPropagativeLayer(it->second, SWCorner, spatialExtent, timeOrigin, Lt, -1);
						domain->setPropagativeLayer(propLayer);
					}
					else if (layerType.find("parameter") != string::npos)
					{
						//	 std::cout << "PARAMETERS LAYERS IN NCFILE.... WHY ??? " << varName;
						map<string, NcVarAtt> attributeList = it->second.getAtts();
						map<string, NcVarAtt>::iterator myIter;
						for (myIter = attributeList.begin(); myIter != attributeList.end(); ++myIter)
						{
							NcVarAtt att = myIter->second;
							NcType attValType = att.getType();
							string attsVal;
							int attiVal;
							float attfVal;
							double attdVal;
							switch ((int)attValType.getTypeClass())
							{
							case NC_CHAR:
								att.getValues(attsVal);
								params->setParameter(myIter->first, attsVal);
								break;
							case NC_INT:
								att.getValues(&attiVal);
								params->setParameter(myIter->first, std::to_string(attiVal));
								break;
							case NC_FLOAT:
								att.getValues(&attfVal);
								params->setParameter(myIter->first, std::to_string(attfVal));
								break;
							case NC_DOUBLE:
								att.getValues(&attdVal);
								params->setParameter(myIter->first, std::to_string(attdVal));
								break;
							default:
								std::cout << myIter->first << " attribute of unhandled type " << attValType.getName() << endl;
								break;
							}
						}
					}
				}
				if (domain->getPropagativeLayer() == 0)
				{
					if (!domain->addPropagativeLayer(params->getParameter("propagationModel")))
					{
						cout << "PROBLEM: it was not possible to retrieve propagation model " << params->getParameter("propagationModel") << endl;
					}
				}

				if (domain->getPropagativeLayer() != 0)
					registerLayer(domain->getPropagativeLayer()->getKey(), domain->getPropagativeLayer());
			}
		}
		catch (NcException &e)
		{
			e.what();
		}
	}
	

	XYZTDataLayer<double> *DataBroker::constructXYZTLayer(NcVar& values, FFPoint &SWCorner, FFPoint &spatialExtent,
														  double timeOrigin, double Lt, int dimTSelected /* = -1 */)
	{
		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount()) {
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
	
	PropagativeLayer<double> *DataBroker::constructPropagativeLayer(NcVar& values, FFPoint &SWCorner, FFPoint &spatialExtent,
		double timeOrigin, double Lt, int dimTSelected /* = -1 */)
	{
		cout << "DataBroker::constructPropagativeLayerFromFile " << " from netCDF Not Implemented" << endl;
		return NULL;
	}

	FuelDataLayer<double>* DataBroker::constructFuelLayer(NcVar& values, FFPoint &SWCorner, FFPoint &spatialExtent,
		double timeOrigin, double Lt, int dimTSelected /* = -1 */)
	{
		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount()) {
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
		/* Getting the data */
		/*------------------*/
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
			FuelDataLayer<double> *newlayer = new FuelDataLayer<double>(property, SWCorner, timeOrigin, spatialExtent, Lt, nx, ny, nz, nt, fuelMap);
			delete[] fuelMap;
			return newlayer;
		}
	 
	}

	FluxLayer<double> *DataBroker::constructFluxLayer(NcVar& values, FFPoint &SWCorner, FFPoint &spatialExtent,
		double timeOrigin, double Lt, int dimTSelected /* = -1 */)
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
		NcVarAtt tmpName = values.getAtt(mname.str());//mname.str());
		if(!tmpName.isNull()) tmpName.getValues(modelName);
		domain->fluxModelInstanciation(indModel, modelName);

		
		size_t nx = 0, ny = 0, nz = 0, nt = 0;
		switch (values.getDimCount()) {
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
