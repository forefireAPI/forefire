/**
 * @file FireDomain.cpp
 * @brief Implements the methods of the FireDomain class
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

 #include "FireDomain.h"
 #include "BurningMapLayer.h"
 
 #include <sys/stat.h>
 #include "RosLayer.h"
 
 namespace libforefire{
 //  On garde 1 domaine pour faire le parallele - domain 1
 //  
 //  
 //  
 //  Test - Domain 1 est celui qui a les fronts - il propage et il a des ghosts de sous domaines.
 //  Il faut qu'in conaisse la taille totale..Il lit ça dans le NC qui a la taille du domaine physique 
 // attention /etc/beegfs/beegfs-client.conf tuneRemoteFSync=true
 //  (pas de halos externes, faire attention  aux points de flux)
 //  Domain 1 va lire des fichiers de vents U/V a tous les updates
 //  Domain N va écrire des BMaps à tous les updates
 //  Recuperer les extents de tous les sous domaines... 
 //  On fait plus de chaines....
 
	 // Static variables
	 long ForeFireAtom::instanceNRCount = 0;
 
	 const double FireDomain::endChain = -1.;
	 const double FireDomain::endCom = -10.;
	 const double FireDomain::noCom = -100.;
 
 
	 const string FireDomain::altitude = "altitude";
	 const FFPoint FireDomain::outPoint
	 = FFPoint(numeric_limits<double>::infinity(), numeric_limits<double>::infinity(),0);
 
	 bool FireDomain::outputs = false;
	 bool FireDomain::commandOutputs = false;
	 bool FireDomain::recycleNodes = false;
	 bool FireDomain::recycleFronts = false;
	 
	 std::list<FireDomain::distributedDomainInfo*> FireDomain::parallelDispatchDomains;
	 std::list<FireNode*> FireDomain::createdNodes;
	 std::list<FireNode*> FireDomain::trashNodes;
	 std::list<FireFront*> FireDomain::trashFronts;
 
	 FireFrontData* FireDomain::mainFrontBackup;
	 size_t FireDomain::atmoIterNumber = 0;
	 FireDomain::FrontDepthScheme FireDomain::fdScheme = FireDomain::normalDir;
 
	 const size_t FireDomain::NUM_MAX_PROPMODELS;
	 const size_t FireDomain::NUM_MAX_FLUXMODELS;
 
	 PropagationModel* FireDomain::propModelsTable[FireDomain::NUM_MAX_PROPMODELS];
	 FluxModel* FireDomain::fluxModelsTable[FireDomain::NUM_MAX_FLUXMODELS];
 
	 FireDomain::PropModelMap& FireDomain::prop_instantiatorMap(){
		 static FireDomain::PropModelMap* inst = new FireDomain::PropModelMap;
		 return *inst;
	 }
 
	 FireDomain::FluxModelMap& FireDomain::flux_instantiatorMap(){
		 static FireDomain::FluxModelMap* inst = new FireDomain::FluxModelMap;
		 return *inst;
	 }
 
	 // Default constructor
	 FireDomain::FireDomain() : ForeFireAtom(0.) {
		 cout<<"Trying to instantiate an empty FireDomain, not relevant"<<endl;
	 }	
 
	 FireDomain::FireDomain(const double& t
							, FFPoint& swc, FFPoint& nec)
	 : ForeFireAtom(t), SWCorner(swc), NECorner(nec) {
		 isFireActive = false;
		 refLatitude = 0.;
		 refLongitude = 0.;
		 atmosphericCoupling = false;
		 propagationSpeedAdjustmentFactor =1;
		 //parallelDispatchDomains.size() = 0;
		 params = SimulationParameters::GetInstance();
		 numIterationAtmoModel = 0;
		 // Maximum time-step for Firenodes is not constrained
		 dtMax = numeric_limits<double>::infinity();
		 double readRes = 0;
 
 
		 if (params->getParameter("runmode") == "masterMNH"){
 
			  #ifdef MPI_COUPLING
			  #else
				 readMultiDomainMetadata();
			 #endif
			 cout<<"all inited, the big now with "<<parallelDispatchDomains.size()<<" total "<<parallelDispatchDomains.size()<<endl;
 
			 atmosphericCoupling = true;
	 
			 if(parallelDispatchDomains.size() >0){
 
			 
			 distributedDomainInfo* fit = parallelDispatchDomains.front();
			 double Mswx = fit->SWCorner->getX();
			 double Mswy = fit->SWCorner->getY();
			 double Mnex = fit->NECorner->getX();
			 double Mney = fit->NECorner->getY();
  
			 readRes = (Mnex-Mswx)/fit->atmoNX;
			 list<distributedDomainInfo*>::iterator it;
			 for (it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); it++)
				 {
				  
						 Mswx = std::min(Mswx,(*it)->SWCorner->getX());
						 Mswy = std::min(Mswy,(*it)->SWCorner->getY());
						 Mnex = std::max(Mnex,(*it)->NECorner->getX());
						 Mney = std::max(Mney,(*it)->NECorner->getY());
			  
			 }
			SWCorner.setLoc(Mswx,Mswy);
			NECorner.setLoc(Mnex,Mney); 

			atmoNX = (Mnex-Mswx)/readRes;
			atmoNY = (Mney-Mswy)/readRes;

			params->setSize("atmoNX",atmoNX);
			params->setSize("atmoNY",atmoNY);
			
 
			 for (it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); it++)
				 {
				  
						 (*it)->refNX = size_t(((*it)->SWCorner->getX()-Mswx)/readRes);
						 (*it)->refNY = size_t(((*it)->SWCorner->getY()-Mswy)/readRes);
					 //	cout<<(*it)->ID<<" HAVE A RESOLUTION OF "<< readRes <<" location "<<(*it)->refNX<<":"<<(*it)->refNY<<" coords:"<<(*it)->SWCorner->print() << ":"<<(*it)->NECorner->print()<<endl;
						 
			 }
					 
			 }else{
				 cout<<"ERROR big problem master mode but 0 domains read, rerun for domain file caching"<<endl;
				 atmoNX = params->getSize("atmoNX");
				 atmoNY = params->getSize("atmoNY");
	  
			 }
 
 
	 
		 }else{
			 params->setParameter("runmode", "standalone");
			 atmoNX = params->getSize("atmoNX");
			 atmoNY = params->getSize("atmoNY");
		 }
		 atmoNX = params->getSize("atmoNX");
		 atmoNY = params->getSize("atmoNY");
		 atmoNZ = params->getSize("atmoNZ");
		 // computing the cells' mesh
		 double cellsMeshX[atmoNX+1];
		 double cellsMeshY[atmoNY+1];
		 cellsMeshX[0] = SWCorner.getX();
		 cellsMeshY[0] = SWCorner.getY();
		 double dx, dy;
		 dx = (NECorner.getX()-SWCorner.getX())/atmoNX;
		 dy = (NECorner.getY()-SWCorner.getY())/atmoNY;
		 for ( size_t i = 1; i < atmoNX; i++ ) {
			 cellsMeshX[i] = cellsMeshX[i-1] + dx;
		 }
		 for ( size_t j = 1; j < atmoNY; j++ ) {
			 cellsMeshY[j] = cellsMeshY[j-1] + dy;
		 }
		 cellsMeshX[atmoNX] = NECorner.getX();
		 cellsMeshY[atmoNY] = NECorner.getY();
 
		 // simulation won't be run in parallel
		 params->setInt("parallel", 0);
 
		 // simulation won't be be run with mnh
		
 
		 // reference time
		 int year = params->getInt("refYear");
		 int yday = params->getInt("refDay");
		 
		 if (yday == 0)
		 {
			 year = params->getInt("year");
			 int month = params->getInt("month");
			 int day = params->getInt("day");
			 yday = getDayNumber(year, month, day);
		 }
		 
		 // Common initialization for all constructors
		 commonInitialization(cellsMeshX,cellsMeshY , year, yday);
 
	 }
 
	 FireDomain::FireDomain(const int& mpirank
							, const int& year, const int& month
							, const int& day, const double& t
							, const double& lat, const double& lon
							, const int& mdimx, const double* meshx
							, const int& mdimy, const double* meshy
							, const int& mdimz, const double& dt)
	 : ForeFireAtom(t), refLatitude(lat), refLongitude(lon) {
 
		 getNewID(mpirank);
		 isFireActive = false;
		 numIterationAtmoModel = 0;
		 params = SimulationParameters::GetInstance();
 
		 // Maximum time-step for Firenodes is constrained by the atmospheric model
		 dtMax = dt;
	 
		 /* simulation is run with mnh */
		 params->setParameter("runmode", "coupled");
		 params->setInt("parallel", 1);
		 params->setInt("mpirank", mpirank);
 
		 atmosphericCoupling = true;
 
		 /* Definition of the mesh */
		 atmoNX = (size_t) mdimx;
		 params->setSize("atmoNX", atmoNX);
		 atmoNY = (size_t) mdimy;
		 params->setSize("atmoNY", atmoNY);
		 atmoNZ = (size_t) mdimz;
		 params->setSize("atmoNZ", atmoNZ);
 
	 string ffOutputsPattern(params->getParameter("caseDirectory")+'/'+params->getParameter("fireOutputDirectory")+'/'+params->getParameter("outputFiles")+"."+params->getParameter("mpirank"));
	 SimulationParameters::GetInstance()->setParameter("ffOutputsPattern", ffOutputsPattern);
 
	 
 
		 // computing the position of the physical and numerical corners
		 SWCorner = FFPoint(meshx[0], meshy[0],0);
		 NECorner = FFPoint(2.*meshx[atmoNX-1]-meshx[atmoNX-2]
							, 2.*meshy[atmoNY-1]-meshy[atmoNY-2],0);
 
		 bool dumpDomainInfo = true;
		 #ifdef MPI_COUPLING
			 dumpDomainInfo = false;
		 #endif
		 string ffDomPattern(params->getParameter("caseDirectory")+'/'+params->getParameter("PPath")+'/'+params->getParameter("mpirank")+".domainData");
		 if (dumpDomainInfo){
			 ofstream FileOut(ffDomPattern.c_str(), ios_base::binary);
 
			 FileOut.write(reinterpret_cast<const char*>(&atmoNX), sizeof(size_t));
			 FileOut.write(reinterpret_cast<const char*>(&atmoNY), sizeof(size_t));
			 FileOut.write(reinterpret_cast<const char*>(&SWCorner.x), sizeof(double));
			 FileOut.write(reinterpret_cast<const char*>(&SWCorner.y), sizeof(double));
			 FileOut.write(reinterpret_cast<const char*>(&NECorner.x), sizeof(double));
			 FileOut.write(reinterpret_cast<const char*>(&NECorner.y), sizeof(double));
			 
			 FileOut.flush();   
			 FileOut.rdbuf()->pubsync(); 		
			 FileOut.close();
		 }
 
		 // computing the cells' mesh
		 double cellsMeshX[atmoNX+1];
		 double cellsMeshY[atmoNY+1];
		 for ( size_t i = 0; i < atmoNX; i++ ) {
			 cellsMeshX[i] = meshx[i];
		 }
		 cellsMeshX[atmoNX] = NECornerX();
		 for ( size_t j = 0; j < atmoNY; j++ ) {
			 cellsMeshY[j] = meshy[j];
		 }
		 cellsMeshY[atmoNY] = NECornerY();
 
		 int yday = getDayNumber(year, month, day);
		 
		 // Common initialization for all constructors
		 commonInitialization(cellsMeshX,cellsMeshY, year, yday);
 
	 }
 
	 FireDomain::~FireDomain() {
		// Deleting nodes
		while (!createdNodes.empty()) {
			if (createdNodes.back()) {
				delete createdNodes.back();
			}
			createdNodes.pop_back();
		}
	
		// Deleting fronts
		if (domainFront) {
			delete domainFront;
			domainFront = nullptr;
		}
		while (!trashFronts.empty()) {
			if (trashFronts.back()) {
				delete trashFronts.back();
			}
			trashFronts.pop_back();
		}
	
		// Deleting cells
		if (cells) {
			for (size_t i = 0; i < atmoNX; ++i) {
				if (cells[i]) {
					delete[] cells[i];
				}
			}
			delete[] cells;
			cells = nullptr;
		}
	
		if (trashCell) {
			delete trashCell;
			trashCell = nullptr;
		}
	
		// Deleting frontiers
		while (!frontiers.empty()) {
			if (frontiers.back()) {
				delete frontiers.back();
			}
			frontiers.pop_back();
		}
		while (!infrontiers.empty()) {
			if (infrontiers.back()) {
				delete infrontiers.back();
			}
			infrontiers.pop_back();
		}
	
		// Deleting data broker and layers
		if (dataBroker) {
			delete dataBroker;
			dataBroker = nullptr;
		}
		if (propagativeLayer) {
			//delete propagativeLayer;
			propagativeLayer = nullptr;
		}
	
		if ( mainFrontBackup != 0 ) delete mainFrontBackup;
	}
 
	 void FireDomain::backupState(){
		 /* Backup of the simulation */
		 if ( mainFrontBackup != 0 ) delete mainFrontBackup;
		 mainFrontBackup = new FireFrontData(domainFront);
	 }
 inline size_t fileSize(const std::string& name) {
   struct stat buffer;   
	   if(stat(name.c_str(), &buffer) != 0) {
		 return 0;
	 }
	 return buffer.st_size;   
 }
 void FireDomain::pushMultiDomainMetadataInList(size_t id, double lastTime, size_t atmoNX, size_t atmoNY, double nswx, double nswy, double nnex, double nney) {
	 distributedDomainInfo *currentInfo = new distributedDomainInfo;
	 currentInfo->ID = id;
	 currentInfo->lastTime = lastTime;
	 currentInfo->atmoNX = atmoNX;
	 currentInfo->atmoNY = atmoNY;
	 currentInfo->SWCorner = new FFPoint(nswx, nswy, 0.);
	 currentInfo->NECorner = new FFPoint(nnex, nney, 0.);
	 
	 parallelDispatchDomains.push_back(currentInfo);
 }
 void FireDomain::readMultiDomainMetadata(){
		 if (getDomainID()!=0) return;
 
		 size_t numReadDom = 1;
		 string ffDomPattern(params->getParameter("caseDirectory")+'/'+params->getParameter("PPath")+'/');
		  size_t domFileSize = fileSize(ffDomPattern+std::to_string(numReadDom)+".domainData");
		 while(domFileSize > 0){
			  numReadDom++;
			 domFileSize = fileSize(ffDomPattern+std::to_string(numReadDom)+".domainData");
		 }
		 numReadDom -=1;
  
		 //parallelDispatchDomains.size() = numReadDom;
 
		 while(numReadDom > 0){
 
			 ifstream FileIn((ffDomPattern+std::to_string(numReadDom)+".domainData").c_str(), ios_base::binary);
 
			 size_t anx = 0;
			 size_t any = 0;
			 double nswx = 0;
			 double nswy = 0;
			 double nnex = 0;
			 double nney = 0;
	 
			 FileIn.read((char *)&anx, sizeof(size_t));
			 FileIn.read((char *)&any, sizeof(size_t));
			 FileIn.read((char *)&nswx, sizeof(double));
			 FileIn.read((char *)&nswy, sizeof(double));
			 FileIn.read((char *)&nnex, sizeof(double));
			 FileIn.read((char *)&nney, sizeof(double));
			 FileIn.close();
			 pushMultiDomainMetadataInList(numReadDom,0,anx,any,nswx,nswy,nnex,nney);
 
			 numReadDom--;
		 }	
	 
	 
	 } 
 
	 void FireDomain::restoreValidState(){
		 /* Restoring a previously saved valid state */
		 if ( mainFrontBackup == 0 ){
			 debugOutput<<getDomainID()<<": PROBLEM, tried to return to a valid state"
			 <<" while no backup state is available"<<endl;
		 } else {
			 debugOutput<<getDomainID()<<": RECONSTRUCTING STATE"<<endl;
		 }
		 /* Trashing the current state */
		 trashFrontsAndNodes();
		 /* Restoring the valid state */
		 mainFrontBackup->reconstructState(domainFront);
	 }
 
	 void FireDomain::trashFrontsAndNodes(){
		 list<FireNode*> toBeTrashed;
		 list<FireNode*>::iterator node;
		 for ( size_t i = 0; i < atmoNX; i++ ) {
			 for ( size_t j = 0; j < atmoNY; j++ ) {
				 // deleting the nodes in the cell
				 toBeTrashed = cells[i][j].fireNodes;
				 for ( node = toBeTrashed.begin();
					  node != toBeTrashed.end(); ++node ){
					 debugOutput<<getDomainID()<<": FireDomain::trashFrontsAndNodes -> ";
					 addToTrashNodes(*node);
				 }
			 }
		 }
		 list<FireFront*> fronts = domainFront->getInnerFronts();
		 list<FireFront*>::iterator front;
		 for ( front = fronts.begin(); front != fronts.end(); ++front ){
			 addToTrashFronts(*front);
		 }
	 }
 
	 // Accessors
	 int FireDomain::getReferenceYear(){
		 return refYear;
	 }
	 int FireDomain::getReferenceDay(){
		 return refDay;
	 }
	 double FireDomain::getSecondsFromReferenceTime(
													const int& year, const int& month
													, const int& day, const int& t){
		 if ( year != refYear ) cout<<"WARNING: change of year "<< year <<" to "<< refYear <<" is not handled in ForeFire data at YY/MM/DD "<<year<<"/"<<month<<"/"<<day<<endl;
		 int iday = getDayNumber(year, month, day);
		 return (iday-refDay)*86400. + t;
	 }
	 list<FireFront*> FireDomain::getMainFronts(){
		 return domainFront->getInnerFronts();
	 }
	 FireFront* FireDomain::getDomainFront(){
		 return domainFront;
	 }
	 TimeTable* FireDomain::getTimeTable(){
		 return schedule;
	 }
	 double FireDomain::getSimulationTime(){
		 if ( schedule->getTime() > this->getTime() ) {
			 return double(int(schedule->getTime()));
		 } else {
			 return this->getTime();
		 }
	 }
 
	 FFPoint& FireDomain::getSWCorner(){
		 return SWCorner;
	 }
	 FFPoint& FireDomain::getNECorner(){
		 return NECorner;
	 }
	 double& FireDomain::SWCornerX(){
		 return SWCorner.getX();
	 }
	 double& FireDomain::SWCornerY(){
		 return SWCorner.getY();
	 }
	 double& FireDomain::NWCornerX(){
		 return NWCorner.getX();
	 }
	 double& FireDomain::NWCornerY(){
		 return NWCorner.getY();
	 }
	 double& FireDomain::NECornerX(){
		 return NECorner.getX();
	 }
	 double& FireDomain::NECornerY(){
		 return NECorner.getY();
	 }
	 double& FireDomain::SECornerX(){
		 return SECorner.getX();
	 }
	 double& FireDomain::SECornerY(){
		 return SECorner.getY();
	 }
 
	 double& FireDomain::getPerimeterResolution(){
		 return perimeterResolution;
	 }
 
	 double& FireDomain::getSpatialIncrement(){
		 return spatialIncrement;
	 }
 
	 double& FireDomain::getMaxTimeStep(){
		 return dtMax;
	 }
 
	 int FireDomain::getNumIterationAtmoModel(){
		 return numIterationAtmoModel;
	 }
	 void FireDomain::increaseNumIterationAtmoModel(){
		 numIterationAtmoModel++;
	 }
 
	 int FireDomain::getDayNumber(const int& year
								  , const int& month, const int& day){
		 bool leap = ( ( year%4 == 0 and year%100 != 0 ) or year%400 == 0 );
		 int febNumDays;
		 leap == true ? febNumDays = 29 : febNumDays = 28;
		 int numDays[11] = {31, febNumDays, 31, 30, 31, 30, 31, 31, 30, 31, 30};
		 int numDay = 0;
		 for ( int imon = 1; imon < month; imon++ ){
			 numDay += numDays[imon-1];
		 }
		 numDay += day;
		 return numDay;
	 }
 
	 // Mutators
	 void FireDomain::setSafeTopologyMode(bool safeMode){
		 safeTopologyMode = safeMode;
	 }
 
	 void FireDomain::setBoundariesFront(FireFront* ff){
		 domainFront = ff;
	 }
	 void FireDomain::setTimeTable(TimeTable* tt){
		 schedule = tt;
	 }
 
	 // input function
	 void FireDomain::input(){
	 }
 
	 // update function
	 void FireDomain::update(){
		 setTime(getUpdateTime());
	 }
 
	 // advance in time function
	 void FireDomain::timeAdvance(){
		 setUpdateTime(numeric_limits<double>::infinity());
	 }
 
	 // Output function
	 void FireDomain::output(){
	 }
 
	 // Adding a newly created Atom to the simulation
	 void FireDomain::addNewAtomToSimulation(ForeFireAtom* atom){
		 schedule->insertBefore(new FFEvent(atom));
	 }
 
	 // Deleting an Atom of the simulation
	 void FireDomain::deleteAtomOfSimulation(ForeFireAtom* atom){
		 schedule->dropAtomEvents(atom);
	 }
 
	 // Searching for an atom in the entire domain
	 FireNode* FireDomain::getFireNodeByID(const long& sid){
		 FireNode* tmpAtom;
		 for ( size_t i=0; i<atmoNX; i++ ){
			 for ( size_t j=0; j<atmoNY; j++ ){
				 tmpAtom = cells[i][j].getFirenodeByID(sid);
				 if ( tmpAtom ) return tmpAtom;
			 }
		 }
		 return 0;
	 }
 
	 FireNode* FireDomain::getFireNodeByID(const double sid){
		 long lsid = getIDfromDouble(sid);
		 return getFireNodeByID(lsid);
	 }
	 
	 FireNode* FireDomain::getFireNodeByIDNeighborCells(
														const double id, FDCell* cell, int range){
		 FireNode* tmpAtom;
		 /* Looking in the current cell */
		 tmpAtom = cell->getFirenodeByID(id);
		 if ( tmpAtom ) return tmpAtom;
		 /* searching in neighboring cells */
		 list<FDCell*> proxCells = getProxCells(cell, range);
		 list<FDCell*>::iterator curCell;
		 for ( curCell = proxCells.begin();
			  curCell != proxCells.end(); ++curCell){
			 tmpAtom = (*curCell)->getFirenodeByID(id);
			 if ( tmpAtom ) return tmpAtom;
		 }
		 // No firenode was found with this ID
		 debugOutput<<getDomainID()<<": "
			 <<"could not find a firenode with Id "
			 <<getDomainID(id)<<", "<<getShortID(id)<<endl;
		 return 0;
	 }
 
	  
 
	 int FireDomain::registerPropagationModelInstantiator(string modelname
														  , PropagationModelInstantiator func){
		 PropModelMap::iterator pmodel = prop_instantiatorMap().find(modelname);
		 if ( pmodel != prop_instantiatorMap().end() ){
			 cout<<"WARNING: propagation model "<<modelname
			 <<" has the same name as a previously"
			 <<" registered propagation model, check your settings"<<endl;
		 }
		 prop_instantiatorMap().insert(make_pair(modelname,func));
		 return 1;
	 }
 
	 void FireDomain::updateFuelTable( string key, double value){
		for ( size_t i = 0; i < NUM_MAX_PROPMODELS; i++ ){
			if(propModelsTable[i] != 0){
 
 
				getDataBroker()->updateFuelValues(propModelsTable[i],key,value);
			}
		}
	 }
	 int FireDomain::registerFluxModelInstantiator(string modelname
												   , FluxModelInstantiator func){
		 FluxModelMap::iterator fmodel = flux_instantiatorMap().find(modelname);
		 if ( fmodel != flux_instantiatorMap().end() ){
			 cout<<"WARNING: flux model "<<modelname
			 <<" has the same name as a previously"
			 <<" registered flux model, check your settings"<<endl;
		 }
		 flux_instantiatorMap().insert(make_pair(modelname,func));
		 return 1;
	 }
 
	 PropagationModel* FireDomain::propModelInstanciation(const int& index, string modelname){
		 PropModelMap::iterator pmodel = prop_instantiatorMap().find(modelname);
		 if ( pmodel != prop_instantiatorMap().end() ){
			 return (pmodel->second)(index, dataBroker);
		 } else {
			 cout<<"ERROR: Propagation model "<<modelname
				 <<" is not recognized, check your configuration !!"<< endl;
			 return 0;
		 }
	 }
 
 
	 FluxModel* FireDomain::fluxModelInstanciation(const int& index, string modelname){
		 FluxModelMap::iterator fmodel;
 
			 
		 fmodel = flux_instantiatorMap().find(modelname);
		 
 
		 if ( fmodel != flux_instantiatorMap().end() ){
 
			 return (fmodel->second)(index, dataBroker);
		 } else {
			 cout<<"ERROR: Flux model "<<modelname
			 <<" is not recognized, check your configuration !!"<< endl;
			 return 0;
		 }
	 }
 
	 void FireDomain::registerPropagationModel(const int& index, PropagationModel* model){
			 // Join all strings in the vector into a single string separated by semicolons
	 std::string properties = "";
	 for(const std::string& prop : model->wantedProperties) {
		 if(properties.empty()) {
			 properties = prop; // First element, add without semicolon
		 } else {
			 properties += ";" + prop; // Subsequent elements, add with semicolon
		 }
	 }
	 
	 // Set the parameter with the joined string
		 params->setParameter(model->getName() + ".keys", properties);
		 //cout<< "loading"<< model->getName()<<index<<endl;
		 propModelsTable[index] = model;
	 }
 
	 void FireDomain::registerFluxModel(const int& index, FluxModel* model){
		  
		 fluxModelsTable[index] = model;
	 }
 
	 bool FireDomain::addPropagativeLayer(string mname){
		 /* searching if there exists a propagation model with associated name */
		 /* affecting it to free index */
		 size_t mindex = getFreePropModelIndex();
		 PropagationModel* model = propModelInstanciation(mindex, mname);
		 if ( model == 0 ) return false;
		 /* Instantiating a flux layer related to this model */
		 propagativeLayer = new PropagativeLayer<double>("ROSlayer", mindex);
		 return true;
	 }
 
	 bool FireDomain::addLayer(string type, string layername, string keyname){
		double timespan = 0;
		size_t  nnx = 1;
		size_t nny = 1;
		size_t nnz = 1;
		size_t nnk = 1;

		double spanx = NECorner.x -SWCorner.x;
		double spany = NECorner.y -SWCorner.y;
 
 
		 if ( type == "BRatio" ){
			 BurningRatioLayer<double>* brlayer = new BurningRatioLayer<double>(layername, SWCorner, NECorner, atmoNX, atmoNY, cells);
			 dataBroker->registerLayer(layername, brlayer);
			 return true;
		 }
		 if ( type == "MaxRos" ){
			 RosLayer<double>* mrlayer = new RosLayer<double>(layername, SWCorner, NECorner, atmoNX, atmoNY, cells);
				 dataBroker->registerLayer(layername, mrlayer);
				 return true;
			 }
 
 
		 if ( !params->isValued(keyname) ){
			 cout << "Unknown parameter "<<keyname<<" for "<<layername << " layer of type "<< type <<", please set parameter"<<endl;
			 return false;
		 }
 

 
		 if ( type == "data" ){
			 double* values = new double[1];
			 values[0] = params->getDouble(keyname);
			 return addScalarLayer( type,  layername,SWCorner.getX(), SWCorner.getY(), getTime(), spanx , spany, timespan,  nnx,	 nny, nnz,  nnk,  values);
		 }else{
			 int* values = new int[1];
			 values[0] = params->getInt(keyname);
			 return addIndexLayer( type,  layername,SWCorner.getX(), SWCorner.getY(), getTime(), spanx , spany, timespan,  nnx,	 nny, nnz,  nnk,  values);
		 }
 
	 }
 
	 bool FireDomain::addScalarLayer(string type, string name, double &x0, double &y0, double& t0, double& width, double& height, double& timespan, size_t& nnx,	size_t& nny, size_t& nnz, size_t& nnk, double* values){
		 FFPoint origin = FFPoint(x0, y0,0);
		 FFPoint span = FFPoint(width, height,0);
 
		 XYZTDataLayer<double>* newLayer = new XYZTDataLayer<double>(name, origin,t0, span, timespan, nnx, nny, nnz, nnk, values);
		 dataBroker->registerLayer(name, newLayer);
 
		 //cout<<"adding scalar Layer "<<name<<" of type "<<type<<" position "<<origin.x<<";"<<origin.y<<" size "<<span.x<< "initial date"<< t0<<" time duration "<<timespan<<endl;
		 if ( type == "windScalDir" ){
			 if ( name == "windU" ){
				 
				 dataBroker->PwindULayer = newLayer;
			 }
			 if ( name == "windV" ){
				 dataBroker->PwindVLayer = newLayer;
			 }
			 }
 
 
		 return true;
	 }
	 bool FireDomain::addIndexLayer(string type, string name, double &x0, double &y0, double& t0, double& width, double& height, double& timespan, size_t& nnx,	size_t& nny, size_t& nnz, size_t& nnk, int* values){
			 FFPoint origin = FFPoint(x0, y0,0);
			 FFPoint span = FFPoint(width, height,0);
 
 
			 if ( type == "flux" ){
				 size_t mindex = values[0];
				 string fmname = name;
				 
				 FluxModel* model = fluxModelInstanciation(mindex, fmname);
				 if ( model != 0 ){
					 /* Instantiating a flux layer related to this model */
					 FluxLayer<double>* newlayer = new FluxLayer<double>(name,SWCorner, NECorner,atmoNX, atmoNY, getCells(), values, origin, t0, span,	timespan, nnx, nny, nnz, nnk);
					 dataBroker->registerFluxLayer(name, newlayer);
					 return true;
				 }
 
			 }
			 if ( type == "propagation" ){
				 size_t mindex = getFreePropModelIndex();
				 PropagationModel* model = propModelInstanciation(mindex, name);
				 if ( model == 0 ) return false;
				 /* Instantiating a prop layer related to this model */
				 propagativeLayer = new PropagativeLayer<double>(name, mindex);
				 return true;
			 }
			 if ( type == "table" ){
 
					 FuelDataLayer<double>* newlayer = new FuelDataLayer<double>(name,	origin, t0, span, timespan, nnx, nny, nnz, nnk,				values);
				 
					 dataBroker->registerLayer(name, newlayer);
			 }
 
			 return false;
		 }
 
	 bool FireDomain::addFluxLayer(string lname){
		 /* searching if there exists a flux model with associated name */
		 /* affecting it to free index */
		
		 // Burning ratio is special as it is derived from the heat flux layer
		 if ( lname == "BRatio" or lname == "Bratio" or lname == "bratio" ){
			 /* Instantiating a burning ratio layer */
			 BurningRatioLayer<double>* brlayer =
			 new BurningRatioLayer<double>(lname,  SWCorner, NECorner,atmoNX, atmoNY, cells);
			 dataBroker->registerLayer(lname, brlayer);
			 return true;
		 }
 
		 // Otherwise, searching for the model in the available ones
		 size_t mindex = getFreeFluxModelIndex();
		 string fmname = "";
		 if ( lname == "heatFlux" ) fmname = "heatFluxBasic";
		 if ( lname == "vaporFlux" ) fmname = "vaporFluxBasic";
		 
		 string defaultModelKey = lname+"DefaultModel";

		 if (params->isValued(defaultModelKey)){
			fmname =  params->getParameter(lname+"DefaultModel");
		 }

		 FluxModel* model = fluxModelInstanciation(mindex, fmname);
 
		 if ( model != 0 ){
			 /* Instantiating a flux layer related to this model */
			 FluxLayer<double>* newlayer = new FluxLayer<double>(lname, SWCorner, NECorner, atmoNX, atmoNY, cells, mindex);
			 registerFluxModel(model->index, model);
			 dataBroker->registerFluxLayer(lname, newlayer);
			 return true;
		 }else{
			 cout<<"ERROR: Flux model "<<fmname
			 <<" is not recognized, check your configuration !!"<< endl;
			 return false;
		 }
		 return false;
	 }
 
	 size_t FireDomain::getFreePropModelIndex(){
		 size_t mindex = NUM_MAX_PROPMODELS - 1;
		 while ( propModelsTable[mindex] != 0 ) mindex--;
		 return mindex;
	 }
 
	 size_t FireDomain::getFreeFluxModelIndex(){
		 size_t mindex = 0;
		 while ( fluxModelsTable[mindex] != 0 ){
			 if(mindex >= NUM_MAX_FLUXMODELS -1){
				 cout<<"ERROR No mor flx models allowed, max:"<< NUM_MAX_FLUXMODELS<<endl;
				 return mindex;
			 }
			 mindex++;
		 }
		 return mindex;
	 }
 
	 double FireDomain::getArrivalTime(FFPoint& loc){
		 if ( !striclyWithinDomain(loc) ) return numeric_limits<double>::infinity();
		 size_t ii = (size_t) ((loc.getX()-SWCornerX())/burningMatrixResX);
		 size_t jj = (size_t) ((loc.getY()-SWCornerY())/burningMatrixResY);
		 return getArrivalTime(ii, jj);
	 }
	 double FireDomain::getMaxSpeed(FFPoint& loc){
		 if ( !striclyWithinDomain(loc) ) return numeric_limits<double>::infinity();
		 size_t ii = (size_t) ((loc.getX()-SWCornerX())/burningMatrixResX);
		 size_t jj = (size_t) ((loc.getY()-SWCornerY())/burningMatrixResY);
		 return getMaxSpeed(ii, jj);
	 }
 
	 double FireDomain::getArrivalTime(const size_t& ii, const size_t& jj){
		 if ( ii > globalBMapSizeX-1 ) return numeric_limits<double>::infinity();
		 if ( jj > globalBMapSizeY-1 ) return numeric_limits<double>::infinity();
		 size_t i = ii/localBMapSizeX;
		 size_t j = jj/localBMapSizeY;
		 return cells[i][j].getArrivalTime(ii%localBMapSizeX,jj%localBMapSizeY);
	 }
 
	 double FireDomain::getMaxSpeed(const size_t& ii, const size_t& jj) {
			 if (ii >= globalBMapSizeX || jj >= globalBMapSizeY) return std::numeric_limits<double>::infinity();
			 double current_time = getArrivalTime(ii, jj);
			 if (current_time == std::numeric_limits<double>::infinity()) return std::numeric_limits<double>::infinity();
 
			 double resolution = params->getDouble("bmapResolution");
			 double grad_x = 0.0, grad_y = 0.0;
 
			 if (ii > 0 && ii < globalBMapSizeX - 1) {
				 double time_left = getArrivalTime(ii - 1, jj);
				 double time_right = getArrivalTime(ii + 1, jj);
				 if (time_left != std::numeric_limits<double>::infinity() && time_right != std::numeric_limits<double>::infinity()) {
					 grad_x = (time_right - time_left);
				 }
			 }
			 if (jj > 0 && jj < globalBMapSizeY - 1) {
				 double time_down = getArrivalTime(ii, jj - 1);
				 double time_up = getArrivalTime(ii, jj + 1);
				 if (time_down != std::numeric_limits<double>::infinity() && time_up != std::numeric_limits<double>::infinity()) {
					 grad_y = (time_up - time_down);
				 }
			 }
 
			 double grad_squared = grad_x * grad_x + grad_y * grad_y;
			 if (grad_squared == 0.0) return std::numeric_limits<double>::infinity(); // Safe division check
			 double grad_mag = (2 * resolution) / sqrt(grad_squared);
			 
			 return grad_mag;
		 }
 
	 void FireDomain::setArrivalTime(const size_t& ii, const size_t& jj, const double& t){
		 // getting the cell where to pixel to set to burning lies
 
		 if ( ii > globalBMapSizeX-1 ) return ;
		 if ( jj > globalBMapSizeY-1 ) return ;
		 if ( ii > globalBMapSizeX-1 ) return ;
		 if ( jj > globalBMapSizeY-1 ) return ;
		 size_t i = ii/localBMapSizeX;
		 size_t j = jj/localBMapSizeY;
		 // setting the arrival time of this pixel
		 cells[i][j].setArrivalTime(ii%localBMapSizeX,jj%localBMapSizeY,t);
	 }
 
	 bool FireDomain::burnCheck(const size_t& ii, const size_t& jj, const double& t){
		 if ( getArrivalTime(ii-1,jj-1) > t ) return true;
		 if ( getArrivalTime(ii-1,jj) > t ) return true;
		 if ( getArrivalTime(ii,jj-1) > t ) return true;
		 if ( getArrivalTime(ii,jj) > t ) return true;
		 return false;
	 }
 
	 void FireDomain::firenodeBurningScan(FireNode* fn){
 
		 /* Local scan of the domain around a firenode. */
		 /* First a bounding box for scanning is computed.
		  * Then a local optimized polygon is defined locally
		  * to optimize the "point in polygon" algorithm,
		  * and lastly the scan is made */
 
		 // I/ Compute the region to be scanned
		 FFPoint swc, nec;
		 if ( fn->getPrev() == 0 or fn->getNext() == 0 ){
			 debugOutput<<getDomainID()<<": WARNING, launching a "
				 <<"FireDomain::firenodeBurningScan for a firenode "
				 <<"with no previous or next"<<endl;
			 swc.setX(fn->getX() - 2.*getPerimeterResolution());
			 swc.setY(fn->getY() - 2.*getPerimeterResolution());
			 nec.setX(fn->getX() + 2.*getPerimeterResolution());
			 nec.setY(fn->getY() + 2.*getPerimeterResolution());
		 } else {
			 computeBoundingBox(fn->getPrev(), fn, fn->getNext(), swc, nec);
		 }
		 // II/ Construct the vertices of a local optimized polygon
		 vector<double> nodesx, nodesy;
		 constructLocalSurroundingPolygon(fn, swc, nec, nodesx, nodesy);
 
		 // III/ Scanning the region
		 size_t nvert = nodesx.size();
		 double* vertx = 0;
		 double* verty = 0;
		 if ( nvert > 0 ){
			 vertx = new double[nvert];
			 verty = new double[nvert];
			 for ( size_t i = 0; i < nvert; i++ ){
				 vertx[i] = nodesx[i];
				 verty[i] = nodesy[i];
			 }
		 } else {
			 size_t nvert = fn->getFront()->getNumFN();
			 vertx = new double[nvert];
			 verty = new double[nvert];
			 fn->getFront()->storeVertices(vertx, verty, nvert);
		 }
		 singlePolygonAreaBurningScan(swc, nec, fn->getTime()
									  , fn->getFront()->isExpanding(), nvert, vertx, verty);
		 delete [] vertx;
		 delete [] verty;
 
	 }
 
	 void FireDomain::constructLocalSurroundingPolygon(FireNode* fn,
													   FFPoint& swc, FFPoint& nec, vector<double>& px, vector<double>& py){
 
		 /* Constructing a local polygon around the desired bounding box
		  * and taking into account the position of the firenodes in the bounding box */
 
		 // Constructing the polygon of the bounding box
		 size_t npoly = 4;
		 double* xpoly = new double[4];
		 double* ypoly = new double[4];
		 xpoly[0] = swc.getX();
		 ypoly[0] = swc.getY();
		 xpoly[1] = swc.getX();
		 ypoly[1] = nec.getY();
		 xpoly[2] = nec.getX();
		 ypoly[2] = nec.getY();
		 xpoly[3] = nec.getX();
		 ypoly[3] = swc.getY();
 
		 // Searching for the previous node outside this bounding box
		 FireNode* tmpfn = (fn->getPrev())->getPrev();
		 if ( tmpfn == 0 ){
			 delete [] xpoly;
			 delete [] ypoly;
			 return;
		 }
		 while ( tmpfn->getLoc().pointInPolygon(npoly, xpoly, ypoly) and tmpfn != fn ){
			 tmpfn = tmpfn->getPrev();
			 if ( tmpfn == 0 ){
				 delete [] xpoly;
				 delete [] ypoly;
				 return;
			 }
		 }
 
		 // If the front just happens to be contained in the bounding box returning nothing
		 if ( tmpfn == fn ){
			 delete [] xpoly;
			 delete [] ypoly;
			 return;
		 }
 
		 // Storing the locations of the firenodes inside the bounding box
		 // (with the previous and next nodes outside this box)
		 FFPoint pOut = tmpfn->getLoc();
		 tmpfn = tmpfn->getNext();
		 FFPoint pIn = tmpfn->getLoc();
		 while ( tmpfn->getLoc().pointInPolygon(npoly, xpoly, ypoly) ){
			 px.push_back(tmpfn->getX());
			 py.push_back(tmpfn->getY());
			 tmpfn = tmpfn->getNext();
			 if ( tmpfn == 0 ){
				 while ( !px.empty() ) {
					 px.pop_back();
					 py.pop_back();
				 }
				 return;
			 }
		 }
		 FFPoint nOut = tmpfn->getLoc();
		 FFPoint nIn = tmpfn->getPrev()->getLoc();
 
		 // Finding the intersection with the bounding box
		 int begSeg, endSeg;
		 FFPoint pInter = findIntersectionWithBoundingBox(pIn, pOut, swc, nec, endSeg);
		 FFPoint nInter = findIntersectionWithBoundingBox(nIn, nOut, swc, nec, begSeg);
		 if((nInter != outPoint )||(pInter != outPoint )){
			 px.push_back(nInter.getX());
			 py.push_back(nInter.getY());
			 
 
		 // Linking the last and first points to have a closed polygon
		 FFPoint corner;
		 if ( begSeg == endSeg ){
			 if ( distanceOnBoundingBox(begSeg, nInter, pInter) < 0. ){
				 begSeg = (begSeg+1)%4;
				 corner = getBoundingBoxCornerFromIndex(begSeg, swc, nec);
				 px.push_back(corner.getX());
				 py.push_back(corner.getY());
			 }
		 }
 
		 while ( begSeg != endSeg ){
			 begSeg = (begSeg+1)%4;
			 corner = getBoundingBoxCornerFromIndex(begSeg, swc, nec);
			 px.push_back(corner.getX());
			 py.push_back(corner.getY());
		 }
 
		 px.push_back(pInter.getX());
		 py.push_back(pInter.getY());
		 }else{
			 cout << getDomainID()<<" have a problem for burning scan"<<endl;
		 }
		 delete [] xpoly;
		 delete [] ypoly;
 
	 }
 
	 void FireDomain::singlePolygonAreaBurningScan(FFPoint& swc, FFPoint& nec, double t
												   , bool expanding, size_t& nvert, double* vertx, double* verty){
		 /* Scanning a domain with a given front */
		 /* looking at the vertices of the matrix of arrival times.
		  * If this vertice is inside the firefront, setting the
		  * arrival time of the four adjacent cells */
		 // I/ Defining the region to be scanned in terms of burning matrix
		 size_t minI = 1;
		 if ( swc.getX() > SWCornerX()+burningMatrixResX )
			 minI = (size_t) ((swc.getX() - SWCornerX())/burningMatrixResX);
		 size_t maxI = (size_t) ((nec.getX() - SWCornerX())/burningMatrixResX) + 1;
		 if ( maxI > globalBMapSizeX - 1 ) maxI = globalBMapSizeX - 1;
		 size_t minJ = 1;
		 if ( swc.getY() > SWCornerY()/burningMatrixResY )
			 minJ = (size_t) ((swc.getY() - SWCornerY())/burningMatrixResY);
		 size_t maxJ = (size_t) ((nec.getY() - SWCornerY())/burningMatrixResY) + 1;
		 if ( maxJ > globalBMapSizeY - 1 ) maxJ = globalBMapSizeY - 1;
 
		 // II/ Scanning the region
		 FFPoint node;
		 for ( size_t i = minI; i <= maxI; i++ ){
			 for ( size_t j = minJ; j <= maxJ; j++ ){
				 node.setX(SWCornerX()+i*burningMatrixResX);
				 node.setY(SWCornerY()+j*burningMatrixResY);
				 if ( burnCheck(i,j,t) and
					 node.pointInPolygon(nvert, vertx, verty) == expanding ){
					 setArrivalTime(i-1, j-1, t);
					 setArrivalTime(i-1, j, t);
					 setArrivalTime(i, j-1, t);
					 setArrivalTime(i, j, t);
				 }
			 }
		 }
 
	 }
 
	 void FireDomain::areaBurningScan(FFPoint& swc, FFPoint& nec, double t){
		 /* Global scan of a bounding box for the update of the burning matrix. */
 
		 // I/ Defining the region to be scanned in terms of burning matrix
		 size_t minI = 1;
		 if ( swc.getX() > SWCornerX()+burningMatrixResX )
			 minI = (size_t) ((swc.getX() - SWCornerX())/burningMatrixResX);
		 size_t maxI = (size_t) ((nec.getX() - SWCornerX())/burningMatrixResX) + 1;
		 if ( maxI > globalBMapSizeX - 1 ) maxI = globalBMapSizeX - 1;
		 size_t minJ = 1;
		 if ( swc.getY() > SWCornerY()+burningMatrixResY )
			 minJ = (size_t) ((swc.getY() - SWCornerY())/burningMatrixResY);
		 size_t maxJ = (size_t) ((nec.getY() - SWCornerY())/burningMatrixResY) + 1;
		 if ( maxJ > globalBMapSizeY - 1 ) maxJ = globalBMapSizeY - 1;
 
		 // II/ Preparing the fronts for the point in polygon algorithm
		 domainFront->constructVerticesVectors();
 
		 // III/ Scanning the region
		 FFPoint node;
		 node.setX(SWCornerX()+minI*burningMatrixResX);
		 for ( size_t i = minI; i <= maxI; i++ ){
			 node.setY(SWCornerY()+minJ*burningMatrixResY);
			 for ( size_t j = minJ; j <= maxJ; j++ ){
				 if ( burnCheck(i, j, t) and checkForBurningStatus(node) ){
					 setArrivalTime(i-1, j-1, t);
					 setArrivalTime(i-1, j, t);
					 setArrivalTime(i, j-1, t);
					 setArrivalTime(i, j, t);
				 }
				 node.setY(node.getY()+burningMatrixResY);
			 }
			 node.setX(node.getX()+burningMatrixResX);
		 }
 
		 domainFront->deleteVerticesVectors();
 
	 }
 
	 void FireDomain::frontInitialBurningScan(const double& t, FireFront* ff
											  , const double& dmax, const double& dt){
 
		 /* Global scan of a front bounding box for
		  * the update of the burning matrix.
		  * Points inside the front are updated
		  * according to their distance to the front */
 
		 bool constantTimeInit = true;
		 if ( dmax > 0. ) constantTimeInit = false;
 
		 // I/ Construct the vertices of the current front and of all the fronts
		 size_t nvert = ff->getNumFN();
		 double* vertx = new double[nvert];
		 double* verty = new double[nvert];
		 ff->storeVertices(vertx, verty, nvert);
 
		 domainFront->constructVerticesVectors();
 
		 // II/ Compute the region to be scanned
		 FFPoint swc, nec;
		 ff->computeBoundingBox(swc, nec);
		 size_t minI = 1;
		 if ( swc.getX() > SWCornerX()+2.*burningMatrixResX )
			 minI = (size_t) ((swc.getX() - SWCornerX())/burningMatrixResX);
		 size_t maxI = (size_t) ((nec.getX() - SWCornerX())/burningMatrixResX) + 1;
		 if ( maxI > globalBMapSizeX - 1 ) maxI = globalBMapSizeX - 1;
		 size_t minJ = 1;
		 if ( swc.getY() > SWCornerY()+burningMatrixResY )
			 minJ = (size_t) ((swc.getY() - SWCornerY())/burningMatrixResY);
		 size_t maxJ = (size_t) ((nec.getY() - SWCornerY())/burningMatrixResY) + 1;
		 if ( maxJ > globalBMapSizeY - 1 ) maxJ = globalBMapSizeY - 1;
 
		 // III/ Scanning the region
		 double dist, at;
		 FFPoint node;
		 node.setX(SWCornerX()+minI*burningMatrixResX);
		 for ( size_t i = minI; i <= maxI; i++ ){
			 node.setY(SWCornerY()+minJ*burningMatrixResY);
			 for ( size_t j = minJ; j <= maxJ; j++ ){
				 if ( checkForBurningStatus(node) ){
					 if ( constantTimeInit ){
						 setArrivalTime(i-1, j-1, t);
						 setArrivalTime(i-1, j, t);
						 setArrivalTime(i, j-1, t);
						 setArrivalTime(i, j, t);
					 } else {
						 dist = node.signedDistanceToPolygon(
															 nvert, vertx, verty, ff->isExpanding());
						 at = t - dist*dt/dmax;
						 setArrivalTime(i-1, j-1, at);
						 setArrivalTime(i-1, j, at);
						 setArrivalTime(i, j-1, at);
						 setArrivalTime(i, j, at);
					 }
				 }
				 node.setY(node.getY()+burningMatrixResY);
			 }
			 node.setX(node.getX()+burningMatrixResX);
		 }
 
		 domainFront->deleteVerticesVectors();
		 delete [] vertx;
		 delete [] verty;
	 }
 
	 void FireDomain::frontBurningScan(FireFront* ff, const double& t){
		 /* Global scan of a front bounding box for the update of the burning matrix. */
 
		 // I/ Compute the region to be scanned
		 FFPoint swc, nec;
		 ff->computeBoundingBox(swc, nec);
 
		 // II/ Defining the region to be scanned in terms of burning matrix
		 size_t minI = 1;
		 if ( swc.getX() > SWCornerX()+burningMatrixResX )
			 minI = (size_t) ((swc.getX() - SWCornerX())/burningMatrixResX);
		 size_t maxI = (size_t) ((nec.getX() - SWCornerX())/burningMatrixResX) + 1;
		 if ( maxI > globalBMapSizeX - 1 ) maxI = globalBMapSizeX - 1;
		 size_t minJ = 1;
		 if ( swc.getY() > SWCornerY()+burningMatrixResY )
			 minJ = (size_t) ((swc.getY() - SWCornerY())/burningMatrixResY);
		 size_t maxJ = (size_t) ((nec.getY() - SWCornerY())/burningMatrixResY) + 1;
		 if ( maxJ > globalBMapSizeY - 1 ) maxJ = globalBMapSizeY - 1;
 
		 // III/ Preparing the fronts for the point in polygon algorithm
		 domainFront->constructVerticesVectors();
 
		 // IV/ Scanning the region
		 FFPoint node;
		 node.setX(SWCornerX()+minI*burningMatrixResX);
		 for ( size_t i = minI; i <= maxI; i++ ){
			 node.setY(SWCornerY()+minJ*burningMatrixResY);
			 for ( size_t j = minJ; j <= maxJ; j++ ){
				 if ( burnCheck(i, j, t) and checkForBurningStatus(node) ){
					 setArrivalTime(i-1, j-1, t);
					 setArrivalTime(i-1, j, t);
					 setArrivalTime(i, j-1, t);
					 setArrivalTime(i, j, t);
				 }
				 node.setY(node.getY()+burningMatrixResY);
			 }
			 node.setX(node.getX()+burningMatrixResX);
		 }
 
		 domainFront->deleteVerticesVectors();
 
	 }
 
	 void FireDomain::computeBoundingBox(FireNode* fna, FireNode* fnb, FireNode* fnc
										 , FFPoint& swc, FFPoint& nec){
 
		 double a = min(fna->getX(), min(fnb->getX(), fnc->getX()));
		 swc.setX(a - 2.*spatialIncrement);
		 a = min(fna->getY(), min(fnb->getY(), fnc->getY()));
		 swc.setY(a - 2.*spatialIncrement);
		 a = max(fna->getX(), max(fnb->getX(), fnc->getX()));
		 nec.setX(a + 2.*spatialIncrement);
		 a = max(fna->getY(), max(fnb->getY(), fnc->getY()));
		 nec.setY(a + 2.*spatialIncrement);
 
	 }
 
	 bool FireDomain::checkForBurningStatus(FFPoint& loc){
		 return domainFront->checkForBurningStatus(loc);
	 }
 
	 // testing if a point is within the domain
	 bool FireDomain::striclyWithinDomain(FFPoint& p){
		 return striclyWithinDomain(p.getX(), p.getY());
	 }
	 bool FireDomain::striclyWithinDomain(const double& px, const double& py){
		 if ( px < SWCornerX() + EPSILONX ) return false;
		 if ( px > NECornerX() - EPSILONX ) return false;
		 if ( py < SWCornerY() + EPSILONX ) return false;
		 if ( py > NECornerY() - EPSILONX ) return false;
		 return true;
	 }
	 bool FireDomain::striclyWithinDomain(FireNode* fn){
		 return striclyWithinDomain(fn->getX(), fn->getY());
	 }
	 bool FireDomain::striclyWithinDomain(FireNodeData* fnd){
		 return striclyWithinDomain(fnd->posX, fnd->posY);
	 }
 
	 // testing if a point is within the domain
	 bool FireDomain::looselyWithinDomain(FFPoint& p){
		 return looselyWithinDomain(p.getX(), p.getY());
	 }
	 bool FireDomain::looselyWithinDomain(const double& px, const double& py){
		 if ( px < SWCornerX() - EPSILONX ) return false;
		 if ( px > NECornerX() + EPSILONX ) return false;
		 if ( py < SWCornerY() - EPSILONX ) return false;
		 if ( py > NECornerY() + EPSILONX ) return false;
		 return true;
	 }
	 bool FireDomain::looselyWithinDomain(FireNodeData* fnd){
		 return looselyWithinDomain(fnd->posX, fnd->posY);
	 }
 
	 // testing if a point is within the physical domain
	 bool FireDomain::withinPhysicalDomain(FFPoint& p){
		 if ( !striclyWithinDomain(p) ) return false;
		 if ( isInOuterHalo(p) ) return false;
		 return true;
	 }
 
	 PropagativeLayer<double>* FireDomain::getPropagativeLayer(){
		 return propagativeLayer;
	 }
 
	 void FireDomain::setPropagativeLayer(PropagativeLayer<double>* layer){
		 propagativeLayer = layer;
	 }
 
	 // Computing the propagation speed of a given firenode
	 double FireDomain::getPropagationSpeed(FireNode* fn) {
		 int modelIndex = propagativeLayer->getModelIndexAt(fn);
 
		 return propModelsTable[modelIndex]->getSpeedForNode(fn) * propagationSpeedAdjustmentFactor;
	 }
 
 
 
	 PropagationModel* FireDomain::getPropagationModel(const std::string& key) {
		 // Iterate from the last index down to 0
		 for (int i = static_cast<int>(NUM_MAX_PROPMODELS) - 1; i >= 0; --i) {
			 if (propModelsTable[i] != nullptr) {
				 if (propModelsTable[i]->getName() == key) {
					 return propModelsTable[i];
				 }
			 }
		 }
		 return nullptr; // Return nullptr if not found
	 }
 
	 // Computing the flux at a given location according to a given flux model
	 double FireDomain::getModelValueAt(int& modelIndex
										, FFPoint& loc, const double& bt, const double& et, const double& at){
		 if(fluxModelsTable[0] == NULL){
			 cout <<"Warning, no  flux layer registered"<<endl;
			 return 0;
		 }
		 return fluxModelsTable[modelIndex]->getValueAt(loc, bt, et, at);
	 }
 
	 // Checking whether a location is still burning
	 bool FireDomain::isBurning(FFPoint& loc, const double& t){
		 /* the location is considered to be burning if the heat released
		  * by the fire is superior to a prescribed value (~500 W.m-2) */
		 
		 double at = getArrivalTime(loc);
		 if ( t >= at ){ 
			 int mind = dataBroker->heatFluxLayer->getFunctionIndexAt(loc, t);
 
			 double heatFlux = getModelValueAt(mind, loc, t, t, at);
			 
			 if ( heatFlux > burningTresholdFlux ) {
				 return true;
			 }
		 }
		 return false;
	 }
 
	 // Checking whether a location is still burning
	 bool FireDomain::isBurnt(FFPoint& loc, const double& t){
		 /* the location is considered to be burnt if at < t*/
		 double at = getArrivalTime(loc);
		
		 return t>getArrivalTime(loc);

		
	 }

	 void FireDomain::setFrontDepthScheme(string scheme){
		 if ( scheme == "normalDir" or scheme == "normalDirection" ) fdScheme = normalDir;
		 if ( scheme == "closest" ) fdScheme = closest;
	 }
 
	 // Computing the front depth from a firenode
	 double FireDomain::computeFrontDepth(FireNode* fn){
 
		 /* This function computes the front depth from a given firenode */
		 double t = fn->getTime();
 
		 if ( fdScheme == normalDir ){
			 /* computes the front depth in the normal direction. */
			 /* First a raw search is from the location of the marker */
			 FFPoint loc;
			 double spinc = max(0.1*fn->getFrontDepth(), burningMatrixRes);
			 FFPoint spatialInc = -spinc*(fn->getNormal().toPoint());
 
			 /* Searching in the normal direction for the fire interface */
			 loc = fn->getLoc() + 1.1*spatialInc;
			 while ( isBurning(loc, t) and fn->getLoc().distance2D(loc) < maxFrontDepth ){
				 loc = loc + spatialInc;
			 }
 
			 /* Maximum front depth allowed */
			 if ( fn->getLoc().distance2D(loc) >= maxFrontDepth )
				 return maxFrontDepth;
 
			 /* Otherwise finishing the search by dichotomy */
			 FFPoint A = loc;
			 FFPoint B = loc - spatialInc;
			 FFPoint C = 0.5*(A+B);
			 while ( A.distance2D(B) > burningMatrixRes ){
				 if ( isBurning(C, t) ){
					 A = C;
				 } else {
					 B = C;
				 }
				 C = 0.5*(A+B);
			 }
 
			 /* Getting the altitude into consideration */
			 double alt = getDataLayer(altitude)->getValueAt(C, fn->getTime());
			 C.setZ(alt);
			 double dist = min(maxFrontDepth, fn->getLoc().distance(C));
			 return dist;
 
		 } else if ( fdScheme == closest ) {
			 // TODO
			 return fn->getFrontDepth();
		 } else {
			 cout<<"WARNING: unknown front depth scheme"<<endl;
			 return fn->getFrontDepth();
		 }
	 }
 
	 // Adding a firenode in the right cell
	 void FireDomain::addFireNodeInCell(FireNode* fn){
		 getCell(fn)->addFireNode(fn);
	 }
 
	 // Removing a firenode from a cell
	 void FireDomain::removeFireNodeInCell(FireNode* fn){
		 getCell(fn)->removeFireNode(fn);
	 }
 
	 // Updating the position of a firenode relatively to the cells
	 void FireDomain::updateFireNodeInCells(FireNode* fn){
		 FFPoint nextloc = fn->getNextLoc();
		 if ( getCell(nextloc) != getCell(fn) ) {
			 getCell(fn)->removeFireNode(fn);
			 getCell(nextloc)->addFireNode(fn);
		 }
	 }
 
	 // Informing the FireDomain that a firenode has moved
	 void FireDomain::hasMoved(FireNode* fn, FFPoint oldLoc, double oldTime){
		 if ( getCell(fn) == trashCell ) stopOutgoingNode(fn,oldLoc,oldTime);
	 }
 
	 // finding the cell that contains a given firenode
	 FDCell* FireDomain::getCell(FireNode* fn){
		 return getCell(fn->getLoc());
	 }
 
	 // finding the cell with the indices
	 FDCell* FireDomain::getCell(const int& i, const int& j){
		 if ( 0<=i and i< (int) atmoNX and 0<=j and j< (int) atmoNY ){
			 return &(cells[i][j]);
		 } else {
			 return 0;
		 }
	 }
	 // finding the cell with the indices
	 FDCell** FireDomain::getCells(){
		 return cells;
	 }
 
 
 
	 // Implementation of loadCellsInBinary
   /*
 void FireDomain::loadCellsInBinary(){
	 if(getDomainID() > 0){
		 // Define header and cell sizes
		 const size_t size_of_header = 4 * sizeof(size_t); // anx, any, snx, sny
		 // Construct the input file name
		 std::string domInName = params->getParameter("caseDirectory") + '/' +
								  params->getParameter("PPath") + '/' +
								  std::to_string(FireDomain::atmoIterNumber % 2) + "/" +
								  std::to_string(getDomainID()) + ".bmapcells";
		 // Get the size of the file
		 size_t domFsize = fileSize(domInName);
		 // Check if the file size is sufficient to contain the header
		 if(domFsize < size_of_header){
			 // File too small to contain valid header; no active cells
			 isFireActive = false;
			 return;
		 }
		 // Open the file in binary mode
		 std::ifstream FileIn(domInName, std::ios::binary);
		 if(!FileIn.is_open()){
			 std::cerr << "Failed to open file for reading: " << domInName << std::endl;
			 isFireActive = false;
			 return;
		 }
		 // Read the header: anx, any, snx, sny
		 size_t anx, any, snx, sny;
		 FileIn.read(reinterpret_cast<char*>(&anx), sizeof(size_t));
		 FileIn.read(reinterpret_cast<char*>(&any), sizeof(size_t));
		 FileIn.read(reinterpret_cast<char*>(&snx), sizeof(size_t));
		 FileIn.read(reinterpret_cast<char*>(&sny), sizeof(size_t));
 
		 if(!FileIn){
			 std::cerr << "Failed to read header from file: " << domInName << std::endl;
			 FileIn.close();
			 isFireActive = false;
			 return;
		 }
		 size_t cntCell = 0;
		 // Vector to store all cells to be loaded
		 std::vector<CellData> activeCells;
		 // Read cells until the end of the file
		 while(FileIn.tellg() < static_cast<std::streampos>(domFsize)){
			 CellData cell;
			 // Read cell metadata: localx, localy, nx, ny, nz, nt
			 FileIn.read(reinterpret_cast<char*>(&cell.localx), sizeof(size_t));
			 FileIn.read(reinterpret_cast<char*>(&cell.localy), sizeof(size_t));
			 FileIn.read(reinterpret_cast<char*>(&cell.nx), sizeof(size_t));
			 FileIn.read(reinterpret_cast<char*>(&cell.ny), sizeof(size_t));
			 FileIn.read(reinterpret_cast<char*>(&cell.nz), sizeof(size_t));
			 FileIn.read(reinterpret_cast<char*>(&cell.nt), sizeof(size_t));
 
			 if(!FileIn){
				 std::cerr << "Failed to read cell metadata from file: " << domInName << std::endl;
				 break;
			 }
			 // Calculate the size of the data array
			 size_t data_size = cell.nx * cell.ny * cell.nz * cell.nt;
 
			 // Read the data array
			 cell.data.resize(data_size);
			 FileIn.read(reinterpret_cast<char*>(cell.data.data()), data_size * sizeof(double));
 
			 if(!FileIn){
				 std::cerr << "Failed to read cell data from file: " << domInName << std::endl;
				 break;
			 }
			 // Store the cell data for later processing
			 activeCells.push_back(std::move(cell));
			 cntCell++;
		 }
 
		 // Calculate leftover bytes (if any)
		 FileIn.seekg(0, std::ios::end);
		 size_t final_pos = FileIn.tellg();
		 size_t total_read = final_pos - size_of_header;
		 size_t expected_read = 0;
		 for(const auto& cell : activeCells){
			 expected_read += 6 * sizeof(size_t) + cell.data.size() * sizeof(double);
		 }
		 size_t leftover = total_read - expected_read;
 
		 if(leftover != 0){
			 std::cerr << "CELL READ PROBLEM DATA LEFT IN Domain ID " << getDomainID()
					   << " bytes " << leftover << std::endl;
			 isFireActive = false;
			 FileIn.close();
			 return;
		 }
 
		 // Close the file as reading is complete
		 FileIn.close();
 
		 isFireActive = (cntCell > 0);
		 // Populate the cells with the loaded data
		 for(const auto& cell : activeCells){
			 cells[cell.localx][cell.localy].setBMapValues(cell.data.data());
		 }
	
	 }
 }
 
 
  */
	void FireDomain::loadCellsInBinary(){
		 if(getDomainID()>0){
			 size_t size_of_header = 4*sizeof(size_t);
			 size_t size_of_cell = 6*sizeof(size_t)+(localBMapSizeX*localBMapSizeX*sizeof(double));
			 
			 string domInName(params->getParameter("caseDirectory")+'/'+params->getParameter("PPath")+'/'+to_string(FireDomain::atmoIterNumber%2)+"/"+to_string(getDomainID())+".bmapcells");
			 size_t domFsize = fileSize(domInName);
			 if(domFsize < size_of_header){
				  return;
				  }
				 if(domFsize >size_of_header){
					 isFireActive=true;
					 size_t	cntCell = (domFsize-size_of_header)/size_of_cell;
					 size_t	leftover = (domFsize-size_of_header)%size_of_cell; 
					 if(leftover == 0 ){
 
					  ifstream FileIn(domInName.c_str(), ios_base::binary);
					 size_t nni;
					 size_t nnj;
					 size_t nnx;
					 size_t nny;
					 size_t nnz;
					 size_t nnt; 
 
					 FileIn.read((char *)&nnx, sizeof(size_t));
					 FileIn.read((char *)&nny, sizeof(size_t));
					 FileIn.read((char *)&nnz, sizeof(size_t));
					 FileIn.read((char *)&nnt, sizeof(size_t));
	 
					 for ( size_t ci = 0; ci < cntCell; ci++ ) {
							 FileIn.read((char *)&nni, sizeof(size_t));
							 FileIn.read((char *)&nnj, sizeof(size_t));					
							 cells[nni][nnj].loadBin(FileIn);
						 }
						 FileIn.close();
					 }else{
						  cout<<"CELL READ PROBLEM DATA LEFT IN "<< getDomainID()<<" bytes "<<leftover<<endl;
					 }
				 }else{
					 if(domFsize < size_of_header){
					  cout<<"CELL READ PROBLEM IN "<< getDomainID()<<" bytes "<<domFsize<<endl;
					 }
					 isFireActive=false;
				 }
			 }
	 }
	 
	 void FireDomain::dumpCellsInBinary(){
		 if (params->getParameter("runmode") == "masterMNH"){
			 if(getDomainID()==0){
			 
						 string domOutPattern(params->getParameter("caseDirectory")+'/'+params->getParameter("PPath")+'/'+to_string((FireDomain::atmoIterNumber+1)%2)+"/");	
					 
					 //	string domOutPattern(params->getParameter("caseDirectory")+'/'+params->getParameter("fireOutputDirectory")+'/'+params->getParameter("outputFiles")+".");
						 list<distributedDomainInfo*>::iterator it;
						 for (it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); it++)
							 {
								 size_t anx = (*it)->atmoNX;
								 size_t any = (*it)->atmoNY;
								 size_t snx = (*it)->atmoNX*localBMapSizeX;
								 size_t sny = (*it)->atmoNY*localBMapSizeY;
								 size_t rnx = (*it)->refNX;
								 size_t rny = (*it)->refNY;
								 size_t localx = 0;
								 size_t localy = 0;
								 ofstream FileOut;
								 size_t cntCell = 0;
 
								 for ( size_t i = rnx; i < rnx+anx; i++ ) {
									 localx = (i-rnx);
									 for ( size_t j = rny; j < rny+any; j++ ) {
										 localy = (j-rny);
												 if(cells[i][j].isActiveForDump()){
													 if(cntCell == 0){
														 FileOut.open((domOutPattern+to_string((*it)->ID)+".bmapcells").c_str(), ios_base::binary|ios_base::trunc  );
														 
														 FileOut.write(reinterpret_cast<const char*>(&anx), sizeof(size_t));
														 FileOut.write(reinterpret_cast<const char*>(&any), sizeof(size_t));
														 FileOut.write(reinterpret_cast<const char*>(&snx), sizeof(size_t));
														 FileOut.write(reinterpret_cast<const char*>(&sny), sizeof(size_t));
													 }
													 
													 FileOut.write(reinterpret_cast<const char*>(&localx), sizeof(size_t));
													 FileOut.write(reinterpret_cast<const char*>(&localy), sizeof(size_t));
													 cells[i][j].getBurningMap()->getMap()->dumpBin(FileOut);
													 cells[i][j].setIfAllDumped();
													 cntCell++;
												 }
									 }
								 }
								 if(cntCell ==0){
									 FileOut.flush();   
									 FileOut.rdbuf()->pubsync(); 	
									 FileOut.close();
									 }
								 
						 }		
				 
			 }
		 }
	 }
 
	 size_t FireDomain::countActiveCellsInDispatchDomain(size_t forID) {	
		 // Early exit if not in "masterMNH" run mode or if Domain ID is 0
		 if (params->getParameter("runmode") != "masterMNH") return 0;
		 if (getDomainID() != 0) return 0;
		 // Initialize pointer to store the found domain
		 distributedDomainInfo* found = getParallelDomainInfo(forID);
		 // If the domain with the specified ID is not found, return 0 and log an error
		 if (!found) {
			 std::cerr << "Domain ID " << forID << " not found in parallelDispatchDomains." << std::endl;
			 return 0;
		 }
		 
		 // Extract domain-specific parameters
		 size_t anx = found->atmoNX;
		 size_t any = found->atmoNY;
		 size_t rnx = found->refNX;
		 size_t rny = found->refNY;
 
		 size_t ncell = 0; // Counter for active cells
 
		 // Iterate over the cells within the specified domain
		 for (size_t i = rnx; i < rnx + anx; ++i) {
			 for (size_t j = rny; j < rny + any; ++j) {
				 if (cells[i][j].isActiveForDump()) {
					 ncell++;
				 }
			 }
		 }
		 return ncell;
	 }
 
	 vector<double> FireDomain::getActiveBBoxLBRT() {
		double left   = std::numeric_limits<double>::infinity();
		double bottom = std::numeric_limits<double>::infinity();
		double right  = -std::numeric_limits<double>::infinity();
		double top    = -std::numeric_limits<double>::infinity();
	
		for (size_t i = 0; i < atmoNX; i++) {
			for (size_t j = 0; j < atmoNY; j++) {
				if (cells[i][j].isActive()) {
					// Update left and bottom from the southwest corner.
					left   = min(left, cells[i][j].getSWCorner().getX());
					bottom = min(bottom, cells[i][j].getSWCorner().getY());
	
					// Update right and top from the northeast corner.
					right  = max(right, cells[i][j].getNECorner().getX());
					top    = max(top, cells[i][j].getNECorner().getY());
				}
			}
		}
	
		return {left, bottom, right, top};
	}
 /*
	 void FireDomain::dumpCellsInBinary(){
		 // Check if the run mode is "masterMNH" and the domain ID is 0
		 if (params->getParameter("runmode") == "masterMNH" && getDomainID() == 0){
			 // Construct the output directory pattern
			 std::string domOutPattern = params->getParameter("caseDirectory") + '/' +
										 params->getParameter("PPath") + '/' +
										 std::to_string((FireDomain::atmoIterNumber + 1) % 2) + "/";
 
			 // Iterate over all distributed domains
			 for(auto it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); ++it){
				 distributedDomainInfo* domainInfo = *it;
				 size_t anx = domainInfo->atmoNX;
				 size_t any = domainInfo->atmoNY;
				 size_t snx = anx * localBMapSizeX;
				 size_t sny = any * localBMapSizeY;
				 size_t rnx = domainInfo->refNX;
				 size_t rny = domainInfo->refNY;
				 size_t domainID = domainInfo->ID;
 
				 DistributedDomainBCellList domainBCellList;
				 domainBCellList.ID = domainID;
 
				 // Collect active cells for this domain
				 for(size_t i = rnx; i < rnx + anx; ++i){
					 size_t localx = i - rnx;
					 for(size_t j = rny; j < rny + any; ++j){
						 size_t localy = j - rny;
						 if(cells[i][j].isActiveForDump()){
							 CellData cell;
							 cell.localx = localx;
							 cell.localy = localy;
 
							 // Retrieve burning map information
							 FFArray<double>* burningMap = cells[i][j].getBurningMap()->getMap();
							 cell.nx = burningMap->getDim("x");
							 cell.ny = burningMap->getDim("y");
							 cell.nz = burningMap->getDim("z");
							 cell.nt = burningMap->getDim("t");
 
							 // Copy data from FFArray
							 size_t dataSize = burningMap->getSize();
							 
							 cell.data.resize(dataSize);
							 std::copy(burningMap->getData(), burningMap->getData() + dataSize, cell.data.begin());
 
							 // Add to the domain's cell list
							 domainBCellList.cells.push_back(cell);
						 }
					 }
				 }
 
				 // Update the number of active cells
				 domainBCellList.numActiveCells = domainBCellList.cells.size();
 
				 // Write to file if there are active cells
				 if(domainBCellList.numActiveCells > 0){
					 std::string fileName = domOutPattern + std::to_string(domainID) + ".bmapcells";
					 std::ofstream FileOut(fileName, std::ios::binary | std::ios::trunc);
					 
					 if(FileOut.is_open()){
						 // Write ID and number of active cells
						 FileOut.write(reinterpret_cast<const char*>(&anx), sizeof(size_t));
						 FileOut.write(reinterpret_cast<const char*>(&any), sizeof(size_t));
						 FileOut.write(reinterpret_cast<const char*>(&snx), sizeof(size_t));
						 FileOut.write(reinterpret_cast<const char*>(&sny), sizeof(size_t));
 
 
						 // Write each active cell
						 for(const auto& cell : domainBCellList.cells){
							 FileOut.write(reinterpret_cast<const char*>(&cell.localx), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(&cell.localy), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(&cell.nx), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(&cell.ny), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(&cell.nz), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(&cell.nt), sizeof(size_t));
							 FileOut.write(reinterpret_cast<const char*>(cell.data.data()), cell.data.size() * sizeof(double));
						 }
 
						 FileOut.close();
					 }
					 else{
						 std::cerr << "Failed to open file for writing: " << fileName << std::endl;
					 }
				 }
			 }
		 }
	 }
 */
 FireDomain::distributedDomainInfo* FireDomain::getParallelDomainInfo(size_t forID) {
	 if (getDomainID() != 0) return nullptr;
	 if (params->getParameter("runmode") != "masterMNH") return nullptr;
 
	 distributedDomainInfo* found = nullptr;
	 for (std::list<distributedDomainInfo*>::iterator it = parallelDispatchDomains.begin(); 
		  it != parallelDispatchDomains.end(); 
		  ++it) 
	 {
		 if ((*it)->ID == forID) {
			 found = *it;
		 }
	 }
 
	 if (found != nullptr) {
		 return found;
	 }
 
	 return nullptr;
 }
 
 
 void FireDomain::loadWindDataInBinary(double refTime){
			 if(getDomainID()!=0)return;
			 if (params->getParameter("runmode") != "masterMNH") return;
	 
		 if(parallelDispatchDomains.size() >0){
			 size_t refNXs[parallelDispatchDomains.size()];
			 size_t refNYs[parallelDispatchDomains.size()];
			 list<distributedDomainInfo*>::iterator it;
			 size_t numreadDom = 0;
			 for (it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); it++)
				 {
						 refNXs[(*it)->ID-1] =  (*it)->refNX;
						 refNYs[(*it)->ID-1] =  (*it)->refNY;
						 numreadDom++;
			 }
			 dataBroker->loadMultiWindBin(refTime,numreadDom,refNXs,refNYs);
		 }
 }
 
	 
	 // finding the cell with the the location
	 FDCell* FireDomain::getCell(FFPoint p){
		 return getCell(p.getX(), p.getY());
	 }
	 FDCell* FireDomain::getCell(FireNodeData* fnd){
		 return getCell(fnd->posX, fnd->posY);
	 }
	 FDCell* FireDomain::getCell(const double& px, const double& py){
		 
		 if ( !striclyWithinDomain(px, py) ) return trashCell;
		 
		 double di, dj;
		 di = (px-SWCornerX())*inverseCellSizeX;
		 dj = (py-SWCornerY())*inverseCellSizeY;
		 
		 if ( di < EPSILONX ) di = 0;
		 if ( di > atmoNX - EPSILONX ) di = atmoNX - 1;
		 if ( dj < EPSILONX ) dj = 0;
		 if ( dj > atmoNY - EPSILONX ) dj = atmoNY - 1;
 
		 size_t i = (size_t) di;
		 size_t j = (size_t) dj;
		 return &(cells[i][j]);
 
	 }
 
	 list<FDCell*> FireDomain::getProxCells(FDCell* cell, int range){
		 list<FDCell*> proxCells;
		 int i = cell->getI();
		 int j = cell->getJ();
		 for ( int ii=-range; ii<range+1; ii++ ){
			 // north cells
			 if (getCell(i+ii,j+range)) proxCells.push_back(getCell(i+ii,j+range));
			 // south cells
			 if (getCell(i+ii,j-range)) proxCells.push_back(getCell(i+ii,j-range));
		 }
		 for ( int jj=-range+1; jj<range; jj++ ){
			 // east cells
			 if (getCell(i+range,j+jj)) proxCells.push_back(getCell(i+range,j+jj));
			 // west cells
			 if (getCell(i-range,j+jj)) proxCells.push_back(getCell(i-range,j+jj));
		 }
		 return proxCells;
	 }
 
	 bool FireDomain::isInInnerHalo(FireNode* fn){
		 if ( !fn or fn->getState() == FireNode::final  ) return false;
		 FDCell* cell = getCell(fn);
		 if ( cell->getI() == 1 or cell->getI() == atmoNX-2 ) return true;
		 if ( cell->getJ() == 1 or cell->getJ() == atmoNY-2 ) return true;
		 return false;
	 }
 
	 bool FireDomain::isInInnerHalo(const FFPoint& loc){
		 FDCell* cell = getCell(loc);
		 if ( cell->getI() == 0 or cell->getI() == atmoNX-1 ) return false;
		 if ( cell->getJ() == 0 or cell->getJ() == atmoNY-1 ) return false;
		 if ( cell->getI() == 1 or cell->getI() == atmoNX-2 ) return true;
		 if ( cell->getJ() == 1 or cell->getJ() == atmoNY-2 ) return true;
		 return false;
	 }
 
	 bool FireDomain::isInOuterHalo(FDCell* cell){
		 if ( cell->getI() == 0 or cell->getI() == atmoNX-1 ) return true;
		 if ( cell->getJ() == 0 or cell->getJ() == atmoNY-1 ) return true;
		 return false;
	 }
 
	 bool FireDomain::isInOuterHalo(FireNode* fn){
		 if ( fn == 0 or fn->getState() == FireNode::final  ) return false;
		 FFPoint tloc = fn->getLoc();
		 return isInOuterHalo(tloc);
	 }
 
	 bool FireDomain::isInOuterHalo(FFPoint& loc){
		 return isInOuterHalo(loc.getX(), loc.getY());
	 }
 
	 bool FireDomain::isInOuterHalo(const double& px, const double& py){
		 FDCell* cell = getCell(px, py);
		 if ( cell->getI() == 0 or cell->getI() == atmoNX-1 ) return true;
		 if ( cell->getJ() == 0 or cell->getJ() == atmoNY-1 ) return true;
		 return false;
	 }
 
	 // Finding the firenodes within a given distance from a given firenode
	 void FireDomain::getPotentialMergingNodes(FireNode* fn
											   , const double& dist){
		 closeNodes.clear();
		 distances.clear();
		 min_position = 0;
		 double min_dist = dist;
		 double d;
		 
		 
		 FDCell* curCell = getCell(fn);
 
		 //cout<<getDomainID()<<" scanning  "<<fn->getLoc().getX()<<":"<<fn->getLoc().getY()<<endl;
	 
		 FFPoint center = 0.5*(curCell->getSWCorner() + curCell->getNECorner());
		 double dx = curCell->getNECorner().getX() - curCell->getSWCorner().getX();
		 double dy = curCell->getNECorner().getY() - curCell->getSWCorner().getY();
		 FFPoint vdx = FFPoint(dx, 0.,0);
		 FFPoint vdy = FFPoint(0., dy,0);
		 FDCell* searchedCell;
		 int numCellsX, numCellsY;
		 numCellsX = (int) dist/dx + 1;
		 numCellsY = (int) dist/dy + 1;
		 // Scanning the current cell and neighbors for close enough firenodes
		 list<FireNode*>::iterator ofn;
		 FFPoint rep;
		 
		 for ( int i=-numCellsX; i<numCellsX+1; i++ ){
			 for ( int j=-numCellsY; j<numCellsY+1; j++ ){
				 rep = center + i*vdx + j*vdy;
				 searchedCell = getCell(rep);
				 if ( searchedCell == trashCell ) continue;
		 
				 for ( ofn = searchedCell->fireNodes.begin();
					  ofn != searchedCell->fireNodes.end(); ++ofn ) {
					 if ( *ofn != fn ){
						 if ( (*ofn)->mergeAllowed() ){
							 d = fn->distance((*ofn)->locAtTime(fn->getTime()));
							 if ( d < dist ) {
								 closeNodes.push_back(*ofn);
								 distances.push_back(d);
								 if ( d < min_dist ) {
									 min_position = closeNodes.size()-1;
									 min_dist = d;
								 }
							 }
						 }
					 }
				 }
			 }
		 }
		 
	 }
 
	 /* Finding the firenodes, in the physical domain,
	  * within a given distance from a given firenode */
	 list<FireNode*> FireDomain::getPhysicalDomainNodesWithin(FFPoint& loc
															  , const double& dist, bool stateDep){
		 list<FireNode*> cnodes;
		 FDCell* curCell = getCell(loc);
		 FFPoint center = 0.5*(curCell->getSWCorner() + curCell->getNECorner());
		 FFPoint dx = FFPoint(curCell->getNECorner().getX() - curCell->getSWCorner().getX(),0.,0.);
		 FFPoint dy = FFPoint(0.,curCell->getNECorner().getY() - curCell->getSWCorner().getY(),0.);
		 FDCell* searchedCell;
		 int numCellsX, numCellsY;
		 numCellsX = ((int) dist/(curCell->getNECorner().getX()-curCell->getSWCorner().getX())) + 1;
		 numCellsY = ((int) dist/(curCell->getNECorner().getY()-curCell->getSWCorner().getY())) + 1;
		 // Scanning the current cell and neighbors for close enough firenodes
		 list<FireNode*>::iterator ofn;
		 FFPoint rep;
		 for ( int i=-numCellsX; i<numCellsX+1; i++ ){
			 for ( int j=-numCellsY; j<numCellsY+1; j++ ){
				 rep = center + i*dx + j*dy;
				 searchedCell = getCell(rep);
				 if ( searchedCell == trashCell or isInOuterHalo(searchedCell) ) continue;
				 for ( ofn = searchedCell->fireNodes.begin();
					  ofn != searchedCell->fireNodes.end(); ++ofn ) {
					 if ( !stateDep or ((*ofn)->getState()!=FireNode::final
										and (*ofn)->getState()!=FireNode::link) )  {
						 if (loc.distance((*ofn)->getLoc()) < dist ) cnodes.push_back(*ofn);
					 }
				 }
			 }
		 }
		 return cnodes;
	 }
 
	 list<FireNode*> FireDomain::getPhysicalDomainNodesWithin(FireNodeData* fnd
															  , const double& dist, bool stateDep){
		 FFPoint loc(fnd->posX, fnd->posY,0.);
		 return getPhysicalDomainNodesWithin(loc, dist, stateDep);
	 }
 
	 // Checking the topology around a firenode
	 void FireDomain::checkTopology(FireNode* fn){
 
		 /* Detecting the possible errors in the topology */
		 /* --------------------------------------------- */
		 // looking for possible outcomes with previous and next
		 if ( fn->getPrev() == 0 and fn->getNext() == 0 ){
			 debugOutput<<getDomainID()<<": WARNING, topological problem for "
					 <<fn->toString()<<" (no previous nor next)"<<endl;
			 addToTrashNodes(fn);
			 return;
		 }
		 if ( fn->getPrev() == 0 or fn->getNext() == 0 ){
			 debugOutput<<getDomainID()<<": WARNING, topological problem for "
					 <<fn->toString()<<" (no previous or next)"<<endl;
			 return;
		 }
		 if ( fn->getPrev() == fn->getNext() ){
			 debugOutput<<getDomainID()<<": WARNING, topological problem for "
					 <<fn->toString()<<" (previous == next)"<<endl;
			 if ( fn->getPrev() == fn ){
				 addToTrashNodes(fn);
				 return;
			 } else {
				 if ( fn->getPrev()->getNext() == fn ){
					 fn ->setFront(0);
					 fn->getPrev()->setFront(0);
					 addToTrashNodes(fn->getPrev());
					 addToTrashNodes(fn);
					 return;
				 } else {
					 debugOutput<<getDomainID()<<": PROBLEM, serious topology problem for "
							 <<fn->toString()<<endl;
					 return;
				 }
			 }
		 }
 
		 // checking the effective linking of the marker ith the others
		 //bool correctlyLinked = true;
		 size_t maxCounts = 100000;
		 size_t nmarker = 0;
		 FireNode* tmpfn = fn->getNext();
		 while ( tmpfn != fn and tmpfn != 0 and nmarker < maxCounts ) tmpfn = tmpfn->getNext();
		 //if ( tmpfn == 0 or nmarker == maxCounts ) correctlyLinked = false;
		 nmarker = 0;
		 tmpfn = fn->getPrev();
		 while ( tmpfn != fn and tmpfn != 0 and nmarker < maxCounts ) tmpfn = tmpfn->getPrev();
		 //if ( tmpfn == 0 or nmarker == maxCounts ) correctlyLinked = false;
 
		 /* Detecting the changes needed in the topology */
		 /* -------------------------------------------- */
		 if ( !safeTopologyMode ) {
 
			 double d = getPerimeterResolution();
 
			 /* Detecting possible merging */
			 if ( fn->mergeAllowed() ){
 
				 /* computing the new firenode proximity list */
				 getPotentialMergingNodes(fn, d);
				 if ( closeNodes.size() >= 1 ) {
				 
					 // there exists a firenode too close -> merging
					 FireNode* fnb = closeNodes[min_position];
					 fn->setMerging(fnb);
					 return;
				 }
			 }
 
			 /* checking the distance with the neighbor markers for splitting */
			 if ( fn->getNext() == 0 ){
				 debugOutput<<getDomainID()<<": Asking to check topology"
						 <<"with no next for "<<fn->toShort()<<endl;
				 ostringstream where;
				 where<<"FireDomain::checkTopology for processor "<<getDomainID()<<endl;
				 throw TopologicalException(debugOutput.str(), where.str());
			 }
			 if ( fn->splitAllowed() or( fn->getState() == FireNode::final
							  and fn->getNext()->splitAllowed() )){
				 double distToNext = fn->distance(fn->getNext()->locAtTime(fn->getTime()));
				 if ( distToNext > 2.*d ) fn->setSplitting();
			 }
 
			 if ( (fn->getPrev()->getState() == FireNode::final or
							   fn->getPrev()->splitAllowed()) and fn->splitAllowed() ){
				 double distToPrev = fn->getPrev()->distance(fn->getLoc());
				 if ( distToPrev > 2.*d ){
					 fn->getFront()->split(fn->getPrev(), fn->getTime());
				 }
			 }
 
		 }
	 }
 
	 void FireDomain::merge(FireNode* fna, FireNode* fnb){
	 
		 if ( fna->getFront() == fnb->getFront() ) {
			 /* Merging firenodes from the same front */
			 FireFront* tmpfront = fna->getFront();
			 tmpfront->merge(fna, fnb);
		 } else if ( fna->getContFront() == fnb->getContFront() ) {
			 FireFront* tmpfront = fna->getContFront();
			 /* Merging inner fronts */
			 tmpfront->mergeInnerFronts(fna, fnb);
		 }
	 }
 
	 void FireDomain::addToTrashNodes(FireNode* fn){
		 debugOutput<<getDomainID()<<": trashing "<<fn->toShort()<<endl;
		 trashNodes.push_back(fn);
		 removeFireNodeInCell(fn);
		 fn->makeTrash();
	 }
 
	 void FireDomain::stopOutgoingNode(FireNode* fn, FFPoint& loc, double& t){
		 if ( fn->getDomainID() == getDomainID() ){
			 debugOutput<<getDomainID()<<": stopping outgoing "<<fn->toShort()<<endl;
			 fn->setNextLoc(loc);
			 updateFireNodeInCells(fn);
			 fn->setLoc(loc);
			 fn->setState(FireNode::final);
		 } else {
			 debugOutput<<getDomainID()<<": FireDomain::stopOutgoingNode -> ";
			 addToTrashNodes(fn);
		 }
	 }
 
	 void FireDomain::addToTrashFronts(FireFront* ff){
		 ff->makeTrash();
		 trashFronts.push_back(ff);
	 }
 
	 FireNode* FireDomain::FireNodeFactory(){
		 if(recycleNodes){
			 if( !trashNodes.empty() ){
				 FireNode* newfn = trashNodes.back();
				 trashNodes.pop_back();
				 return newfn;
			 }
		 }
 
		 FireNode* newfn = new FireNode(this);
		 createdNodes.push_back(newfn);
		 return newfn;
	 }
 
	 FireNode* FireDomain::addFireNode(FireNodeData* fnd
									   , FireFront* ff, FireNode* prevNode){
		 // Adding the firenode in the prescribed front
		 // if no front, it is a node coming from a parallelization process
		 FireNode* newfn = FireNodeFactory();
		 newfn->initialize(fnd, this, ff, prevNode);
		 // Making the firenode active in the simulation (if needed)
		 if ( newfn->getState() == FireNode::init ) addNewAtomToSimulation(newfn);
		 return newfn;
	 }
 
	 FireNode* FireDomain::addFireNode(FFPoint& loc, FFVector& vel
									   , double t, double fDepth, double kappa
									   , FireFront* ff, FireNode* prevNode
									   , int dom, int id, FireNode::State state){
		 // Adding the firenode in the prescribed front
		 // if no front, it is a node coming from a parallelization process
		 FireNode* newfn = FireNodeFactory();
	 
		 newfn->initialize(loc, vel, t, fDepth, kappa, this, ff, prevNode); 
		 newfn->setState(state);
		 // If a domain and and id is specified, imposing it
		 if ( dom != 0 and id != 0 ) newfn->setID(dom, id);
		 // Making the firenode active in the simulation, if needed
		 if ( state != FireNode::link and state != FireNode::final ) addNewAtomToSimulation(newfn);
		 return newfn;
	 }
 
	 FireNode* FireDomain::addLinkNode(FFPoint& loc, FireFront* ff, FireNode* prevNode){
		 /* adding a link node (not included in simulator) */
		 FireNode* newfn = FireNodeFactory();
		 FFVector vel = FFVector();
		 double t = 0.;
		 double fdepth = 0.;
		 double kappa = 0.;
		 newfn->initialize(loc, vel, t, fdepth, kappa, this, ff, prevNode);
		 newfn->setState(FireNode::link);
		 newfn->setID(0);
		 linkNodes.push_back(newfn);
		 return newfn;
	 }
 
	 FireFront* FireDomain::FireFrontFactory(){
 
		 if (recycleFronts){
			 if ( trashFronts.size() > 0 ){
				 FireFront* newff = trashFronts.back();
				 trashFronts.pop_back();
				 return newff;
			 }
		 }
		 FireFront* newff = new FireFront(this);
		 return newff;
 
	 }
 
	 FireFront* FireDomain::addFireFront(double t, FireFront* ff){
		 FireFront* curff;
		 if ( ff ){
			 curff = ff;
		 } else {
			 curff = domainFront;
		 }
		 FireFront* newff = FireFrontFactory();
		 newff->initialize(t, curff);
		 return newff;
	 }
 
	 // Inserting a new event for an existing atom
	 FFEvent* FireDomain::addNewEvent(ForeFireAtom* atom
									  , double time, string type){
		 FFEvent* newEv = new FFEvent(atom, time, type);
		 schedule->insert(newEv);
		 return newEv;
	 }
 
	 FFPoint FireDomain::findIntersectionWithFrontiers(
													   FFPoint& pointA, FFPoint& pointB){
		 /* Finding the intersection with frontiers of the domain */
		 FFPoint inter = outPoint;
  
		 list<Frontier*>::iterator frontier = frontiers.begin();
		 while ( inter == outPoint and frontier != frontiers.end() ){
			 inter = findIntersection(pointA, pointB
									  , (*frontier)->pointA, (*frontier)->pointB);
			 ++frontier;
		 }
		 if ( inter == outPoint ) debugOutput<<getDomainID()<<": WARNING, "
			 <<"did not find intersection with frontiers for "
			 <<pointA.print()<<"->"<<pointB.print()<<endl;
		 return inter;
	 }
 
	 FFPoint FireDomain::findIntersectionWithFrontiers(
													   FireNodeData* fnda, FireNodeData* fndb){
		 FFPoint pointA(fnda->posX, fnda->posY,0.);
		 FFPoint pointB(fndb->posX, fndb->posY,0.);
		 return findIntersectionWithFrontiers(pointA, pointB);
	 }
 
	 FFPoint FireDomain::findIntersectionWithInnerFrontiers(
															FFPoint& pointA, FFPoint& pointB){
		 /* Finding the intersection with inner frontiers of the domain */
		 FFPoint inter = outPoint;
		 list<Frontier*>::iterator frontier = infrontiers.begin();
		 while ( inter == outPoint and frontier != infrontiers.end() ){
			 inter = findIntersection(pointA, pointB
									  , (*frontier)->pointA, (*frontier)->pointB);
			 ++frontier;
		 }
		 return inter;
	 }
 
	 FFPoint FireDomain::findIntersectionWithInnerFrontiers(
															FireNodeData* fnda, FireNodeData* fndb){
		 FFPoint pointA(fnda->posX, fnda->posY,0.);
		 FFPoint pointB(fndb->posX, fndb->posY,0.);
		 return findIntersectionWithInnerFrontiers(pointA, pointB);
	 }
 
	 FFPoint FireDomain::findIntersectionWithBoundingBox(
														 FFPoint& pointA, FFPoint& pointB
														 , FFPoint& swc, FFPoint& nec, int& seg){
		 FFPoint nwc = FFPoint(swc.getX(), nec.getY(),0.);
		 FFPoint sec = FFPoint(nec.getX(), swc.getY(),0.);
		 FFPoint inter = findIntersection(pointA, pointB, swc, nwc);
		 seg = 0;
		 if ( inter == outPoint ){
			 inter = findIntersection(pointA, pointB, nwc, nec);
			 seg = 1;
		 }
		 if ( inter == outPoint ){
			 inter = findIntersection(pointA, pointB, nec, sec);
			 seg = 2;
		 }
		 if ( inter == outPoint ){
			 inter = findIntersection(pointA, pointB, sec, swc);
			 seg = 3;
		 }
		 if ( inter == outPoint ){
			 cout<<getDomainID()<<": WARNING, did not find intersection with a bounding box"<<endl;
		 }
		 return inter;
	 }
 
	 FFPoint FireDomain::findIntersection(FFPoint& A, FFPoint& B
										  , FFPoint& C, FFPoint& D){
		 double Ax = A.getX();
		 double Ay = A.getY();
		 double Bx = B.getX();
		 double By = B.getY();
		 double Cx = C.getX();
		 double Cy = C.getY();
		 double Dx = D.getX();
		 double Dy = D.getY();
 
		 double Sx;
		 double Sy;
		 double eps = EPSILONX;
		 if ( Ax == Bx ) {
			 if ( Cx == Dx ){
				 return outPoint;
			 } else {
				 double pCD = (Cy-Dy)/(Cx-Dx);
				 Sx = Ax;
				 Sy = pCD*(Ax-Cx)+Cy;
			 }
		 } else {
			 if ( Cx == Dx ){
				 double pAB = (Ay-By)/(Ax-Bx);
				 Sx = Cx;
				 Sy = pAB*(Cx-Ax)+Ay;
			 } else {
				 double pCD = (Cy-Dy)/(Cx-Dx);
				 double pAB = (Ay-By)/(Ax-Bx);
				 double oCD = Cy-pCD*Cx;
				 double oAB = Ay-pAB*Ax;
				 if( abs(pCD-pAB) < eps ) return outPoint;
				 Sx = (oAB-oCD)/(pCD-pAB);
				 Sy = pCD*Sx+oCD;
			 }
		 }
 
		 if( (Sx<Ax-eps && Sx<Bx-eps)|(Sx>Ax+eps && Sx>Bx+eps)
			| (Sx<Cx-eps && Sx<Dx-eps)|(Sx>Cx+eps && Sx>Dx+eps)
			| (Sy<Ay-eps && Sy<By-eps)|(Sy>Ay+eps && Sy>By+eps)
			| (Sy<Cy-eps && Sy<Dy-eps)|(Sy>Cy+eps && Sy>Dy+eps)){
			 return outPoint;
		 } else {
			 return FFPoint(Sx,Sy,0.);
		 }
	 }
 
	 bool FireDomain::firenodeInList(FireNode* sfn
									 , const list<FireNode*>& hlist){
		 list<FireNode*>::const_iterator fn;
		 for ( fn = hlist.begin(); fn != hlist.end(); ++fn ){
			 if ( *fn == sfn ) return true;
		 }
		 return 0;
	 }
 
	 FireNodeData* FireDomain::idInList(const double& tid
										, const list<FireNodeData*>& hlist){
		 list<FireNodeData*>::const_iterator fn;
		 for ( fn = hlist.begin(); fn != hlist.end(); ++fn ){
			 if ( (*fn)->id == tid ) return *fn;
		 }
		 return 0;
	 }
 
	 bool FireDomain::idInChain(const double& tid
								, const list<FireNodeData*>& hlist){
		 if ( tid == 0 or hlist.size() == 0 ) return false;
		 return ( idInList(tid, hlist)
				 and !isPrevLinkDataInList(tid, hlist)
				 and !isNextLinkDataInList(tid, hlist) );
	 }
 
	 bool FireDomain::isPrevLinkDataInList(const double& tid
										   , const list<FireNodeData*>& hlist){
		 list<FireNodeData*>::const_iterator fn = hlist.begin();
		 list<FireNodeData*>::const_iterator next;
		 if ( (*fn)->id == tid ) return true;
		 ++fn;
		 while ( fn != hlist.end() ){
			 if ( (*fn)->time < endChain + EPSILONT ){
				 next = fn;
				 ++next;
				 if ( (*next)->id == tid ) return true;
			 }
			 ++fn;
		 }
		 return false;
	 }
 
	 bool FireDomain::isNextLinkDataInList(const double& tid
										   , const list<FireNodeData*>& hlist){
		 list<FireNodeData*>::const_iterator fn = hlist.begin();
		 list<FireNodeData*>::const_iterator prev;
		 while ( fn != hlist.end() ){
			 if ( (*fn)->time < endChain + EPSILONT ){
				 prev = fn;
				 --prev;
				 if ( (*prev)->id == tid ) return true;
			 }
			 ++fn;
		 }
		 return false;
	 }
 
	 FireNodeData* FireDomain::posInList(
										 const double& x, const double& y
										 , const list<FireNodeData*>& haloList){
		 list<FireNodeData*>::const_iterator fn;
		 for ( fn = haloList.begin(); fn != haloList.end(); ++fn ){
			 if ( (*fn)->posX == x and (*fn)->posY == y ) return *fn;
		 }
		 return 0;
	 }
 
 
	  
 
	 void FireDomain::commonInitialization(double* cellsMeshX, double* cellsMeshY
										   , const int& year, const int& yday){
 
 
		 /*------------------------------------*/
		 /* Handling the outputs for debugging */
		 /*------------------------------------*/
 
		 if ( getDomainID()==params->getInt("watchedProc")
			 or params->getInt("watchedProc") == -1 ) {
			 if ( params->getInt("CommandOutputs") != 0 ) commandOutputs = true;
			 if ( params->getInt("FireDomainOutputs") != 0 ) outputs = true;
			 if ( params->getInt("FireFrontOutputs") != 0 ) FireFront::outputs = true;
			 if ( params->getInt("FireNodeOutputs") != 0 ) FireNode::outputs = true;
			 if ( params->getInt("FDCellOutputs") != 0 ) FDCell::outputs = true;
		 }
		// std::cout<<"ID"<<getID()<< " la:"<< refLatitude<<" lo:"<<refLongitude<<" SOUTH:"<<params->getParameter("SOUTH")<<" WEST:"<<params->getParameter("WEST")<<" NORTH:"<<params->getParameter("NORTH")<<" EAST:"<<params->getParameter("EAST")<<endl;
 
		 if(params->getParameter("runmode") == "standalone"){
			 if (params->isValued("SWLngLat")){
				 refLongitude = params->getDoubleArray("SWLngLat")[0];
				 refLatitude = params->getDoubleArray("SWLngLat")[1];
			 }else if (params->isValued("BBoxWSEN")){
				 refLongitude = params->getDoubleArray("BBoxWSEN")[0];
				 refLatitude = params->getDoubleArray("BBoxWSEN")[1];
				 params->setParameter("SWLngLat", std::to_string(refLongitude)+","+std::to_string(refLatitude));
			 }else if (params->isValued("WSENLBRT")){
				 refLongitude = params->getDoubleArray("WSENLBRT")[0];
				 refLatitude = params->getDoubleArray("WSENLBRT")[1];
				 params->setParameter("SWLngLat", std::to_string(refLongitude)+","+std::to_string(refLatitude));
			 }else if (params->isValued("DefaultSWLngLat")){
				 refLongitude = params->getDoubleArray("DefaultSWLngLat")[0];
				 refLatitude = params->getDoubleArray("DefaultSWLngLat")[1];
				 params->setParameter("SWLngLat", std::to_string(refLongitude)+","+std::to_string(refLatitude));
			 }
		 }
		 if(params->getParameter("runmode") == "coupled"){
			 // in this case i'm the master domain in a meso-nh run, i have full Fire simulation, and coupled have most likely be called earlie
			 if(refLatitude*refLatitude < EPSILONV and refLongitude*refLongitude < EPSILONV){
				 //so basically i put it in default
				 // it is an idealized case... i have to take default values if none are provided
				 if(params->isValued("SWLngLat")){
					 refLongitude = params->getDoubleArray("SWLngLat")[0];
					 refLatitude = params->getDoubleArray("SWLngLat")[1];
				 }else{
					 refLongitude = params->getDoubleArray("DefaultSWLngLat")[0];
					 refLatitude = params->getDoubleArray("DefaultSWLngLat")[1];
				 }
			 }
			 params->setParameter("SWLngLat", std::to_string(refLongitude)+","+std::to_string(refLatitude));
			 
 
			 // in this case i'm a slave sub-domain in a meso-nh run
		 }
 
		 if(params->getParameter("runmode") == "masterMNH"){
			 // in this case i'm the master domain in a meso-nh run, i have full Fire simulation, and coupled have most likely be called earlier
			 refLongitude = params->getDoubleArray("SWLngLat")[0];
			 refLatitude = params->getDoubleArray("SWLngLat")[1];
			 // std::cout<<"ID"<<getID()<< " la:"<< refLatitude<<" lo:"<<refLongitude<< " mx:"<<cellsMeshX[0]<<" my:"<<cellsMeshY[0] <<std::endl;
		 }
 


 
		 /* domain front */
		 domainFront = FireFrontFactory();
		 mainFrontBackup = 0;
		 /* timetable */
		 schedule = 0;
		 /* boolean of parallel simulations */
		 parallel = (bool) params->getInt("parallel");
  
		 /* number of the iteration of the Atmospheric Model */
		 numIterationAtmoModel = 0;
		 /* heat flux treshold for considering a location burning */
		 burningTresholdFlux = params->getDouble("burningTresholdFlux");
		 /* trigger distance for the computation of front depth */
 
		 maxFrontDepth = params->getDouble("maxFrontDepth");
		 propagationSpeedAdjustmentFactor  = params->getDouble("propagationSpeedAdjustmentFactor");
		 safeTopologyMode = false;
 
		 /*------------------------------*/
		 /* Defining the date properties */
		 /*------------------------------*/
 
		 refYear = year;
		 params->setInt("refYear", year);
		 refDay = yday;
		 params->setInt("refDay", refDay);
 
		 /*----------------------------------------------------*/
		 /* Defining the front properties' computation schemes */
		 /*----------------------------------------------------*/
 
		 if ( params->isValued("normalScheme") )
			 FireNode::setNormalScheme(params->getParameter("normalScheme"));
		 if ( params->isValued("curvatureComputation") )
			 FireNode::setCurvatureComputation(params->getInt("curvatureComputation"));
		 if ( params->isValued("curvatureScheme") )
			 FireNode::setCurvatureScheme(params->getParameter("curvatureScheme"));
		 if ( params->isValued("frontDepthComputation") )
			 FireNode::setFrontDepthComputation(params->getInt("frontDepthComputation"));
		 if ( params->isValued("frontDepthScheme") )
			 setFrontDepthScheme(params->getParameter("frontDepthScheme"));
		 if ( params->isValued("smoothing") )
			 FireNode::setSmoothing(params->getDouble("smoothing"));
		 if ( params->isValued("relax") )
			 FireNode::setRelax(params->getDouble("relax"));
		 if ( params->isValued("minSpeed") ){
 
			 FireNode::setMinSpeed(params->getDouble("minSpeed"));
		 }
		 if ( params->isValued("minimalPropagativeFrontDepth") )
			 FireNode::setMinDepth(params->getDouble("minimalPropagativeFrontDepth"));
		 /*---------------------------------*/
		 /* Defining the spatial properties */
		 /*---------------------------------*/
 
		 /* Other corners */
		 NWCorner.setX(SWCornerX());
		 NWCorner.setY(NECornerY());
		 SECorner.setX(NECornerX());
		 SECorner.setY(SWCornerY());
 
		 /* Origin of mass points */
		 double dx = (NECornerX()-SWCornerX())/atmoNX;
		 double dy = (NECornerY()-SWCornerY())/atmoNY;
		 params->setDouble("massOriginX", SWCornerX()+dx/2.);
		 params->setDouble("massOriginY", SWCornerY()+dy/2.);
 
		 /* Computing the perimeter resolution according
		  * to the maximum number of firenodes in a cells */
		 inverseCellSizeX = 1./dx;
		 inverseCellSizeY = 1./dy;
 
		 /*----------------------------------*/
		 /* Spatial scales of the simulation */
		 /*----------------------------------*/
 
		 // Perimeter resolution
		 perimeterResolution = params->getDouble("perimeterResolution");
		 if ( parallel ){
			 params->setInt("numFNMax", atmoNZ);
			 double maxResolution = getSimulationMaxResolution(dx, dy, atmoNZ);
			 if ( perimeterResolution < maxResolution ){
				 if ( getDomainID()==1 ) cout<<"changing perimeter resolution to "
					 <<maxResolution<<" for parallel issues"<<endl;
				 params->setDouble("perimeterResolution", maxResolution);
				 perimeterResolution = maxResolution;
				 spatialCFL = spatialIncrement/perimeterResolution;
				 params->setDouble("spatialCFL", spatialCFL);
			 }
		 }
		 // Spatial increment
		 spatialIncrement = params->getDouble("spatialIncrement");
		 spatialCFLMax = params->getDouble("spatialCFLmax");
		 spatialCFL = spatialIncrement/perimeterResolution;
		 if ( spatialCFL > spatialCFLMax ){
			 spatialIncrement = spatialCFLMax*perimeterResolution;
			 spatialCFL = spatialCFLMax;
			 params->setDouble("spatialCFL", spatialCFL);
			 cout<<"changing spatial increment to "
			 <<spatialIncrement<<" to satisfy maximum spatial CFL condition"<<endl;
		 }
 
		 // Burning map resolution
		 BMapsResolution = getBurningMapResolution(spatialIncrement
														  , params->getDouble("minimalPropagativeFrontDepth"));
		 localBMapSizeX = (size_t) (dx/BMapsResolution );
		 if ( localBMapSizeX < 1 ){
			 cout<<"Warning, atmo (fluxes integration) cell has less than 1 BMap point given "
			 <<BMapsResolution<<" BMap resolution and "<< dx<< " atmo resolution, changing to 1"<<endl;
			 localBMapSizeX = 1;
		  }
		 params->setDouble("bmapResolution", BMapsResolution);
		 params->setSize("localBMapSizeX", localBMapSizeX);
		 localBMapSizeY = (size_t) (dy/BMapsResolution);
		 if ( localBMapSizeY < 1 ) {
						 cout<<"Warning, atmo (fluxes integration) cell has less than 1 BMap point given "
			 <<BMapsResolution<<" BMap resolution and "<< dx<< " atmo resolution, changing to 1"<<endl;
			 localBMapSizeY = 1;
			 }
		 params->setSize("localBMapSizeY", localBMapSizeY);
		 globalBMapSizeX = atmoNX*localBMapSizeX;
		 globalBMapSizeY = atmoNY*localBMapSizeY;
		 burningMatrixResX = (NECornerX()-SWCornerX())/globalBMapSizeX;
		 burningMatrixResY = (NECornerY()-SWCornerY())/globalBMapSizeY;
		 burningMatrixRes = max(burningMatrixResX, burningMatrixResY);
 
		 params->setDouble("matrixResolutionX", (NECornerX()-SWCornerX())/atmoNX);
		 params->setDouble("matrixResolutionY", (NECornerY()-SWCornerY())/atmoNY);
 
 
 
	 
		 /*--------------------*/
		 /* Defining the cells */
		 /*--------------------*/
 
		 size_t allocated_dim = 0;
		 try {
			 // allocating along the first dimension
			 cells = new FDCell*[atmoNX];
			 // second dimension
			 for ( allocated_dim = 0; allocated_dim < atmoNX; ++allocated_dim) {
				 cells[allocated_dim] = new FDCell[atmoNY];
			 }
			 FFPoint CellSWCorner, CellNECorner;
			 for ( size_t i = 0; i < atmoNX; i++ ) {
				 for ( size_t j = 0; j < atmoNY; j++ ) {
					 // setting the parent Fire domain
					 cells[i][j].setDomain(this);
					 // computing the corners of the cell
					 CellSWCorner.setX(cellsMeshX[i]);
					 CellSWCorner.setY(cellsMeshY[j]);
					 CellNECorner.setX(cellsMeshX[i+1]);
					 CellNECorner.setY(cellsMeshY[j+1]);
					 cells[i][j].setMatrixSize(localBMapSizeX, localBMapSizeY);
					 cells[i][j].setCorners(CellSWCorner, CellNECorner);
					 cells[i][j].setGlobalCoordinates(i,j);
				 }
			 }
			 if (params->getParameter("runmode") == "masterMNH"){
				 if(getDomainID()==0){
				 list<distributedDomainInfo*>::iterator it;
					 for (it = parallelDispatchDomains.begin(); it != parallelDispatchDomains.end(); it++)
						 {
							 for ( size_t i = (*it)->refNX; i < ((*it)->refNX+(*it)->atmoNX); i++ ) {
								 for ( size_t j = (*it)->refNY; j < ((*it)->refNY+(*it)->atmoNY); j++ ) {
									 cells[i][j].toDumpDomainID =(*it)->ID;
								 }
							 }
					 }
				 }
				 cout <<"Domain "<<getDomainID()<<" size:"<<atmoNX<<":"<<atmoNY<<":"<<atmoNZ<<" area:"<<(NECornerX()-SWCornerX())<<":"<<(NECornerY()-SWCornerY())<<" Bmap size="<<globalBMapSizeX<<":"<<globalBMapSizeY<< " res=" <<burningMatrixResX<<":"<< burningMatrixResY<< endl;
			 }
		 } catch ( const bad_alloc & ) {
			 // deleting what has been allocated
			 for ( size_t i = 0; i < allocated_dim; ++i ) {
				 delete [] cells[i];
			 }
			 delete [] cells;
			 cout << "Not enough free space for allocating the FireDomain cells" << endl;
		 }
		 
		 /* defining the trash cell */
		 trashCell = new FDCell(this);
		 trashCell->makeTrash();
 
 
		 /*----------------------------------------------------*/
		 /* Reading A-time state from a file                   */
		 /*----------------------------------------------------*/
		 /* If a matrix is already stored with given values, taken it */
 
 
		 if ( params->isValued("BMapFiles") ){
			 
			 ostringstream oss;
			 if ( params->getInt("parallelInit") != 0 ){
				 
				 oss<<params->getParameter("caseDirectory")<<'/'
				 <<params->getParameter("ForeFireDataDirectory")<<'/'
				 <<params->getParameter("BMapFiles")
				 <<"."<<getDomainID()<<".nc";
			 } else {
				 oss<<params->getParameter("caseDirectory")<<'/'
				 <<params->getParameter("ForeFireDataDirectory")<<'/'
				 <<params->getParameter("BMapFiles");
			 }
			 this->loadArrivalTimeNC(oss.str());
 
		 }
 
 
		 /*------------------------------------*/
		 /* Constructing the list of frontiers */
		 /*------------------------------------*/
		 FFPoint Corner1, Corner2;
		 Corner1.setX(SWCornerX());
		 Corner1.setY(NECornerY());
		 Corner2.setY(SWCornerY());
		 Corner2.setX(NECornerX());
		 frontiers.push_back(new Frontier(SWCorner, Corner1));
		 frontiers.push_back(new Frontier(Corner1, NECorner));
		 frontiers.push_back(new Frontier(NECorner, Corner2));
		 frontiers.push_back(new Frontier(Corner2, SWCorner));
 
		 infrontiers.push_back(new Frontier(cells[1][1].getSWCorner()
											, cells[1][atmoNY-1].getSWCorner()));
		 infrontiers.push_back(new Frontier(cells[1][atmoNY-1].getSWCorner()
											, cells[atmoNX-1][atmoNY-1].getSWCorner()));
		 infrontiers.push_back(new Frontier(cells[atmoNX-1][atmoNY-1].getSWCorner()
											, cells[atmoNX-1][1].getSWCorner()));
		 infrontiers.push_back(new Frontier(cells[atmoNX-1][1].getSWCorner()
											, cells[1][1].getSWCorner()));
 
 
		 /*-------------------------------------*/
		 /* Building the models and data broker */
		 /*-------------------------------------*/
 
		 /* defining the dataBroker */
		 dataBroker = new DataBroker(this);
 
		 /* setting atmospheric domain variables */
		 dataBroker->setAtmosphericDomain(SWCorner, NECorner, atmoNX, atmoNY);
 
		 /* initializations for the propagation model */
		 propagativeLayer = nullptr;
		 for ( size_t i = 0; i < NUM_MAX_PROPMODELS; i++ ) propModelsTable[i] = 0;
 
		 ostringstream infile;
		 infile << params->GetPath(params->getParameter("NetCDFfile"));
	 
		/* initializations for the flux models */
		 for ( size_t i = 0; i < NUM_MAX_FLUXMODELS; i++ ) fluxModelsTable[i] = NULL;
 
 
		 /* loading the layers for atmospheric variables and coupling variables */
		 if ( atmosphericCoupling ) {
			 dataBroker->initializeAtmosphericLayers(getTime());

		 }

		 dataBroker->loadFromNCFile(infile.str());

		 if ( params->isValued("metersPerDegreeLat") ){
			 metersPerDegreeLat = params->getDouble("metersPerDegreeLat");
			 metersPerDegreesLon = params->getDouble("metersPerDegreesLon");
		 } else {
			double EARTH_RADIUS_ALONG_MERIDIAN = 6342516;
			double EARTH_RADIUS_ALONG_LATITUDE = 6371189;
			
			double refLatitudeRad = refLatitude * (PI / 180.0);
		   
			metersPerDegreeLat = (PI * EARTH_RADIUS_ALONG_LATITUDE) / 180.0;
			metersPerDegreesLon = (PI * EARTH_RADIUS_ALONG_MERIDIAN * std::cos(refLatitudeRad)) / 180.0;
		}
 
		 /* Insuring the presence of needed layers */
		 dataBroker->insureLayersExistence();
		 /* Initializing flux layers */
		 dataBroker->initFluxLayers(getTime());
		 /* Initializing the burning map layer */
		 if ( params->getInt("bmapLayer") != 0 ) {
			 BurningMapLayer<double>* bmlayer =
			 new BurningMapLayer<double>("BMap", this, globalBMapSizeX, globalBMapSizeY);
			 dataBroker->registerLayer("BMap", bmlayer);
		 }
 
		 /* loading the layers for parallel processing */
		 if ( not parallel ){
			 
			 if ( getDomainID() != 0 ) {
				 cout<<" WARNING: not in parallel, but this proc has an mpi rank of "<<getDomainID()<<endl;
			 }
		 }
		
	 }
 
	 int FireDomain::getNumFN(){
		 return domainFront->getTotalNumFN();
	 }
	 size_t FireDomain::getlocalBMapSize(){
		 return localBMapSizeX*localBMapSizeY;
	 }
	 int FireDomain::getNumFF(){
		 return domainFront->getNumInnerFronts();
	 }
 
	 double FireDomain::getSimulationMaxResolution(
		 double& length, double& width, const size_t& nz){
		 return 0.25*sqrt(length*width/nz);
	 }
 
	 double FireDomain::getBurningMapResolution(double& dl, double de){
		 return max(dl/sqrt(2), de);
	 }
 
	 DataBroker* FireDomain::getDataBroker(){
		 return dataBroker;
	 }
 
	 DataLayer<double>* FireDomain::getDataLayer(const string& name){
 
		 return dataBroker->getLayer(name);
	 }
 
	 FluxLayer<double>* FireDomain::getFluxLayer(const string& name){
 
 
		 return dataBroker->getFluxLayer(name);
	 }
	 // Getter for the reference latitude.
	 double FireDomain::getRefLatitude()  {
		 return refLatitude;
	 }
 
	 // Getter for the reference longitude.
	 double FireDomain::getRefLongitude()  {
		 return refLongitude;
	 }
 
	 // Getter for the latitude conversion factor.
	 double FireDomain::getMetersPerDegreeLat()  {
		 return metersPerDegreeLat;
	 }
 
	 // Getter for the longitude conversion factor.
	 double FireDomain::getMetersPerDegreesLon()  {
		 return metersPerDegreesLon;
	 }


	double FireDomain::getXFromLon(double lon) {
		return (lon - getRefLongitude()) * getMetersPerDegreesLon();
	}
	
	double FireDomain::getYFromLat(double lat) {
		return (lat - getRefLatitude()) * getMetersPerDegreeLat();
	}
	
	double FireDomain::getLonFromX(double x) {
		return x / getMetersPerDegreesLon() + getRefLongitude();
	}
	
	double FireDomain::getLatFromY(double y) {
		return y / getMetersPerDegreeLat() + getRefLatitude();
	}

 
	 void FireDomain::getChainAt(
								 const list<FireNodeData*>& hlist, const int& pos
								 , list<FireNodeData*>* chain){
 
		 chain->clear();
		 if ( hlist.empty() ) return;
 
		 int numChains = 0;
		 list<FireNodeData*>::const_iterator startOfChain = hlist.begin();
		 while ( numChains < pos and startOfChain != hlist.end() ){
			 while ( (*startOfChain)->time > endChain + EPSILONT ){
				 ++startOfChain;
			 }
			 ++startOfChain;
			 numChains++;
		 }
		 if ( (*startOfChain)->time < endCom + EPSILONT
			 or startOfChain == hlist.end() ) return;
 
		 while ( (*startOfChain)->time > endChain + EPSILONT ){
			 chain->push_back(*startOfChain);
			 ++startOfChain;
		 }
	 }
 
	 list<FireNodeData*>* FireDomain::popChainAt(
												 list<FireNodeData*>& hlist, const int& pos){
 
		 if ( hlist.empty() ) return 0;
 
		 list<FireNodeData*>* extractedList = new list<FireNodeData*>;
 
		 int numChains = 0;
		 list<FireNodeData*>::iterator startOfChain = hlist.begin();
		 while ( numChains < pos and startOfChain != hlist.end() ){
			 while ( (*startOfChain)->time > endChain + EPSILONT ){
				 ++startOfChain;
			 }
			 ++startOfChain;
			 numChains++;
		 }
		 if ( (*startOfChain)->time < endCom + EPSILONT
			 or startOfChain == hlist.end() ) return 0;
		 while ( (*startOfChain)->time > endChain + EPSILONT ){
			 extractedList->push_back(*startOfChain);
			 startOfChain = hlist.erase(startOfChain);
		 }
		 delete *startOfChain;
		 hlist.erase(startOfChain);
		 return extractedList;
	 }
 
	 
	 void FireDomain::appendChain(list<FireNodeData*>& hlist
								  , const list<FireNodeData*>& chain){
 
		 if ( chain.empty() ) return;
		 list<FireNodeData*>::const_iterator data;
 
		 // append the data in the chain
		 for ( data = chain.begin(); data != chain.end(); ++data ){
 
		 
		  
				 hlist.push_back(*data);
			  
		 }
 
		 // append a marker for end of chain
		 hlist.push_back(new FireNodeData(0, 0, 0, 0, 0, endChain));
		 return;
	 }
 
	 void FireDomain::pushBackInChain(list<FireNodeData*>* chain, FireNodeData* data){
		 FireNodeData* newdata = new FireNodeData(*data);
		 chain->push_back(newdata);
	 }
 
	 void FireDomain::pushFrontInChain(list<FireNodeData*>* chain, FireNodeData* data){
		 FireNodeData* newdata = new FireNodeData(*data);
		 chain->push_front(newdata);
	 }
 
  
 
  
 
 
	 void FireDomain::deleteLinkNodes(list<FireNode*>& lnodes){
		 /* Deleting all the link nodes */
		 while ( !lnodes.empty() ){
			 debugOutput<<getDomainID()<<": FireDomain::deleteLinkNodes -> ";
			 addToTrashNodes(lnodes.back());
			 lnodes.pop_back();
		 }
	 }
  
 
	 int FireDomain::getFrontierIndex(FFPoint loc){
		 if ( abs(loc.getX()-SWCornerX()) < EPSILONX ) return 0;
		 if ( abs(loc.getY()-NECornerY()) < EPSILONX ) return 1;
		 if ( abs(loc.getX()-NECornerX()) < EPSILONX ) return 2;
		 if ( abs(loc.getY()-SWCornerY()) < EPSILONX ) return 3;
		 return -1;
	 }

	 FireNode* FireDomain::findRelatedPreviousLink(
												   list<FireNode*> prevLinks, FFPoint loc){
 
		 debugOutput<<getDomainID()
			 <<": searching for prevLinkNode associated to link node at "<<loc.print()<<endl;
		 FireNode* prevLink = 0;
 
		 /* shortcut if only one link node is to be considered */
		 if ( prevLinks.size() == 1 ){
			 prevLink = prevLinks.front();
			 prevLinks.remove(prevLink);
			 debugOutput<<getDomainID()
				 <<":"<<'\t'<<"found "<<prevLink->toShort()<<endl;
			 return prevLink;
		 }
 
		 /* Finding the closest link node clockwise */
		 list<FireNode*>::iterator node;
		 double minDist = numeric_limits<double>::infinity();
		 double dist;
		 FFPoint nodeLoc;
		 /* first the given frontier */
		 for ( node = prevLinks.begin(); node != prevLinks.end(); ++node ){
			 nodeLoc = (*node)->getLoc();
			 dist = distanceAlongFrontier(loc, nodeLoc);
			 if ( dist < minDist ){
				 prevLink = *node;
				 minDist = dist;
			 }
		 }
		 if ( prevLink != 0 ){
			 prevLinks.remove(prevLink);
			 debugOutput<<getDomainID()
						 <<":"<<'\t'<<"found "<<prevLink->toShort()<<endl;
		 } else {
			 debugOutput<<getDomainID()
						 <<":"<<'\t'<<"didn't find a related prevLinkNode"<<endl;
			 throw ParallelException(debugOutput.str(), "FireDomain::findRelatedPreviousLink");
		 }
		 return prevLink;
	 }
 
	 void FireDomain::relateLinkNodes(FireNode* linkA, FireNode* linkB){
		 FireNode* curLink = linkA;
		 FireNode* interLink = 0;
		 FFPoint linkPos;
 
		 int frt = getFrontierIndex(linkA->getLoc());
		 int endfrt = getFrontierIndex(linkB->getLoc());
 
		 if ( frt == endfrt ){
			 if ( distanceOnFrontier(frt, linkA->getLoc(), linkB->getLoc()) < 0. ){
				 frt = (frt+1)%4;
				 linkPos = getStartCornerFromIndex(frt);
				 interLink = addLinkNode(linkPos);
				 interLink->setPrev(curLink);
				 interLink->setFront(curLink->getFront());
				 curLink->setNext(interLink);
				 curLink = interLink;
			 }
		 }
 
		 while ( frt != endfrt ){
			 frt = (frt+1)%4;
			 linkPos = getStartCornerFromIndex(frt);
			 interLink = addLinkNode(linkPos);
			 interLink->setPrev(curLink);
			 interLink->setFront(curLink->getFront());
			 curLink->setNext(interLink);
			 curLink = interLink;
		 }
 
		 linkB->setPrev(curLink);
		 curLink->setNext(linkB);
 
	 }
 
	 void FireDomain::validateTopology(string call){
		 debugOutput<<getDomainID()<<": Validating the topology"
			 <<" after "<<call<<endl
			 <<"***************************************************"<<endl;
		 domainFront->getTotalNumFN();
		 for ( size_t i = 0; i < atmoNX; i++ ) {
			 for ( size_t j = 0; j < atmoNY; j++ ) {
				 // validating the topology inside each cell
				 cells[i][j].validateTopology(call);
			 }
		 }
  
	 }
 
	 double FireDomain::distanceOnFrontier(int& frt, FFPoint locA, FFPoint locB){
		 int frontier = frt%4;
		 if ( frontier == 0 ) return locB.getY() - locA.getY();
		 if ( frontier == 1 ) return locB.getX() - locA.getX();
		 if ( frontier == 2 ) return locA.getY() - locB.getY();
		 if ( frontier == 3 ) return locA.getX() - locB.getX();
		 return 0;
	 }
 
	 double FireDomain::distanceOnBoundingBox(int& frt, FFPoint locA, FFPoint locB){
		 int frontier = frt%4;
		 if ( frontier == 0 ) return locB.getY() - locA.getY();
		 if ( frontier == 1 ) return locB.getX() - locA.getX();
		 if ( frontier == 2 ) return locA.getY() - locB.getY();
		 if ( frontier == 3 ) return locA.getX() - locB.getX();
		 return 0;
	 }
 
	 double FireDomain::distanceAlongFrontier(FFPoint& locA, FFPoint& locB){
		 /* Distance between 2 points of the frontier, clockwise */
 
		 double distance = 0.;
		 int frontierA = getFrontierIndex(locA);
		 int frontierB = getFrontierIndex(locB);
 
		 if ( frontierA == frontierB ){
			 distance = distanceOnFrontier(frontierA, locA, locB);
			 if ( distance < 0 ){
				 /* this means it's counter-clockwise, computing the distance clockwise */
				 distance = 2.*(NECornerX()-SWCornerX()+NECornerY()-SWCornerY()) + distance;
			 }
		 } else {
			 /* adding the distances between the first location and corner,
			  * between successive corners and finally the last corner to end point */
			 int numFrt = 0;
			 int frt = frontierA;
			 int nextfrt;
			 FFPoint tmpLoc = locA;
			 while ( frt != frontierB and numFrt < 4 ){
				 nextfrt = (frt+1)%4;
				 distance += distanceOnFrontier(frt, tmpLoc, getStartCornerFromIndex(nextfrt));
				 tmpLoc = getStartCornerFromIndex(nextfrt);
				 frt = nextfrt;
				 numFrt++;
			 }
			 distance += distanceOnFrontier(frontierB, getStartCornerFromIndex(frontierB), locB);
		 }
 
		 return distance;
	 }
 
 
 
	 FFPoint& FireDomain::getStartCornerFromIndex(const int& num){
		 int frontier = num%4;
		 if ( frontier == 0 ) return SWCorner;
		 if ( frontier == 1 ) return NWCorner;
		 if ( frontier == 2 ) return NECorner;
		 return SECorner;
	 }
 
	 FFPoint FireDomain::getBoundingBoxCornerFromIndex(const int& num
													   , FFPoint& swc, FFPoint& nec){
		 int frontier = num%4;
		 if ( frontier == 0 ) return swc;
		 if ( frontier == 1 ) return FFPoint(swc.getX(), nec.getY(),0.);
		 if ( frontier == 2 ) return nec;
		 return FFPoint(nec.getX(), swc.getY(),0.);
	 }
 
  
 
	 FireNode* FireDomain::findExistingNodeNear(FireNodeData* fn){
		 /* Searching for an existing firenode near an incoming parallel data */
 
		 FireNode* closeNode = 0;
 
		 // Searching distance is one perimeter resolution
		 double dist = getPerimeterResolution();
 
		 // Determining which cells are to be scanned
		 FDCell* curCell = getCell(fn);
		 FFPoint center = 0.5*(curCell->getSWCorner() + curCell->getNECorner());
		 FFPoint dx = FFPoint(curCell->getNECorner().getX() - curCell->getSWCorner().getX(),0.,0.);
		 FFPoint dy = FFPoint(0.,curCell->getNECorner().getY() - curCell->getSWCorner().getY(),0.);
		 FDCell* searchedCell;
		 int numCellsX, numCellsY;
		 numCellsX = ((int) dist/(curCell->getNECorner().getX()-curCell->getSWCorner().getX())) + 1;
		 numCellsY = ((int) dist/(curCell->getNECorner().getY()-curCell->getSWCorner().getY())) + 1;
 
		 // Scanning the cells for the closest firenode (if it exists)
		 list<FireNode*>::iterator ofn;
		 FFPoint rep;
		 for ( int i=-numCellsX; i<numCellsX+1; i++ ){
			 for ( int j=-numCellsY; j<numCellsY+1; j++ ){
				 rep = center + i*dx + j*dy;
				 searchedCell = getCell(rep);
				 if ( searchedCell == trashCell ) continue;
				 for ( ofn = searchedCell->fireNodes.begin();
					  ofn != searchedCell->fireNodes.end(); ++ofn ) {
					 if ( (*ofn)->getState()!=FireNode::final
						 and (*ofn)->getState()!=FireNode::link )  {
						 if ( fn->distance(*ofn) < dist ) {
							 dist = fn->distance(*ofn);
							 closeNode = *ofn;
						 }
					 }
				 }
			 }
		 }
		 return closeNode;
	 }
 
	 FireNode* FireDomain::findClosestNodeInList(FFPoint& loc
												 , const list<FireNode*>& nodeList){
		 /* Searching to match an incoming firenode with an excess one */
		 if ( nodeList.size() == 0 ) return 0;
		 if ( nodeList.size() == 1 ) return nodeList.front();
		 list<FireNode*>::const_iterator ifn;
		 FireNode* closeNode = 0;
		 double distance = numeric_limits<double>::infinity();
		 double dist;
		 for ( ifn = nodeList.begin(); ifn != nodeList.end(); ++ifn ){
			 if ( (*ifn)->getState() != FireNode::link ){
				 dist = (*ifn)->distance2D(loc);
				 if ( dist < distance ){
					 distance = dist;
					 closeNode = *ifn;
				 }
			 }
		 }
		 return closeNode;
	 }
 
	 FireNode* FireDomain::findClosestNodeInList(FireNodeData* fnd
												 , const list<FireNode*>& nodeList){
		 FFPoint loc(fnd->posX, fnd->posY,0.);
		 return findClosestNodeInList(loc, nodeList);
	 }
 
	 FireNode* FireDomain::findClosestWithinNodeInList(FFPoint& loc
													   , const double& searchRange, const list<FireNode*>& nodeList){
		 /* Searching to match an incoming firenode with an excess one */
		 if ( nodeList.size() == 0 ) return 0;
		 list<FireNode*>::const_iterator ifn;
		 FireNode* closeNode = 0;
		 double distance = numeric_limits<double>::infinity();
		 double dist;
		 for ( ifn = nodeList.begin(); ifn != nodeList.end(); ++ifn ){
			 dist = (*ifn)->distance2D(loc);
			 if ( dist < distance and dist < searchRange ){
				 distance = dist;
				 closeNode = *ifn;
			 }
		 }
		 return closeNode;
	 }
 
	 FireNode* FireDomain::findClosestWithinNodeInList(FireNodeData* fnd
													   , const double& searchRange, const list<FireNode*>& nodeList){
		 FFPoint loc(fnd->posX, fnd->posY,0.);
		 return findClosestWithinNodeInList(loc, searchRange, nodeList);
	 }
 
 
	 // Visitor function
	 void FireDomain::accept(Visitor* v) {
 
		  
		 v->visit(this);
		 v->postVisitInner(this);
		 domainFront->accept(v);
		 v->postVisitAll(this);
	 }
 
	 string FireDomain::toString(){
		 ostringstream oss;
		 oss<<"FireDomain[sw="<<SWCorner.print()<<";ne="<<NECorner.print()
		 << ";t=" << getSimulationTime()<< "]";
		 return oss.str();
	 }
 
	 string FireDomain::getFluxModelName(int fluxModelIndice){
		 if (fluxModelIndice < (int)FireDomain::NUM_MAX_FLUXMODELS) {
			 if(fluxModelsTable[fluxModelIndice] != NULL)
				 return fluxModelsTable[fluxModelIndice]->getName();
		 }
		 return "";
	 }
 
	 string FireDomain::printMainFronts(){
		 return domainFront->print();
	 }
 
	 // Function: getDataMatrix
// Description: Returns a 2D matrix of doubles corresponding to the provided data name.
    std::vector<std::vector<double>> FireDomain::getDataMatrix(const std::string& name) {
		size_t eni = 0;
		size_t enj = 0;
		return getDataMatrix(name, SWCorner ,NECorner,eni,enj);
	}
	std::vector<std::vector<double>> FireDomain::getDataMatrix(const std::string& name, FFPoint& SWbound, FFPoint& NEbound, size_t& eni, size_t& enj) {
		double extractWidth = NEbound.getX() - SWbound.getX();
		double extractHeight = NEbound.getY() - SWbound.getY();
		double domainWidth = NECornerX() - SWCornerX();
		double domainHeight = NECornerY() - SWCornerY();

		if((extractWidth <= 0) || (extractHeight <= 0)) {
			std::cerr << "Error: Invalid bounding box " << std::endl;
			return std::vector<std::vector<double>>(1, std::vector<double>(1, -9999));
		}


		double resX = 0.0;
		double resY = 0.0;
		double lTime = getSimulationTime();
		// Case 1: "speed" data
		if ((name == "speed") || (name == "arrival_time")) {

			// Create a matrix with dimensions (globalBMapSizeX x globalBMapSizeY), initialized to -9999
			if ((eni == 0)||(enj == 0)) {
				resX = domainWidth/globalBMapSizeX;
				resY = domainHeight/globalBMapSizeY;		
				eni = extractWidth/resX;
				enj = extractHeight/resY;
			}else{
				resX = extractWidth/eni;
				resY = extractHeight/enj;
			}

			std::vector<std::vector<double>> matrix(eni, std::vector<double>(enj, -9999));

			FFPoint itp = FFPoint(SWbound.getX()+resX/2, SWbound.getY()+resY/2, 0.0);
			if (name == "speed") {
				for (size_t i = 0; i < eni; i++) {
					itp.setX(SWbound.getX()+resX/2 + i*resX);
					for (size_t j = 0; j < enj; j++) {
						itp.setY(SWbound.getY()+resY/2 + j*resY);
						matrix[i][j] = getMaxSpeed(itp);  
					}
				}
			}
			if (name == "arrival_time") {
				for (size_t i = 0; i < eni; i++) {
					itp.setX(SWbound.getX()+resX/2 + i*resX);
					for (size_t j = 0; j < enj; j++) {
						itp.setY(SWbound.getY()+resY/2 + j*resY);
						matrix[i][j] = getArrivalTime(itp);  
					}
				}
			}
			return matrix;
		}
		// Case 3: Data available from a FluxLayer
		DataLayer<double>* dataLayer = getDataLayer(name);
		if (dataLayer == 0) {
			cout << "DataLayer is null from data" << endl;
			dataLayer = getFluxLayer(name);
		
		}
	 	if (dataLayer != 0) {
			
			if ((eni == 0)||(enj == 0)) {
				resX = dataLayer->getDx();
				resY = dataLayer->getDy();	
				eni = extractWidth/resX;
				enj = extractHeight/resY;
			}else{
				resX = extractWidth/eni;
				resY = extractHeight/enj;
			}
			FFPoint itp = FFPoint(SWbound.getX()+resX/2, SWbound.getY()+resY/2, 0.0);
			std::vector<std::vector<double>> matrix(eni, std::vector<double>(enj, -9999));
			for (size_t i = 0; i < eni; i++) {
				itp.setX(SWbound.getX()+resX/2 + i*resX);
				for (size_t j = 0; j < enj; j++) {
					itp.setY(SWbound.getY()+resY/2 + j*resY);
					matrix[i][j] = dataLayer->getValueAt(itp,lTime);  
				}
			}
			return matrix;
		}

		// Error case: Unrecognized data name
		std::cerr << "Error: Data name '" << name << "' not recognized." << std::endl;
		// Return a default 1x1 matrix with a placeholder value
		return std::vector<std::vector<double>>(1, std::vector<double>(1, -9999));
	}
 
 
	 void FireDomain::dumpBurningMatrixAsBinary(){
		 // Binary file
		 ostringstream oss;
		 
 
		 oss<<params->getParameter("ffOutputsPattern")<<".bmap";
		 fstream binary_file(oss.str().c_str(),ios::out|ios::binary);
		 binary_file.write(reinterpret_cast<char *>(&globalBMapSizeX),sizeof(size_t));
		 binary_file.write(reinterpret_cast<char *>(&globalBMapSizeY),sizeof(size_t));
		 double value;
		 for ( size_t i = 0; i < globalBMapSizeX; i++ ){
			 for ( size_t j = 0; j < globalBMapSizeY; j++ ){
				 value = getArrivalTime(i,j);
				 binary_file.write(reinterpret_cast<char *>(&value),sizeof(double));
			 }
		 }
		 binary_file.close();
	 }
 
 
 
	 void FireDomain::loadArrivalTimeNC(string fname){
			 if (getDomainID()!=0) return;
 
				 try
						 {
								 NcFile dataFile(fname.c_str(), NcFile::read);
								 
								 NcVar atime = dataFile.getVar("arrival_time_of_front");
								 
								 size_t   FSPACE_DIM1 		= atime.getDim(1).getSize();	// Nb De lignes au total
								 size_t   FSPACE_DIM2 		= atime.getDim(0).getSize();	// 	Nb De colonnes au total
 
								 if((globalBMapSizeX!=FSPACE_DIM1)||(globalBMapSizeY!=FSPACE_DIM2)){
													 cout<<getDomainID()<<":ERROR: number of cells of the burning matrices ("
													 <<globalBMapSizeX<<"x"<<globalBMapSizeY<<")"
													 <<" not compatible with read data "
													 <<FSPACE_DIM1<<"x"<<FSPACE_DIM2<<endl;
													 dataFile.close();
													 return;
											  
								 }
 
								 NcVar dom = dataFile.getVar("domain"); 
				 
								 double Xorigin = 0;
								 dom.getAtt("SWx").getValues(&Xorigin);
								 double Yorigin = 0;
								 dom.getAtt("SWy").getValues(&Yorigin);
								 double Xlen = 0;
								 dom.getAtt("Lx").getValues(&Xlen);
								 Xlen = Xlen;
								 double Ylen = 0;
								 dom.getAtt("Ly").getValues(&Ylen);
								 Ylen=Ylen;
								 int pyear = 0;
								 dom.getAtt("refYear").getValues(&pyear);
								 int pday = 0;
								 dom.getAtt("refDay").getValues(&pday);
								 refYear = pyear;
								 params->setInt("refYear", pyear);
								 refDay = pday;
								 params->setInt("refDay", pday);
 
								 double max_time = params->getDouble("InitTime");
		 // Dynamically allocate the matrix on the heap
  
								 double* allDataAtime = new double[globalBMapSizeY * globalBMapSizeX]; 
								 
								 atime.getVar(allDataAtime);
 
								 double tmpval = 0;    
								 for (size_t i = 0; i < globalBMapSizeX; i++) {
									 for (size_t j = 0; j < globalBMapSizeY; j++) {
										 tmpval = allDataAtime[j * globalBMapSizeX + i]; 
										 if ((tmpval < max_time) && (tmpval > -9999)) {
											 this->setArrivalTime(i , j,tmpval );
										 }
									 }
								 }
								 dataFile.close();
								 cout<<getDomainID()<<": Read "<< fname<<"("<<globalBMapSizeX<<"x"<<globalBMapSizeY<<")" <<FSPACE_DIM1<<"x"<<FSPACE_DIM2<<endl;
								 delete[] allDataAtime;
 
 
					 }
					 catch (std::exception const & e)
					 {
						 cout << "Exception: " << e.what() << endl;
					 }
					 catch (...)
					 {
						 cout << "Error: unknown error." << endl;
					 } 
						 
			 
			  
		 }
 
   void FireDomain::saveArrivalTimeNC(){
	 
	try {
		 // Setup file paths and identifiers
		 ostringstream oss;
		 oss << params->getParameter("caseDirectory") << '/'
			 << params->getParameter("fireOutputDirectory") << '/'
			 << params->getParameter("experiment") << "." << getDomainID() << ".nc";
 
		 // Create and configure the NetCDF file
		 NcFile dataFile(oss.str(), NcFile::replace);
		 NcDim xDim = dataFile.addDim("DIMX", globalBMapSizeX); // Width
		 NcDim yDim = dataFile.addDim("DIMY", globalBMapSizeY); // Height
		 vector<NcDim> dims = {yDim, xDim}; // Order is important for visualization
		 NcVar atime = dataFile.addVar("arrival_time_of_front", ncDouble, dims);
		 atime.setCompression(true, true, 6);
  
		 double* matrix = new double[globalBMapSizeY * globalBMapSizeX];
 
		 for (size_t i = 0; i < globalBMapSizeX; i++) {
			 for (size_t j = 0; j < globalBMapSizeY; j++) {
				 double tmpval = this->getArrivalTime(i , j );
				 if (std::isinf(tmpval)) { 
					 matrix[j * globalBMapSizeX + i] =  -9999 ;
				 }else{
					 matrix[j * globalBMapSizeX + i] =  tmpval; 
				 }
			 }
		 }
		 
		 // Write the arrival time data
		 atime.putVar(&matrix[0]);
		 // Add domain and reference attributes
		 NcDim domdim = dataFile.addDim("domdim", 1);
		 NcVar dom = dataFile.addVar("domain", ncChar, domdim);
		 dom.putAtt("SWx", NC_DOUBLE, SWCorner.getX());
		 dom.putAtt("SWy", NC_DOUBLE, SWCorner.getY());
		 dom.putAtt("Lx", NC_DOUBLE, NECorner.getX() - SWCorner.getX());
		 dom.putAtt("Ly", NC_DOUBLE, NWCorner.getY() - SWCorner.getY());
		 dom.putAtt("Lz", NC_DOUBLE, 0.);
		 dom.putAtt("refYear", NC_INT, int(refYear));
		 dom.putAtt("refDay", NC_INT, int(refDay));
		 // Close the file
		 dataFile.close();
 
 
		 delete[] matrix;
 
	 } 
	 catch (std::exception const & e)
	 {
		 cout << "Exception in creating a_time: " << e.what() << endl;
	 }
	 catch (...)
	 {
		 cout << "Error: unknown error." << endl;
	 } 
	 }
 
 
	 void FireDomain::visualizeBurningMatrixAroundNode(FireNode* fn){
		 if( !striclyWithinDomain(fn) ) return;
		 /* Position of the firenode in the burning matrix */
		 int ii = (int)((fn->getX() - SWCornerX())) / burningMatrixResX;
		 int jj = (int)((fn->getY() - SWCornerY())) / burningMatrixResY;
		 /* which quarter plane to print */
		 int xm, xp, ym,yp;
		 if ( fn->getVx()*fn->getVy() > 0. ) {
			 if ( fn->getVx() > 0. ){
				 xm = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 xp = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 ym = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 yp = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
			 } else {
				 xm = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 xp = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 ym = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 yp = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
			 }
		 } else {
			 if ( fn->getVx() > 0. ){
				 xm = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 xp = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 ym = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 yp = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
			 } else {
				 xm = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
				 xp = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 ym = (int) 1.2*fn->getFrontDepth()/burningMatrixRes;
				 yp = (int) 0.2*fn->getFrontDepth()/burningMatrixRes;
			 }
		 }
		 /* bounding box */
		 int imin, imax, jmin, jmax;
		 ii > xm ? imin = ii-xm : imin = 0;
		 ii+xp < (int)globalBMapSizeX ? imax = ii+xp : imax =  (int)globalBMapSizeX;
		 jj > ym ? jmin = jj-ym : jmin = 0;
		 jj+yp <  (int)globalBMapSizeY ? jmax = jj+yp : jmax =  (int)globalBMapSizeY;
		 /* printing the matrix around the node */
		 ostringstream oss;
		 int width = 5;
		 for ( int i = imin; i < imax; i++ ){
			 for ( int j = jmin; j < jmax; j++ ){
				 if ( i == ii and j == jj ){
					 oss<<setw(width)<<" xxx "<<" ";
				 } else {
					 oss<<setw(width)<<getArrivalTime(i, j)<<" ";
				 }
			 }
			 oss<<endl;
		 }
		 cout<<oss.str();
	 }
 
 }
 