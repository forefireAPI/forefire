/**
 * @file DataBroker.cpp
 * @brief DataBroker class is a static class that manages spatial data access and uses layers as data elements, it is the one loading data files
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
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
 

		// Deleting optimized data brokers
		delete[] optimizedPropDataBroker;
		delete[] optimizedFluxDataBroker;
	}

	void DataBroker::commonInitialization()
	{
 

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

		std::string paramTable = params->getParameter("fuelsTable");
		if (paramTable != "1234567890")
		{
		 
			if (paramTable.substr(0, 3) == "STD")
			{
			 
				readTableFromString(params->getParameter(paramTable), fuelPropertiesTable);
			}
			else
			{
				readTableFromString(paramTable, fuelPropertiesTable);
			}
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
		bool moistAsked = false;
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
				if ((model->wantedProperties)[prop].substr(0, 5) == "moist")
				{
					if (!moistAsked)
					{
						propDataGetters[model->index].push_back(&getMoisturesProperties);
						numPropDataGetters[model->index]++;
						neededProperties.push_back("fuel");
						moistAsked = true;
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

			//	if (params->getParameter("runmode") == "standalone")
			//	{
			// cout << "Redefining layer for variable " << name << " !" << endl;
			DataLayer<double> *oldlayer = ilayer->second;
			layersMap.erase(name);
			layers.remove(oldlayer);
			delete oldlayer;
			//}
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


	void DataBroker::createEmpty2DLayer(const string &name,  FFPoint &swC,  FFPoint &neC, const size_t nx, const size_t ny )
	{
		    FFArray<double>* xARR  = new FFArray<double>(name, 0., nx, ny);
			Array2DdataLayer<double> *xLAY = new Array2DdataLayer<double>(name, xARR, swC, neC);
			registerLayer(name, xLAY);
	}

	void DataBroker::createEmptyTwoTimeArrayLayer(const string &name,  FFPoint &Forigin,  double addTime, const double dx, const double dy )
	{

		FFArray<double>* xARR  = new FFArray<double>(name, 0., atmosphericNy, atmosphericNy);
		FFArray<double>*  xARRDT = new FFArray<double>(name, 0., atmosphericNx+2, atmosphericNy+2);
		FFArray<double>*  xARRT = new FFArray<double>("Old"+name, 0., atmosphericNx+2, atmosphericNy+2);
		TwoTimeArrayLayer<double> *xLAY = new TwoTimeArrayLayer<double>(name,
																	   xARRDT, addTime, xARRT, addTime,
																	   Forigin, dx, dy);
	
		registerLayer(name, xLAY);
	}
	void DataBroker::createEmptyScalarLayer(const string &name,   double addTime )
	{
		double dx = (atmoNECorner.getX() - atmoSWCorner.getX()) / atmosphericNx;
		double dy = (atmoNECorner.getY() - atmoSWCorner.getY()) / atmosphericNy;


		FFPoint scalarOrigin = FFPoint(atmoSWCorner.getX() - 0.5 * dx, atmoSWCorner.getY() - 1 * dy, 0);
		createEmptyTwoTimeArrayLayer(name,  scalarOrigin,  addTime,  dx, dy );

	}


	void DataBroker::initializeAtmosphericLayers(const double &time)
	{
 
		double dx = (atmoNECorner.getX() - atmoSWCorner.getX()) / atmosphericNx;
		double dy = (atmoNECorner.getY() - atmoSWCorner.getY()) / atmosphericNy;

		if (domain->getDomainID() != 0)
		{
			createEmpty2DLayer("altitude", atmoSWCorner, atmoNECorner, atmosphericNx, atmosphericNy);
		}

				// Loading the atmospheric layers
		FFPoint windUOrigin = FFPoint(atmoSWCorner.getX() - dx,  atmoSWCorner.getY() - 0.5 * dy, 0);
        createEmptyTwoTimeArrayLayer("windU",  windUOrigin,  time,  dx, dy );

		FFPoint windVOrigin = FFPoint(atmoSWCorner.getX() - 0.5 * dx, atmoSWCorner.getY() - dy, 0);
        createEmptyTwoTimeArrayLayer("windV",  windVOrigin,  time,  dx, dy );
		
		vector<string> optLayers =	params->getParameterArray("MNHExchangeScalarLayersNames");
		for (size_t i = 0; i < optLayers.size(); i++)
		{
			createEmptyScalarLayer(optLayers[i], time);
		}
		optLayers =	params->getParameterArray("accumulatedDiagnosticScalarLayersNames");
		for (size_t i = 0; i < optLayers.size(); i++)
		{
			createEmptyScalarLayer(optLayers[i]+"Accumulated", time);
		}

	}

    /*
	@brief Load the atmospheric data from binary files
	*/
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

					if (windULayer == 0)
					{
						double val = 0.;
						if (params->isValued("windU"))
							val = params->getDouble("windU");
						addConstantLayer("windU", "data", "", val, domain->getSWCorner(), domain->getNECorner() - domain->getSWCorner());
					}
					if (windVLayer == 0)
					{
						double val = 0.;
						if (params->isValued("windV"))
							val = params->getDouble("windV");
						addConstantLayer("windV", "data", "", val, domain->getSWCorner(), domain->getNECorner() - domain->getSWCorner());
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
					addConstantLayer("altitude", "data", "", val, domain->getSWCorner(), domain->getNECorner() - domain->getSWCorner());
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
					addConstantLayer(neededProperties.back(), "data", "", val, domain->getSWCorner(), domain->getNECorner() - domain->getSWCorner());
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
			addConstantLayer("altitude", "data", "", val, domain->getSWCorner(), domain->getNECorner() - domain->getSWCorner());
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
					dval = 0.0;
				//	cout<<"FUEL: " << vals[pos] <<" parameter " << paramNames[pos] <<" index " + vals[0]<<endl;
				}
				currentMap[paramNames[pos]] = dval;
			}
			table.push_back(std::move(currentMap));
		}
	}

	void DataBroker::readTableFromAsciiFile(std::string filename,
											std::vector<std::map<std::string, double>> &table)
	{
		std::ifstream file(filename);
		if (!file)
		{
			std::cout << "WARNING: could not load file for fuel properties " << filename << std::endl;
			return;
		}

		// Read the entire file content into a string.
		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string data = buffer.str();
		file.close();

		// Call the existing function to parse the table.
		readTableFromString(data, table);
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

	vector<string> DataBroker::getAllLayerNames()
	{
		vector<string> names;

		// Iterate over regular layers and add their names.
		for (const auto &entry : layersMap)
		{
				names.push_back(entry.first);
		}

		// Iterate over flux layers and add their names.
		
		return names;
	}

	DataLayer<double> *DataBroker::getLayer(const string &property)
	{
		// Scanning the scalar layers
		
		ilayer = layersMap.find(property);
		if (ilayer != layersMap.end()){
			//cout << "found lmayer  "<<ilayer->second->getKey()<<endl;
			return ilayer->second;
		}
			

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
		{

			return flayer->second;
		}

		return 0;
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

	int DataBroker::getMoisturesProperties(FireNode *fn, PropagationModel *model, int keynum)
	{   //
		double m_ones = 0.032;
		double m_liveh =  0.7;
		double m_tens = 0.04;
		double m_livew = 1;
		double m_hundreds = 0.06;
		
		if (params->isValued("moistures.ones"))
			m_ones = params->getDouble("moistures.ones");
	
		if (params->isValued("moistures.liveh"))
			m_liveh = params->getDouble("moistures.liveh");
		
		if (params->isValued("moistures.tens"))
			m_tens = params->getDouble("moistures.tens");
		
		if (params->isValued("moistures.livew"))
			m_livew = params->getDouble("moistures.livew");
		
		if (params->isValued("moistures.hundreds"))
			m_hundreds = params->getDouble("moistures.hundreds");

		(model->properties)[keynum] = m_ones;
		(model->properties)[keynum+1] = m_liveh; 
		(model->properties)[keynum+2] = m_tens;
		(model->properties)[keynum+3] = m_livew;
		(model->properties)[keynum+4] = m_hundreds;
		return 5;
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

	void DataBroker::addConstantLayer(const string &name, const string &layertype, const string &modelName, double value,
									  const FFPoint &SWOrigin, const FFPoint &shapeExtends)
	{
		// Set time origin to 0 and Lt to infinity.
		double timeOrigin = 0.0;
		double Lt = std::numeric_limits<double>::infinity();

		// Define grid dimensions.
		// These are placeholder values; replace with your actual grid resolution.
		size_t nx = 1;
		size_t ny = 1;
		size_t nz = 1;
		size_t nt = 1;
		size_t totalElements = nx * ny * nz * nt;
		FFPoint spatialExtent = FFPoint(shapeExtends.x, shapeExtends.y, shapeExtends.z);
		FFPoint SWCorner = FFPoint(SWOrigin.x, SWOrigin.y, SWOrigin.z);

		if (layertype == "data")
		{
			// Create an array filled with the constant value.
			double *data = new double[totalElements];
			for (size_t i = 0; i < totalElements; ++i)
			{
				data[i] = value;
			}
			// Construct the data layer using the constant field.
			XYZTDataLayer<double> *layer = new XYZTDataLayer<double>(name, SWCorner, timeOrigin, spatialExtent, Lt,
																	 nx, ny, nz, nt, data);

			// delete[] data;
			registerLayer(name, layer);
		}
		else if (layertype == "flux")
		{

			size_t mindex = domain->getFreeFluxModelIndex();

			FluxModel *newFluxmodel = domain->fluxModelInstanciation(mindex, modelName);

			int *data = new int[totalElements];
			for (size_t i = 0; i < totalElements; ++i)
			{
				data[i] = (int)mindex;
			}
			// Construct the flux layer.
			FluxLayer<double> *layer = new FluxLayer<double>(name,
															 atmoSWCorner, atmoNECorner, atmosphericNx, atmosphericNy, domain->getCells(),
															 data, SWCorner, timeOrigin, spatialExtent, Lt, nx, ny, nz, nt);
			// delete[] data;
			registerFluxModel(newFluxmodel);

			registerFluxLayer(name, layer);
		}
		else if (layertype == "BRatio" or layertype == "Bratio" or layertype == "bratio")
		{
			/* Instantiating a burning ratio layer */
			BurningRatioLayer<double> *brlayer =
				new BurningRatioLayer<double>(name, atmoSWCorner, atmoNECorner, atmosphericNx, atmosphericNy, domain->getCells());
			registerLayer(name, brlayer);
		}
		else
		{
			std::cout << "addConstantLayer: Unknown layer type " << layertype << std::endl;
		}
	}

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

	PropagativeLayer<double> *DataBroker::constructPropagativeLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
																	double timeOrigin, double Lt, int dimTSelected /* = -1 */)
	{
		cout << "DataBroker::constructPropagativeLayerFromFile " << " from netCDF Not Implemented" << endl;
		return NULL;
	}

	FuelDataLayer<double> *DataBroker::constructFuelLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
														  double timeOrigin, double Lt, int dimTSelected)
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
			FuelDataLayer<double> *newlayer = new FuelDataLayer<double>(property, SWCorner, timeOrigin, spatialExtent, Lt, nx, ny, nz, nt, fuelMap);
			delete[] fuelMap;
			return newlayer;
		}
	}

	FluxLayer<double> *DataBroker::constructFluxLayer(NcVar &values, FFPoint &SWCorner, FFPoint &spatialExtent,
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
