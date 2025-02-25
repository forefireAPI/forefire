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

#ifndef DATABROKER_H_
#define DATABROKER_H_

#include "DataLayer.h"
#include "GradientDataLayer.h"
#include "TimeGradientDataLayer.h"
#include "NCXYZTDataLayer.h"
#include "FuelDataLayer.h"
#include "ArrayDataLayer.h"
#include "BurningRatioLayer.h"
#include "TwoTimeArrayLayer.h"
#include "PropagativeLayer.h"
#include "FluxLayer.h"
#include "FFArrays.h"
#include "AtmosphericData.h"
#include "ParallelData.h"
#include "SimulationParameters.h"
#include <netcdf>
using namespace netCDF;
using namespace netCDF::exceptions;

using namespace std;

namespace libforefire {

class FireDomain;
class FluxModel;
template<typename T> class MultiplicativeLayer;

/*! \class DataBroker
 * \brief Data Broker for a fire simulation
 *
 *  DataBroker implements a data management tool tailored for FireDomain.
 *  Several layers of available data are stored and methods to communicate
 *  the desired values to the FireDomain are defined. The FireDomain has
 *  first registered the properties needed by the propagation model and
 *  DataBroker is responsible for accessing these values at given location
 *  and time. DataBroker also manages the storing of atmospheric properties
 *  (for coupled fire/atmopshere simulations) and parallel-related arrays
 *  (for parallel simulations).
 */
class DataBroker {

	// Information concerning the fire model
	FireDomain* domain; /*!< pointers to the domain */
	static SimulationParameters* params;

	// Information concerning the atmospheric model
	FFPoint atmoSWCorner, atmoNECorner; /*!< spatial limit of the domain */
	size_t atmosphericNx, atmosphericNy; /*!< size of the atmospheric matrices */
	AtmosphericData* atmosphericData; /*!< pointer to the atmospheric-related data */

	// Information concerning the embedded parallelism
	ParallelData* parallelData; /*!< pointer to the parallel-related data */

	// map of the data layers
	map<string, DataLayer<double>* > layersMap; /*!< map of the different scalar layers stored in the data broker */
	map<string, DataLayer<double>* >::iterator ilayer; /*!< iterator of the list of layers */
	map<string, FluxLayer<double>* > fluxLayersMap; /*!< map of the different flux layers stored in the data broker */
	map<string, FluxLayer<double>* >::iterator flayer; /*!< iterator of the list of layers */

	// List of the data layers
	list<DataLayer<double>* > layers; /*!< list of the different layers stored in the data broker */
	list<FluxLayer<double>* > fluxLayers; /*!< list of the different flux layers stored in the data broker */

	// Pre-defined layers
	static DataLayer<double>* fuelLayer; /*!< predefined layer for fuel parameters */
	static DataLayer<double>* dummyLayer; /*!< predefined layer for a dummy variable (optimization) */
	static DataLayer<double>* altitudeLayer; /*!< predefined layer for the altitude (optimization) */

	static DataLayer<double>* forcedArrivalTimeLayer;
	static DataLayer<double>* slopeLayer; /*!< predefined layer for the slope (optimization) */
	static DataLayer<double>* moistureLayer; /*!< predefined layer for moisture (optimization) */
	static DataLayer<double>* temperatureLayer; /*!< predefined layer for temperature (optimization) */
	static DataLayer<double>* windULayer; /*!< predefined layer for longitudinal wind (optimization) */
	static DataLayer<double>* windVLayer; /*!< predefined layer for lateral wind (optimization) */



	vector<map<string, double> > fuelPropertiesTable; /*!< values of the properties of the fuels */
	void extractFuelProperties(vector<map<string, double> >, ForeFireModel*);

	/* Handling of the propagation data brokers */
	typedef int (*propPropGetter)(FireNode*, PropagationModel*, int);
	typedef map<string, propPropGetter> propGetterMap;
	static propGetterMap makePGmap(){
		// Construction of the map from strings to desired property computation
		propGetterMap pgM;
		pgM["fieldSpeed"] = &getDummy;
		pgM["altitude"] = &getAltitude;
		pgM["arrival_time_gradient"] = &getArrival_time_gradient;
		pgM["slope"] = &getSlope;
		pgM["windU"] = &getWindU;
		pgM["windV"] = &getWindV;
		pgM["normalWind"] = &getNormalWind;
		pgM["fuel"] = &getFuelProperties;
		pgM["moisture"] = &getMoisture;
		pgM["temperature"] = &getTemperature;
		pgM["frontDepth"] = &getFrontDepth;
		pgM["frontCurvature"] = &getFrontCurvature;
		pgM["frontFastestInSection"] = &getFrontFastestInSection;
		pgM["nodeLocationX"] = &getFirenodeLocX;
		pgM["nodeLocationY"] = &getFirenodeLocY;
		pgM["nodeID"] = &getFirenodeID,
		pgM["nodeState"] = &getFirenodeState,
		pgM["nodeTime"] = &getFirenodeTime;
		return pgM;
	}
	static const propGetterMap propPropertiesGetters; /*!< map to predefined functors to compute properties */
	vector<vector<propPropGetter> > propDataGetters; /*!< vector of functors to compute properties for propagation models (optimization) */
	vector<size_t> numPropDataGetters; /*!< vector of the number of functors for propagation models (optimization) */


	/* Handling of the propagation data brokers */
	typedef int (*fluxPropGetter)(FFPoint, const double&, FluxModel*, int);
	typedef map<string, fluxPropGetter> fluxGetterMap;
	static fluxGetterMap makeFGmap(){
		// Construction of the map from strings to desired property computation
		fluxGetterMap pgM;
		pgM["fuel"] = &getFuelProperties;
		pgM["moisture"] = &getMoisture;
		pgM["altitude"] = &getAltitude;
		pgM["windU"] = &getWindU;
		pgM["windV"] = &getWindV;
		return pgM;
	}
	static const fluxGetterMap fluxPropertiesGetters; /*!< map to predefined functors to compute properties */
	vector<vector<fluxPropGetter> > fluxDataGetters; /*!< vector of functors to compute properties for flux models (optimization) */
	vector<size_t> numFluxDataGetters; /*!< vector of the number of functors for flux models (optimization) */

	/* list of all the properties needed by the models in use */
	vector<string> neededProperties;
	void dropProperty(string);

	/*! \brief getting the desired property for given firenode */
	double getProperty(FireNode*, string);

	/*! \brief getting the desired property for given location */
	double getProperty(FFPoint, string);

	/* Pre-defined function for propagation models */

	/*! \brief predefined function for getting the value of a dummy variable for given firenode */
	static int getDummy(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the fuel parameters at firenode location */
	static int getFuelProperties(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the moisture at firenode location */
	static int getMoisture(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the temperature at firenode location */
	static int getTemperature(FireNode*, PropagationModel*, int);

	/*! \brief predefined function for getting the altitude for given firenode */
	static int getAltitude(FireNode*, PropagationModel*, int);

    static int getFirenodeLocX(FireNode *fn, PropagationModel *model, int keynum);

    static int getFirenodeLocY(FireNode *fn, PropagationModel *model, int keynum);

    static int getFirenodeID(FireNode *fn, PropagationModel *model, int keynum);

    static int getFirenodeTime(FireNode *fn, PropagationModel *model, int keynum);
    static int getFirenodeState(FireNode *fn, PropagationModel *model, int keynum);

    static int getArrival_time_gradient(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the slope for given firenode */
	static int getSlope(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the longitudinal wind for given firenode */
	static int getWindU(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the transverse wind for given firenode */
	static int getWindV(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the normal wind for given firenode */
	static int getNormalWind(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the front depth */
	static int getFrontDepth(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the front curvature */
	static int getFrontCurvature(FireNode*, PropagationModel*, int);
	/*! \brief predefined function for getting the front fastest speed nearby in the section */
	static int getFrontFastestInSection(FireNode*, PropagationModel*, int);

	/* Pre-defined function for flux models */

	/*! \brief predefined function for getting the fuel parameters at given location and time */
	static int getFuelProperties(FFPoint, const double&, FluxModel*, int);
	/*! \brief predefined function for getting the moisture at given location and time */
	static int getMoisture(FFPoint, const double&, FluxModel*, int);
	/*! \brief predefined function for getting the altitude for given location and time */
	static int getAltitude(FFPoint, const double&, FluxModel*, int);
	/*! \brief predefined function for getting the longitudinal wind for given location and time */
	static int getWindU(FFPoint, const double&, FluxModel*, int);
	/*! \brief predefined function for getting the transverse wind for given location and time */
	static int getWindV(FFPoint, const double&, FluxModel*, int);

	/*! \brief common initialization (for all constructors) */
	void commonInitialization();



	/*! \brief testing inclusion of a variable domain inside the fire domain */
	bool isRelevantData(FFPoint&, FFPoint&);


	/*! \brief loading a NCXYZTDataLayer from an NcFile */
	XYZTDataLayer<double>* constructXYZTLayer(NcVar&, FFPoint&, FFPoint&,double , double , int );
	/*! \brief loading a FuelDataLayer from an NcFile */
	FuelDataLayer<double>* constructFuelLayer(NcVar&, FFPoint&, FFPoint&,double , double , int );
	/*! \brief loading a PropagativeLayer from an NcFile */
	PropagativeLayer<double>* constructPropagativeLayer(NcVar&, FFPoint&, FFPoint&,double , double , int );
	/*! \brief loading a FluxLayer from an NcFile */
	FluxLayer<double>* constructFluxLayer(NcVar&, FFPoint&, FFPoint&,double , double , int );
	/*! \transpose data from fortran netcdf*/
	double* readAndTransposeFortranProjectedField(NcVar* , const size_t& ,const size_t&  , const size_t& ,const size_t& ,bool  ,  int );
	int*    readAndTransposeIntFortranProjectedField(NcVar* , const size_t& ,const size_t&  , const size_t& ,const size_t&, bool ,  int );

public:

	// Pre-defined layers
	static FluxLayer<double>* heatFluxLayer; /*!< predefined layer for heat flux */


	static XYZTDataLayer<double>* PwindULayer; /*!< predefined layer for longitudinal wind (optimization) */
	static XYZTDataLayer<double>* PwindVLayer; /*!< predefined layer for lateral wind (optimization) */

	/*! \brief default constructor */
	DataBroker(FireDomain* = 0);
	/*! \brief destructor */
	virtual ~DataBroker();

	/*! \brief registering a propagation model */
	void registerPropagationModel(PropagationModel*);

	void updateFuelValues(PropagationModel*, string key, double value );

	/*! \brief registering a flux model */
	void registerFluxModel(FluxModel*);

	/*! \brief registering a layer in the data broker */
	void registerLayer(string, DataLayer<double>*);

	/*! \brief registering a flux layer in the data broker */
	void registerFluxLayer(string, FluxLayer<double>*);

	/*! \brief loadFromNCFile function for NetCDF files */
	void loadFromNCFile(string);

	/*! \brief reading a table from an ascii file */
	void readTableFromAsciiFile(string, vector<map<string, double> >&);
	/*! \brief reading a table from an sttring variable  */

	void readTableFromString(const std::string&, vector<map<string, double> >&);

	/*! \brief splitting a string according to a delimiter */
	void tokenize(const string&, vector<string>&, const string&);

	/*! \brief initializing the atmospheric layers */
	void setAtmosphericDomain(const FFPoint&, const FFPoint&
			, const size_t&, const size_t&);
	void initializeAtmosphericLayers(const double&, const size_t&, const size_t&);

	/*! \brief initializing the propagative layer */
	//void initializePropagativeLayer(string);

	/*! \brief initializing the flux layers */
	//void initializeFluxLayers(string);

	/*! \brief initializing the parallel properties */
	void initializeParallelProperties(const size_t&, const size_t&
			, const size_t&, const double&);

	/*! \brief insuring the presence of all needed layers */
	void insureLayersExistence();

	/*! \brief initialize flux layers */
	void initFluxLayers(const double&);

	/*! \brief default constant layer constructor */
	void addConstantLayer(string, const double&);

	/*! \brief typical double layer constructor */
	void addLayer(string name, double &x0 , double &y0, double& t0, double& width , double& height, double& timespan, size_t& nnx, size_t& nny, size_t& nnz,  double* values);

	/*! \brief accessor to the desired data layer */
	DataLayer<double>* getLayer(const string&);
	
	bool hasLayer(const string& ) ;

 	XYZTDataLayer<double>* getwindULayer(); 
	/*! \brief accessor to the desired flux layer */
	FluxLayer<double>* getFluxLayer(const string&);

	/*! \brief recomputes flux actives surfaces for each model */
	void computeActiveSurfacesFlux(const double&);

	void loadMultiWindBin(double , size_t , size_t* , size_t* );
	/*! \brief accessor to the desired set of properties for propagation models */
	void getPropagationData(PropagationModel*, FireNode*);
	bool* optimizedPropDataBroker;

	/*! \brief accessor to the desired set of properties for flux models */
	void getFluxData(FluxModel*, FFPoint&, const double&);
	bool* optimizedFluxDataBroker;

	/*! \brief accessor to the data contained in the desired layer */
	void getMatrix(string, const FFPoint&, const FFPoint&
			, const double&, FFArray<double>**);

	string printLayers();
};

}

#endif /* DATABROKER_H_ */
