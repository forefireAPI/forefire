/**
 * @file FireDomain.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef FIREDOMAIN_H_
#define FIREDOMAIN_H_

#include "ForeFireAtom.h"
#include "Visitable.h"
#include "Visitor.h"
#include "FireNode.h"
#include "FireFront.h"
#include "FFVector.h"
#include "FDCell.h"
#include "FFEvent.h"
#include "TimeTable.h"
#include "SimulationParameters.h"
#include "DataBroker.h"
#include "PropagationModel.h"
#include "FluxModel.h"
#include "include/FFConstants.h"
#include "DataLayer.h"
#include "FluxLayer.h"
#include "FFArrays.h"
#include "FireNodeData.h"
#include "FireFrontData.h"
#include "ParallelException.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire{

/*! \class FireDomain
 * \brief Class describing the "world" of the simulation
 *
 *  FireDomain represents all the data and models
 *  provided to simulate the fire propagation. It
 *  embodies the 'FireFronts', the 'boundaries',
 *  the 'dataBroker' and physical models necessary
 *  for the simulation.
 */
class FireDomain: public ForeFireAtom, Visitable {


	/* Internal variables of the simulation */
    /*--------------------------------------*/
	SimulationParameters* params; /*!< parameters of the simulation */
	FireFront* domainFront; /*!< Container for the fireFront in the domain  */
	list<FireFront*>::iterator currentfront;
	FDCell** cells; /*!< Table of the FDCells matching the meteorological ones */

	FDCell* trashCell; /*!< cell containing all the firenodes outside the domain */
	TimeTable* schedule; /*!< timetable of the events taking place in the domain */
	DataBroker* dataBroker; /*!< data broker of the simulation */
	PropagativeLayer<double>* propagativeLayer; /*!< layer of the propagation models */
	bool safeTopologyMode; /*!< special mode for no topology handling */

	/* Mesh properties */
    /*-----------------*/
	FFPoint SWCorner; /*!< SouthWest Corner of the mesh */
	FFPoint NWCorner; /*!< NorthWest Corner of the mesh */
	FFPoint NECorner; /*!< NorthEast Corner of the mesh */
	FFPoint SECorner; /*!< SouthEast Corner of the mesh */


	/* reference properties */
    /*----------------------*/
	int refYear; /*!< reference year */
	int refDay; /*!< reference day (since 1st January; 01/01 == 1) */
	double refLatitude, refLongitude; /*!< reference latitude and longitude */
	double metersPerDegreeLat, metersPerDegreesLon; /*!< reference latitude to meter ratio and longitude to meter ratio*/

	int numIterationAtmoModel; /*! current number of the iteration if coupled to MNH */

	size_t atmoNX; /*!< number of atmo/FDCells in X direction */
	size_t atmoNY; /*!< number of atmo/FDCells in Y direction */
	size_t atmoNZ; /*!< number of cells in vertical direction in the atmospheric model */

	static const string altitude;

	/* Spatial parameters */
    /*--------------------*/
	double perimeterResolution; /*!< perimeter resolution of the fronts */
	double propagationSpeedAdjustmentFactor;/*!< adjustment factor for propagation speed */
	double spatialCFL; /*!< spatial CFL-like coefficient */
	double spatialCFLMax; /*!< maximum spatial CFL admitted */
	double spatialIncrement; /*!< spatial increment of each firenode */
	double dtMax; /*!< maximum time-step allowed for firenodes */

	/* Burning matrices and front depth computation related objects */
    /*--------------------------------------------------------------*/

	size_t localBMapSizeX, localBMapSizeY; /*! sizes of the local burning matrices */
	size_t globalBMapSizeX; /*! Size in X direction of the global burning matrix */
	size_t globalBMapSizeY; /*! Size in Y direction of the global burning matrix */

	double burningMatrixResX; /*! Resolution in X direction of the global burning matrix */
	double burningMatrixResY; /*! Resolution in Y direction of the global burning matrix */
	double burningMatrixRes; /*! global burning matrix resolution */

	double inverseCellSizeX; /*! Resolution in X direction of the global burning matrix */
	double inverseCellSizeY; /*! Resolution in Y direction of the global burning matrix */

	enum FrontDepthScheme {
		normalDir = 0,
		closest = 1
	} ;
	static FrontDepthScheme fdScheme; /*!< front depth scheme scheme */
	void setFrontDepthScheme(string);

	double burningTresholdFlux;
	double maxFrontDepth;
	double BMapsResolution;

	/* search algorithms */
    /*-------------------*/

	/*! \brief vector of the firenodes from within a given distance of a given firenode */
	vector<FireNode*> closeNodes;
	vector<double> distances;
	size_t min_position;
	void getPotentialMergingNodes(FireNode*, const double&);

	/* Trash related objects */
    /*-----------------------*/

	/*! \brief trash lists for the atoms */
	static list<FireNode*> createdNodes;
	static list<FireNode*> trashNodes;
	static list<FireFront*> trashFronts;

	/* Factories of models */
    /*---------------------*/

	/*! \brief factory for the propagation models */
	typedef PropagationModel* (*PropagationModelInstantiator)(const int&, DataBroker*);
	typedef map<string, PropagationModelInstantiator> PropModelMap;
	static PropModelMap& prop_instantiatorMap();

	/*! \brief factory for the flux models */
	typedef FluxModel* (*FluxModelInstantiator)(const int&, DataBroker*);
	typedef map<string, FluxModelInstantiator> FluxModelMap;
	static FluxModelMap& flux_instantiatorMap();

	/* backup of the simulation */
    /*--------------------------*/
	static FireFrontData* mainFrontBackup;

	/*---------------------------------------------------*/
	/* VARIABLES AND ALGORITHMS FOR PARALLEL SIMULATIONS */
	/*---------------------------------------------------*/


	/*! \brief markers for communication in parallel simulations */
	static const double endChain; /*!< Marker for the end of the chain */
	static const double endCom; /*!< Marker for the end of communication */
	static const double noCom; /*!< Marker for no communication (non-active halos) */

	/*! \brief list of link nodes */
	list<FireNode*> linkNodes;

	/*! \brief map of nodes that changed id during formIncomingStructures */
	map<double, double> matchedIds;

	struct Frontier{
	public:
		FFPoint pointA, pointB;
		Frontier(const FFPoint& pA, const FFPoint& pB) : pointA(pA), pointB(pB) {
		}
		~Frontier(){}
	};

	list<Frontier*> frontiers;
	list<Frontier*> infrontiers;
	/*! \brief list of firenodes that has just changed id */
	list<FireNode*> recentlyIDChangedNodes;

	/*! \brief list of firefronts impacted by the changes during the parallel process */
	list<FireFront*> impactedFronts;

	/*! \brief string flux for debugging purpose */
	ostringstream debugOutput;

 


	/*! \brief obtaining the chain following number of appearance */
	void getChainAt(const list<FireNodeData*>&, const int&
			, list<FireNodeData*>*);

	/*! \brief removes and returns the chain following number of appearance */
	list<FireNodeData*>* popChainAt(list<FireNodeData*>&, const int&);


	/*! \brief append chain to a given list */
	void appendChain(list<FireNodeData*>&
			, const list<FireNodeData*>&);

	/*! \brief merging two chains of halofirenodes data */
	list<FireNodeData*>* mergeChains(list<FireNodeData*>&
			, list<FireNodeData*>&);
	void pushBackInChain(list<FireNodeData*>*, FireNodeData*);
	void pushFrontInChain(list<FireNodeData*>*, FireNodeData*);

	/* Functions for parallel handling */
	bool idInChain(const double&, const list<FireNodeData*>&);
	bool isPrevLinkDataInList(const double&, const list<FireNodeData*>&);
	bool isNextLinkDataInList(const double&, const list<FireNodeData*>&);
	
	FireNode* findRelatedPreviousLink(list<FireNode*>, FFPoint);
	double distanceAlongFrontier(FFPoint&, FFPoint&);
	double distanceOnFrontier(int&, FFPoint, FFPoint);
	double distanceOnBoundingBox(int&, FFPoint, FFPoint);


	/*! \brief deleting link nodes */
	void deleteLinkNodes(list<FireNode*>&);

	
	/*! \brief table for the possible corner nodes */
	FFPoint& getStartCornerFromIndex(const int&);
	FFPoint getBoundingBoxCornerFromIndex(const int&, FFPoint&, FFPoint&);
	/*! \brief finding the frontier for a given point */
	int getFrontierIndex(FFPoint loc);
	

	/*! \brief finding closest node from a location within a given set of nodes */
	FireNode* findClosestNodeInList(FFPoint&, const list<FireNode*>&);
	FireNode* findClosestNodeInList(FireNodeData*, const list<FireNode*>&);

	/*! \brief finding closest node from a location within a given set of nodes, with a cutoff */
	FireNode* findClosestWithinNodeInList(FFPoint&, const double&, const list<FireNode*>&);
	FireNode* findClosestWithinNodeInList(FireNodeData*, const double&, const list<FireNode*>&);


	FireNode* findExistingNodeNear(FireNodeData*);

	/*-----------------*/
	/* OTHER FUNCTIONS */
	/*-----------------*/

	/*! \brief Common initialization for all the different types of constructors */
	void commonInitialization(double*, double*
			, const int&, const int&);

	/*! \brief boolean for checing the arrival time in the burning matrix */
	bool burnCheck(const size_t&, const size_t&, const double&);

	/*! \brief Storing information in the burning matrix */
	void setArrivalTime(const size_t&, const size_t&, const double&);

	/*! \brief Trashing all the fronts of the simulation */
	void trashFrontsAndNodes();

	/*! \brief getting the number of the day since the 1st of January */
	int getDayNumber(const int& = 2012, const int& = 1, const int& = 1);

public:

	
	struct distributedDomainInfo{
		size_t ID;
		size_t atmoNX;
		size_t atmoNY;
		size_t refNX;
		size_t refNY;
		FFPoint* SWCorner;
		FFPoint* NECorner;
		double lastTime;
	};

	// Define the CellData structure
	struct CellData {
		size_t localx;
		size_t localy;
		size_t nx;
		size_t ny;
		size_t nz;
		size_t nt;
		std::vector<double> data; // Assuming T is double
	};

	// Define the DistributedDomainBCellList structure
	struct DistributedDomainBCellList {
		size_t ID;
		size_t numActiveCells;
		std::vector<CellData> cells;
	};

	// Alias for a list of DistributedDomainBCellList
	using ListOfDistributedDomainBCellList = std::vector<DistributedDomainBCellList>;


	static std::list<distributedDomainInfo*> parallelDispatchDomains;

	/* Propagation models */
	static const size_t NUM_MAX_PROPMODELS = 50; /*!< maximum number of propagation models */
	static PropagationModel* propModelsTable[NUM_MAX_PROPMODELS];
	static int registerPropagationModelInstantiator(string, PropagationModelInstantiator);
	void updateFuelTable( string , double );
	PropagationModel* propModelInstanciation(const int&, string);

	void registerPropagationModel(const int&, PropagationModel*);
	bool addPropagativeLayer(string);
	size_t getFreePropModelIndex();

	/* Mesh properties */
    /*-----------------*/
	FFPoint SWLngLat; /*!< SouthWest Corner of the mesh */
	FFPoint NELngLat; /*!< NorthEast Corner of the mesh */
	/* Flux models */
	static const size_t NUM_MAX_FLUXMODELS = 500; /*!< maximum number of flux models */
	static FluxModel* fluxModelsTable[NUM_MAX_FLUXMODELS];
	static int registerFluxModelInstantiator(string, FluxModelInstantiator);
	FluxModel* fluxModelInstanciation(const int&, string);
	void registerFluxModel(const int&, FluxModel*);
	bool addFluxLayer(string);
	bool addLayer(string , string ,string);
	bool addScalarLayer(string lname, string type, double &x0, double &y0, double& t0, double& width, double& height, double& timespan, size_t& nnx,	size_t& nny, size_t& nnz, size_t& nnk, double* values);
	bool addIndexLayer(string lname, string type, double &x0, double &y0, double& t0, double& width, double& height, double& timespan, size_t& nnx,	size_t& nny, size_t& nnz, size_t& nnk, int* values);

	size_t getFreeFluxModelIndex();

    // Getter for the reference latitude.
    double getRefLatitude()  ;

    // Getter for the reference longitude.
    double getRefLongitude()  ;

    // Getter for the latitude conversion factor.
    double getMetersPerDegreeLat() ;

    // Getter for the longitude conversion factor.
    double getMetersPerDegreesLon() ;

	double getXFromLon(double) ;
	double getYFromLat(double) ;
	double getLonFromX(double) ;
	double getLatFromY(double) ;
	vector<double> getActiveBBoxLBRT();

	static bool commandOutputs; /*! boolean for command outputs */
	static bool outputs; /*! boolean for outputs */
	static bool recycleNodes; // to recycle nodes in memory
	static bool recycleFronts; //to recycle fronts in memory
	static size_t atmoIterNumber;


	bool atmosphericCoupling; /*! boolean for coupled simulations */
	bool parallel; /*! boolean for parallel simulations */
	bool isFireActive; /*! boolean if some has been read*/
	/*! \brief Default constructor */
	FireDomain();
	/*! \brief Constructor with boundaries and time */
	FireDomain(const double&, FFPoint&, FFPoint&);
	/*! \brief Constructor with boundaries, time and reference longitude and latitude */
	FireDomain(const int&
			, const int&, const int&
			, const int&, const double&
			, const double&, const double&
			, const int&, const double*
			, const int&, const double*
			, const int&, const double&);
	/*! \brief Destructor */
	virtual ~FireDomain();

	/*! \brief present time of the simulation */
	double getSimulationTime();
    
	/*! \brief Storing information of the simulation */
	void backupState();

	/*! \brief Returning to a valid state of simulation */
	void restoreValidState();

	/*! \brief Accessors */
	int getReferenceYear();
	int getReferenceDay();
	double getSecondsFromReferenceTime(const int&, const int&, const int&, const int&);
	list<FireFront*> getMainFronts();
	FireFront* getDomainFront();
	TimeTable* getTimeTable();
	DataBroker* getDataBroker();
	FDCell* getCell(const int&, const int&);
	FDCell* getCell(FFPoint);
	FDCell* getCell(FireNodeData*);
	FDCell* getCell(const double&, const double&);
	FDCell** getCells();
	list<FDCell*> getProxCells(FDCell*, int = 1);
	double& getPerimeterResolution();
	double& getSpatialIncrement();
	double& getMaxTimeStep();
	double getArrivalTime(FFPoint&);
	double getArrivalTime(const size_t&, const size_t&);
	double getMaxSpeed(FFPoint&);
	double getMaxSpeed(const size_t&, const size_t&);
	
	FFPoint& getSWCorner();
	FFPoint& getNECorner();
	int getNumIterationAtmoModel();
	void increaseNumIterationAtmoModel();
	PropagativeLayer<double>* getPropagativeLayer();
	/* Shortcuts to simulation parameters */
	double& SWCornerX();
	double& SWCornerY();
	double& NWCornerX();
	double& NWCornerY();
	double& NECornerX();
	double& NECornerY();
	double& SECornerX();
	double& SECornerY();
	void readMultiDomainMetadata();
	void pushMultiDomainMetadataInList(size_t id, double lastTime, size_t atmoNX, size_t atmoNY, double nswx, double nswy, double nnex, double nney);

	/*! \brief cell containing a firenode */
	FDCell* getCell(FireNode*);

	/*! \brief validating the topology for the nodes in the simulation */
	void validateTopology(string);

	/*! \brief Setting the safe mode for topology */
	void setSafeTopologyMode(bool);

	/*! \brief give Max com nodes */
	int getMaxComNodes();


	/*! \brief Mutators */
	void setBoundariesFront(FireFront*);
	void setTimeTable(TimeTable*);
	void setPropagativeLayer(PropagativeLayer<double>*);

	/*! input function (overloads 'input()' from 'ForeFireAtom') */
	void input();

	/*! updates the 'FireDomain'
	 *  (overloads 'update()' from 'ForeFireAtom') */
	void update();

	/*! computes the next FireDomain properties
	 *  (overloads 'timeAdvance()' from 'ForeFireAtom') */
	void timeAdvance();

	/*! Output function */
	void output();

	/*! \brief inserting a newly created atom in the simulation */
	void addNewAtomToSimulation(ForeFireAtom*);

	/*! \brief deleting an atom in the simulation */
	void deleteAtomOfSimulation(ForeFireAtom*);

	/*! \brief getting the layer associated with a property */
	DataLayer<double>* getDataLayer(const string&);

	/*! \brief getting a specified flux layer */
	FluxLayer<double>* getFluxLayer(const string&);

	/*! \brief searching a firenode by ID */
	FireNode* getFireNodeByID(const long&);
	FireNode* getFireNodeByID(const double);
	FireNode* getFireNodeByIDNeighborCells(const double, FDCell*, int = 1);

 

	/*! \brief visitor function */
	void accept(Visitor*);

	/*! \brief Test on the location */
	bool striclyWithinDomain(FFPoint&);
	bool striclyWithinDomain(const double&, const double&);
	bool striclyWithinDomain(FireNode*);
	bool striclyWithinDomain(FireNodeData*);
	bool looselyWithinDomain(FFPoint&);
	bool looselyWithinDomain(const double&, const double&);
	bool looselyWithinDomain(FireNodeData*);
	bool withinPhysicalDomain(FFPoint&);

	/*! \brief Test to see if the firenode is in one of the halos */
	bool isInInnerHalo(FireNode*);
	bool isInInnerHalo(const FFPoint&);
	bool isInOuterHalo(FDCell*);
	bool isInOuterHalo(FireNode*);
	bool isInOuterHalo(FFPoint&);
	bool isInOuterHalo(const double&, const double&);
	/*! \brief Test to see if a firenode pertains to a list */
	bool firenodeInList(FireNode*, const list<FireNode*>&);

	/*! \brief Test to see if a firenode data pertains to a list */
	FireNodeData* idInList(const double&, const list<FireNodeData*>&);

	/*! \brief Test to see if a position is in a list */
	FireNodeData* posInList(const double&, const double&
			, const list<FireNodeData*>&);

	/*! \brief finding the intersection between a segment and the frontier */
	static const FFPoint outPoint;
	FFPoint findIntersectionWithFrontiers(FFPoint&, FFPoint&);
	FFPoint findIntersectionWithFrontiers(FireNodeData*, FireNodeData*);
	FFPoint findIntersectionWithInnerFrontiers(FFPoint&, FFPoint&);
	FFPoint findIntersectionWithInnerFrontiers(FireNodeData*, FireNodeData*);

	/*! \brief finding the intersection between two segments */
	FFPoint findIntersection(FFPoint&, FFPoint&, FFPoint&, FFPoint&);

	/*! \brief finding the intersection with a bounding box */
	FFPoint findIntersectionWithBoundingBox(FFPoint&, FFPoint&
			, FFPoint&, FFPoint&, int &);

	/*! \brief checking if a location is burning */
	bool isBurning(FFPoint&, const double&);

	/*! \brief Computing the front depth a given firenode */
	double computeFrontDepth(FireNode*);

	/*! \brief Computing the propagation speed of a given firenode */
	double getPropagationSpeed(FireNode*);

    PropagationModel* getPropagationModel(const std::string& key);

    /*! \brief Computing the propagation speed of a given firenode */
	double getModelValueAt(int&, FFPoint&, const double&, const double&, const double&);

	/*! \brief recycle/create a firenode */
	void addFireNodeInCell(FireNode*);

	/*! \brief finds the related cell and remove a given firenode to the list */
	void removeFireNodeInCell(FireNode*);

	/*! \brief update the position of a given firenode in the cells */
	void updateFireNodeInCells(FireNode*);

	/*! \brief callback function from a firenode after it moved */
	void hasMoved(FireNode*, FFPoint, double);

	/*! \brief callback function from a firenode to check the topology */
	void checkTopology(FireNode*);

	/*! \brief handling the merges in the domain */
	void merge(FireNode*, FireNode*);

	/*! \brief relating two link nodes with respect to the frontiers */
	void relateLinkNodes(FireNode*, FireNode*);

	/*! \brief managing the trash of the simulation */
	void addToTrashNodes(FireNode*);
	void stopOutgoingNode(FireNode*, FFPoint&, double&);
	void addToTrashFronts(FireFront*);
	void dumpCellsInBinary();
	size_t countActiveCellsInDispatchDomain(size_t ) ;
    distributedDomainInfo *getParallelDomainInfo(size_t forID);
    void loadWindDataInBinary(double);
    void loadCellsInBinary();
	/*! \brief creating new atoms in the domain */
	FireNode* FireNodeFactory();
	FireFront* FireFrontFactory();
	FireNode* addFireNode(FFPoint&, FFVector&, double
			, double = 0., double = 0.
			, FireFront* = 0, FireNode* = 0
			, int = 0, int = 0
			, FireNode::State = FireNode::init);
	FireNode* addFireNode(FireNodeData*, FireFront* = 0, FireNode* = 0);
	FireNode* addLinkNode(FFPoint&, FireFront* = 0, FireNode* = 0);
	FireFront* addFireFront(double, FireFront* = 0);
	FFEvent* addNewEvent(ForeFireAtom*, double, string = "all");

	/*! \brief listing all the firenodes close to a given one */
	list<FireNode*> getNodesWithin(FireNode*, const double&, bool = true);
	list<FireNode*> getNodesWithin(FireNodeData*, const double&, bool = true);

	/*! \brief listing all the firenodes, in the physical domain, close to a given one */
	list<FireNode*> getPhysicalDomainNodesWithin(FFPoint&, const double&, bool = true);
	list<FireNode*> getPhysicalDomainNodesWithin(FireNodeData*, const double&, bool = true);

	/*! \brief saving the simulation */
	void saveArrivalTimeNC();
	/*! \brief loading the simulation */
	void loadArrivalTimeNC(string);

	/*! \brief dumping the map of arrival times for debugging */
	void dumpBurningMatrixAsBinary();

	string toString();
	string getFluxModelName(int );
	string printMainFronts();
	void visualizeBurningMatrixAroundNode(FireNode*);
	int getNumFN();
	int getNumFF();
	size_t getlocalBMapSize();

	double getSimulationMaxResolution(double&, double&, const size_t&);
	double getBurningMapResolution(double&, double);

	/*! \brief defining a bounding box containing three fire nodes */
	void computeBoundingBox(FireNode*, FireNode*, FireNode*, FFPoint&, FFPoint&);

	/*! \brief defining an optimized polygon around a firenode and its neighbors */
	void constructLocalSurroundingPolygon(FireNode*, FFPoint&, FFPoint&
			, vector<double>&, vector<double>&);

	/*! \brief local scanning around a firenode for burning matrix */
	void firenodeBurningScan(FireNode*);

	/*! \brief initial burning scan */
	void frontInitialBurningScan(const double&, FireFront*
			, const double&, const double&);

	/*! \brief global scanning for burning matrix during simulation */
	void frontBurningScan(FireFront*, const double&);

	/*! \brief scanning a region according to one polygon */
	void singlePolygonAreaBurningScan(FFPoint&, FFPoint&, double
			, bool, size_t&, double*, double*);

	/*! \brief scanning a region with respect to all fire fronts */
	void areaBurningScan(FFPoint&, FFPoint&, double);

    /*! \brief retrun a diag matrix */
	std::vector<std::vector<double>> getDataMatrix(const std::string& ) ;
	std::vector<std::vector<double>> getDataMatrix(const std::string& , FFPoint& , FFPoint& , size_t& , size_t& ) ;
	/*! \brief checking the burning status of a given location */
	bool checkForBurningStatus(FFPoint&);

};

}

#endif /* FIREDOMAIN_H_ */
