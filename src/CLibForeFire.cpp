/**
 * @file CLibForeFire.cpp
 * @brief Bindings to C and Fortran as well as MPI processing if activated (requires a coupler model)
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "CLibForeFire.h"
#include "SimulationParameters.h"
#include <cmath>
#include <random>

#ifdef MPI_COUPLING
#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <limits>
#include <sstream>

#endif


using namespace std;

#ifdef MPI_COUPLING

#endif



namespace libforefire {

Command executor;
static Command::Session* session = &(executor.currentSession);
int world_rank;
int world_size;
size_t mnhPause;
double updateBinStreamFrequency = 10;
double updateOutputFrequency = 0;


Command* getLauncher(){
	return &executor;
}

void MNHInit(const double t){
	executor.setReferenceTime(t);


	/* Reading all the information on the parameters of ForeFire */
	ostringstream paramsfile;
	paramsfile<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
			<<SimulationParameters::GetInstance()->getParameter("ForeFireDataDirectory")<<'/'
			<<SimulationParameters::GetInstance()->getParameter("paramsFile");

	mnhPause = SimulationParameters::GetInstance()->getInt("MNHalt");
	ifstream inputParams(paramsfile.str().c_str());
	if ( inputParams ) {
		string line;
		while ( getline( inputParams, line ) ) {
			// checking for comments or newline
			if((line[0] == '#')||(line[0] == '*')||(line[0] == '\n'))
				continue;
			// treating the command of the current line
			executor.ExecuteCommand(line);
		}
	} else {
		cout<<"ERROR: File for ForeFire initialization not found !!"<<endl;
		cout<<'\t'<<"looked in "<<paramsfile.str()<<endl;
	}

	executor.outputDirs = SimulationParameters::GetInstance()->getParameterArray("atmoOutputDirectories");

}

void MNHCreateDomain(const int id
		, const int year, const int month
		, const int day, const double t
		, const double lat, const double lon
		, const int mdimx, const double* meshx
		, const int mdimy, const double* meshy
		, const int mdimz, const double* zgrid
		, const double dt){

	/* Defining the Fire Domain */
	if (session->fd) delete session->fd;

	SimulationParameters::GetInstance()->setDouble("atmosphericTimeStep", dt);
	SimulationParameters::GetInstance()->setDouble("atmosphericCellLength", (meshx[1] - meshx[0]));
	SimulationParameters::GetInstance()->setDouble("atmosphericCellWidth", (meshy[1] - meshy[0]));


	SimulationParameters::GetInstance()->setDouble("sumInjected", 0.0);
	SimulationParameters::GetInstance()->setDouble("sumSedimented", 0.0);

	session->fd = new FireDomain(id, year, month, day, t, lat, lon
			, mdimx, meshx, mdimy, meshy, mdimz, dt);

    #ifdef MPI_COUPLING

        int world_rank;
        int world_size;

        // Obtenir le rang et la taille
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);

        // Vérifier qu'il y a au moins 2 processus
        if (world_size < 2) {
            if (world_rank == 0) {
                std::cerr << "Ce programme nécessite au moins 2 processus.\n";
            }
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        // Collecte des données à envoyer
        int data_int[3];
        data_int[0] = id;
        data_int[1] = mdimx;
        data_int[2] = mdimy;

        double data_double[4];
        data_double[0] = meshx[0];
        data_double[1] = meshy[0];
        data_double[2] = meshx[mdimx-1] + (meshx[1] - meshx[0]);
        data_double[3] = meshy[mdimy-1] + (meshy[1] - meshy[0]);

        if (world_rank != 0) {
            // Envoyer les données à rank 0
            MPI_Send(data_int, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(data_double, 4, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }

        if (world_rank == 0) {
            // Afficher les données de rank 0

			session->fd->pushMultiDomainMetadataInList(id,0,mdimx,mdimy,meshx[0],meshy[0],meshx[mdimx-1] + (meshx[1] - meshx[0]),meshy[mdimy-1] + (meshy[1] - meshy[0]));
            // Recevoir et afficher les données des autres rangs
            for (int src = 1; src < world_size; ++src) {
                int recv_int[3];
                double recv_double[4];
                MPI_Recv(recv_int, 3, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(recv_double, 4, MPI_DOUBLE, src, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				session->fd->pushMultiDomainMetadataInList(recv_int[0],0,recv_int[1] ,recv_int[2],recv_double[0],recv_double[1],recv_double[2],recv_double[3]);
           
            }

        }

    #endif

	//executor.setDomain(session->fd);

	// A FireDomain has been created, the level is increased
	executor.increaseLevel();
	session->ff = session->fd->getDomainFront();
	// Defining the timetable of the events to be be in the domain
	if (session->tt) delete session->tt;
	session->tt = new TimeTable();
	// Associating this timetable to the domain
	session->fd->setTimeTable(session->tt);
	// Defining the simulator
	if (session->sim) delete session->sim;
	session->sim = new Simulator(session->tt, session->fd->outputs);

	/* Managing the outputs */
	ostringstream ffOutputsPattern;
	ffOutputsPattern<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
			<<SimulationParameters::GetInstance()->getParameter("fireOutputDirectory")<<'/'
			<<SimulationParameters::GetInstance()->getParameter("outputFiles")
			<<"."<<session->fd->getDomainID();
	SimulationParameters::GetInstance()->setParameter("ffOutputsPattern", ffOutputsPattern.str());



	session->outStrRep = new StringRepresentation(executor.getDomain());
	if ( SimulationParameters::GetInstance()->getInt("outputsUpdate") != 0 ){

		session->tt->insert(new FFEvent(session->outStrRep));

		updateOutputFrequency = SimulationParameters::GetInstance()->getDouble("outputsUpdate");
		// handle bynary updates

	}
  
	// Reading all the information on the initialization of ForeFire
	ostringstream initfile;
	if ( SimulationParameters::GetInstance()->getInt("parallelInit") != 1 ) {
		// It is parallel, but Mono-file case: one file for main processor rank 0
		initfile<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("ForeFireDataDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("InitFile");
	} else {
		// It is parallel, but multidomain file case: one file for each processor
		initfile<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("ForeFireDataDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("InitFiles")
								<<"."<<id<<"."<<SimulationParameters::GetInstance()->getParameter("InitTime");

	}


	ifstream inputInit(initfile.str().c_str());
	#ifdef MPI_COUPLING
	if (world_rank == 0) {
	#else
	if (1) {
	#endif
		if ( inputInit ) {
			
			string line;
			//size_t numLine = 0;
			// skip the firest "firedomain" line for // init with multiple files
			if ( SimulationParameters::GetInstance()->getInt("parallelInit") == 1 ) getline( inputInit, line );
			while ( getline( inputInit, line ) ) {
				//numLine++;
				// checking for comments or newline
				if((line[0] == '#')||(line[0] == '*')||(line[0] == '\n'))
					continue;
				// treating the command of the current line
				executor.ExecuteCommand(line);
			}
		} else {
			cout<<"File for ForeFire initialization "<<initfile.str()
					<<" not found !! No markers will be advected" << endl;
		}

		// completing the last front
		executor.completeFront(executor.currentSession.ff);
	}
	#ifdef MPI_COUPLING
	ostringstream globalInitFile;
	globalInitFile<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("ForeFireDataDirectory")<<'/'
								<<SimulationParameters::GetInstance()->getParameter("GlobalInitFile");

	ifstream globalInputInit(globalInitFile.str().c_str());
			if ( globalInputInit ) {
			
			string line;
			 
			while ( getline( globalInputInit, line ) ) {
				//numLine++;
				// checking for comments or newline
				if((line[0] == '#')||(line[0] == '*')||(line[0] == '\n'))
					continue;
				// treating the command of the current line
				executor.ExecuteCommand(line);
			}
		} else {
			cout<<"No global initialization "<<globalInitFile.str()<<" files for all domains" << endl;
		}
	#endif
 
	// advancing the simulation to the beginning of the atmospheric simulation
	double deltaT = session->fd->getSecondsFromReferenceTime(year, month, day, t);

	executor.setReferenceTime(deltaT);
	executor.setStartTime(deltaT);



}

void CheckLayer(const char* lname){
	string tmpname(lname);
	// searching for concerned layer
	
	FluxLayer<double>* myLayer = session->fd->getFluxLayer(tmpname);

	if ( myLayer == 0 ){
		//cout<<"WARNING: "<<tmpname <<" is required but not defined, creating default"<<endl;
		if ( !session->fd->addFluxLayer(tmpname) ){
			cout<<"WARNING: layer for "<<tmpname
					<<" could not be found within ForeFire framework, "
					<<"this should cause serious problems when coupling"<<endl;
		}
	}
}

void MNHStep(double dt){

	#ifdef MPI_COUPLING
		size_t sizeofcell = session->fd->getlocalBMapSize();
		updateBinStreamFrequency = SimulationParameters::GetInstance()->getDouble("updateBinStreamFrequency");
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);

		if (world_rank == 0) {

			
			FDCell** mycells = session->fdp->getCells();
			size_t domainID =  1;
			FireDomain::distributedDomainInfo* domainInfo = session->fdp->getParallelDomainInfo(domainID);
			size_t anx = domainInfo->atmoNX;
			size_t any = domainInfo->atmoNY;
			size_t rnx = domainInfo->refNX;
			size_t rny = domainInfo->refNY;
			// counting all active cells to be sent
			for(size_t i = rnx; i < rnx + anx; ++i){
				for(size_t j = rny; j < rny + any; ++j){
					if(mycells[i][j].isActiveForDump()){
						FFArray<double>* SRCburningMap = mycells[i][j].getBurningMap()->getMap();
						session->fd->getCells()[i - rnx][j - rny].setBMapValues(SRCburningMap->getData());
					//	cout << "sending Active cell found at (" << i << ", " << j << ")" << std::endl;
						//mycells[i][j].setIfAllDumped();
					}
				}
			}
		
			for (int nr = 1; nr < world_size; ++nr) {
				// Send the number of active cells to each rank depending on which subdomain they are, first the number of active cells per domain
				domainID = nr + 1;
				int32_t numberOfActiveCells = 0;//session->fdp->countActiveCellsInDispatchDomain(domainID);
				FireDomain::distributedDomainInfo* domainInfo = session->fdp->getParallelDomainInfo(domainID);
				size_t anx = domainInfo->atmoNX;
				size_t any = domainInfo->atmoNY;
				size_t rnx = domainInfo->refNX;
				size_t rny = domainInfo->refNY;
				// Iterate over the cells within the specified domain
				for (size_t i = rnx; i < rnx + anx; ++i) {
					for (size_t j = rny; j < rny + any; ++j) {
						if (mycells[i][j].isActiveForDump()) {
							numberOfActiveCells++;
						}
					}
				}
			
				MPI_Send(&numberOfActiveCells, 1, MPI_INT32_T, nr, 0, MPI_COMM_WORLD);
				// Send the data of the active cells
				if (numberOfActiveCells>0){
					size_t totalBytes = numberOfActiveCells * (2 * sizeof(int32_t) + sizeofcell * sizeof(double));
					std::vector<char> BMAP_DATA_to_send(totalBytes);
					size_t offset = 0;
					// Iterate through the cells and serialize active cell data
						for(size_t i = rnx; i < rnx + anx; ++i){
							for(size_t j = rny; j < rny + any; ++j){
								if(mycells[i][j].isActiveForDump()){
									// Extract local indices
									int32_t localx = static_cast<int32_t>(i - rnx);
									int32_t localy = static_cast<int32_t>(j - rny);
									// Extract cell data
									FFArray<double>* burningMap =  session->fdp->getCells()[i][j].getBurningMap()->getMap();
									// Serialize localx
									memcpy(BMAP_DATA_to_send.data() + offset, &localx, sizeof(int32_t));
									offset += sizeof(int32_t);
									memcpy(BMAP_DATA_to_send.data() + offset, &localy, sizeof(int32_t));
									offset += sizeof(int32_t);
									memcpy(BMAP_DATA_to_send.data() + offset, burningMap->getData(), sizeofcell * sizeof(double));
									offset += sizeofcell * sizeof(double);
									// marked it dumped
									//mycells[i][j].setIfAllDumped();
								}
							}
						}
					MPI_Send(BMAP_DATA_to_send.data(), totalBytes, MPI_CHAR, nr, 1, MPI_COMM_WORLD);
				}	
			}
				// mark as dumped the cells in the subdomain for rank 1
			domainInfo = session->fdp->getParallelDomainInfo(1);
				anx = domainInfo->atmoNX;
				any = domainInfo->atmoNY;
				rnx = domainInfo->refNX;
				rny = domainInfo->refNY;
			// Iterate over the cells within the specified domain
			for (size_t i = rnx; i < rnx + anx; ++i) {
				for (size_t j = rny; j < rny + any; ++j) {
					if (mycells[i][j].isActiveForDump()) {
						//mycells[i][j].setIfAllDumped();
					}
				}
			}
			bool timeForDump = (std::fmod(session->fdp->getTime(), updateOutputFrequency) < 1e-6);
			vector<string> optLayers =	SimulationParameters::GetInstance()->getParameterArray("accumulatedDiagnosticScalarLayersNames");
			for (size_t i = 0; i < optLayers.size(); i++)
			{
				DataLayer<double>* myMasterLayer = session->fdp->getDataLayer(optLayers[i]+"Accumulated");
				FFArray<double>* fullMatrix;
				myMasterLayer->getMatrix(&fullMatrix,0);

				DataLayer<double>* myInstantLayer = session->fdp->getDataLayer(optLayers[i]);
				FFArray<double>* instantMatrix;
				myInstantLayer->getMatrix(&instantMatrix,0);


				size_t nx = fullMatrix->getDim("x");
				size_t ny = fullMatrix->getDim("y");
				double* matrixData = fullMatrix->getData();
				double* instantData = instantMatrix->getData();

				for (size_t i = 0; i < nx; ++i) {
					for (size_t j = 0; j < ny; ++j) {
						size_t index = i * ny + j;
						matrixData[index] += instantData[index]*dt;
					}
				}
				if(timeForDump){
					cout<< session->fdp->getTime()<< " Layer " << optLayers[i] << " total: " << fullMatrix->sum() << endl;
					std::ostringstream fnoss;
					fnoss  << optLayers[i] << "Accumulated_fp64_"<<nx<<"_"<<ny<<".dat";
					std::string filename = fnoss.str();
					std::ofstream ofs(filename, std::ios::binary | std::ios::app);
					if (ofs) {
						ofs.write(reinterpret_cast<const char*>(matrixData), nx * ny * sizeof(double));
						ofs.close();
					}
					fullMatrix->fill(0.0);
				}
			}
			

		}else {
			int32_t numberOfActiveCellsInDomain = 0;
			MPI_Recv(&numberOfActiveCellsInDomain, 1, MPI_INT32_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
			if (numberOfActiveCellsInDomain>0){	
				size_t totalBytes = numberOfActiveCellsInDomain * (2 * sizeof(int32_t) + sizeofcell * sizeof(double));
				std::vector<char> BMAP_DATA_received(totalBytes);
				MPI_Recv(BMAP_DATA_received.data(), totalBytes, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				size_t offset = 0;
				
				for(int32_t c = 0; c < numberOfActiveCellsInDomain; ++c) {
					int32_t localx;
					int32_t localy;
					memcpy(&localx, BMAP_DATA_received.data() + offset, sizeof(int32_t));
					offset += sizeof(int32_t);
					memcpy(&localy, BMAP_DATA_received.data() + offset, sizeof(int32_t));
					offset += sizeof(int32_t);
					std::vector<double> cellData(sizeofcell);
					memcpy(cellData.data(), BMAP_DATA_received.data() + offset, sizeofcell * sizeof(double));
					offset += sizeofcell * sizeof(double);
					session->fd->getCell(localx,localy)->setBMapValues(cellData.data());
				}
			}		
		}
	
	#endif
 		
	ostringstream cmd;
	cmd << "step[dt=" << dt <<"]";
	string scmd = cmd.str(); 
	executor.ExecuteCommand(scmd); 
	
/*
mnhPause = SimulationParameters::GetInstance()->getInt("MNHalt");
	while (mnhPause>0) {
		sleep(static_cast<unsigned int>(mnhPause));
		mnhPause = SimulationParameters::GetInstance()->g/has
		etInt("MNHalt");
		std::cout<<"setParameter[MNHalt=0] to restart, waiting for "<<mnhPause<<std::endl;
	}
	SimulationParameters::GetInstance()->setInt("MNHalt",0);
 */
	
	}
	



void FFGetDoubleArray(const char* mname, double t
		, double* x, size_t sizein, size_t sizeout){
	string tmpname(mname);
	double ct = executor.refTime + t;
	// searching for the layer to put data 
	DataLayer<double>* myLayer = session->fd->getDataLayer(tmpname); 

	if ( myLayer ){
			myLayer->setMatrix(tmpname, x, sizein, sizeout, ct);
		
	
			#ifdef MPI_COUPLING
			
		 	// Special case for altitude.. not to be sent
			if ( tmpname != "altitude"   ){
				
				FFArray<double>* t2;
				myLayer->getMatrix(&t2,0);
				MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
				MPI_Comm_size(MPI_COMM_WORLD, &world_size);
				if (world_rank == 0){
					DataLayer<double>* myMasterLayer = session->fdp->getDataLayer(tmpname);
					FFArray<double>* fullMatrix;
					myMasterLayer->getMatrix(&fullMatrix,0);
					FireDomain::distributedDomainInfo* DM = session->fdp->getParallelDomainInfo(1);
					fullMatrix->setDataAtLoc(t2->getData(),DM->atmoNX+2,DM->atmoNY+2,DM->refNX,DM->refNY,DM->ID);
					for (int nr = 1; nr < world_size; ++nr) {
						FireDomain::distributedDomainInfo* DR = session->fdp->getParallelDomainInfo(nr+1);
						size_t dsize = (DR->atmoNX+2)*(DR->atmoNY+2);
						std::vector<double> data_processed(dsize);
						MPI_Recv(data_processed.data(),dsize, MPI_DOUBLE, nr, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					    fullMatrix->setDataAtLoc(data_processed.data(),DR->atmoNX+2,DR->atmoNY+2,DR->refNX,DR->refNY,DR->ID);				
					}
					if (tmpname == "injectedAtGround"){
						double atmosphericTimeStep = SimulationParameters::GetInstance()->getDouble("atmosphericTimeStep");
						double atmoSurf = SimulationParameters::GetInstance()->getDouble("atmosphericCellLength") * SimulationParameters::GetInstance()->getDouble("atmosphericCellWidth");
						double sumInjected = SimulationParameters::GetInstance()->getDouble("sumInjected");

						
						sumInjected = sumInjected + ((fullMatrix->sum()) * atmosphericTimeStep );;
						SimulationParameters::GetInstance()->setDouble("sumInjected", sumInjected);

					}

					if (tmpname == "spotAtGround"){
						double atmosphericTimeStep = SimulationParameters::GetInstance()->getDouble("atmosphericTimeStep");
						double atmoSurf = SimulationParameters::GetInstance()->getDouble("atmosphericCellLength") * SimulationParameters::GetInstance()->getDouble("atmosphericCellWidth");
				
						double sumSedimented = SimulationParameters::GetInstance()->getDouble("sumSedimented");
						sumSedimented = sumSedimented + ((fullMatrix->sum()) * atmosphericTimeStep );;

						SimulationParameters::GetInstance()->setDouble("sumSedimented", sumSedimented);


					}


					//cout << updateBinStreamFrequency<<endl;
					if (std::fmod(t, updateBinStreamFrequency) < 1e-6) {

					 	if(tmpname == "spotAtGround"){
						//	cout <<"Time: "<< t<< " DT:"<< SimulationParameters::GetInstance()->getDouble("atmosphericTimeStep")<< " sumInjected: " << SimulationParameters::GetInstance()->getDouble("sumInjected") << " sumSedimented: " << SimulationParameters::GetInstance()->getDouble("sumSedimented") << endl;
						
							if (SimulationParameters::GetInstance()->isValued("reignitionThresholdFromSpotting")){
								double reignitionThreshold = SimulationParameters::GetInstance()->getDouble("reignitionThresholdFromSpotting");
								double reignitionMaxProbalilityValueFromSpotting = SimulationParameters::GetInstance()->getDouble("reignitionMaxProbalilityValueFromSpotting");
								 
								// --- Re‑ignition logic ---
								size_t nx = fullMatrix->getDim("x");
								size_t ny = fullMatrix->getDim("y");
								double* matrixData = fullMatrix->getData();

								// Random number generator (static so it is created only once)
								static std::mt19937 rng(std::random_device{}());
								std::uniform_real_distribution<double> unif(0.0, 1.0);
								
								for (size_t i = 2; i < nx-2; ++i) {
									for (size_t j = 2; j < ny-2; ++j) {
										const size_t idx = i * ny + j;
										double excess = matrixData[idx] - reignitionThreshold;

										if (excess <= 0.0)
											continue;                          // nothing to ignite here
										if (excess > 2*reignitionThreshold)
											continue;                          // nothing to ignite here
										

										
										if (unif(rng) < reignitionMaxProbalilityValueFromSpotting) {
											double igX = myMasterLayer->getOriginX()+
													((i+unif(rng)) * myMasterLayer->getDx()) ;
											double igY = myMasterLayer->getOriginY()+
													((j+unif(rng)) * myMasterLayer->getDy());
					

											std::ostringstream fireCmd;
										//	cout <<" nx << " << nx << " ny "<< ny << " i "<< i << " j "<< j << " idx "<< idx << " excess "<< idx << " DX "<<igX<<"  "<<igY<< endl;
											fireCmd << "startFire[loc=(" << igX << ", " << igY << ",0)]";
										//	cout << "Re-ignition command: " << fireCmd.str() << std::endl;
											//cout <<"Time: "<<t << "Re-ignition command: " << fireCmd.str() << std::endl;
											string scmd = fireCmd.str();
											executor.ExecuteCommand(scmd);
										}
									}
								}
								// --- End re‑ignition logic ---
							}
						}					
						std::string opath = SimulationParameters::GetInstance()->getParameter("genRawBytesDir");
						if (opath != "1234567890") {
							SimulationParameters::GetInstance()->isValued(tmpname+"Range");
							vector<double> range = SimulationParameters::GetInstance()->getDoubleArray(tmpname+"Range");
							if (range.size() == 2) {
								double vmin = range[0];
								double vmax = range[1];
								std::ostringstream fnoss;
								fnoss << opath << "/" << tmpname
									<< "_uint8_" << fullMatrix->getDim("x")
									<< "_" << fullMatrix->getDim("y") << ".dat";
								 
								std::string filename = fnoss.str();
								std::ofstream ofs(filename.c_str(), std::ios::binary | std::ios::app);
								if (ofs) {
									size_t total = fullMatrix->getSize();
									std::vector<uint8_t> buf(total);
									for (size_t i = 0; i < total; ++i) {
										double v = fullMatrix->getData()[i];
										double vnorm= (v - vmin) / (vmax - vmin);
										vnorm = std::min(std::max(vnorm, 0.0), 1.0);
										buf[i] = static_cast<uint8_t>(vnorm * std::numeric_limits<uint8_t>::max());
									}
									ofs.write(reinterpret_cast<const char*>(buf.data()), buf.size() * sizeof(uint8_t));
								}		
							}
				
						}
					}

				}else{				
					MPI_Send(t2->getData(), t2->getSize(), MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
				}
			}
			#endif 

	} 
	else {
		cout<<"MesoNH trying to set data for unknown layer :"<<tmpname<<endl;
	}
}

void MNHGoTo(double time){
	ostringstream cmd;
	cmd << "goTo[t=" << time <<"]";
	string scmd = cmd.str();
	executor.ExecuteCommand(scmd);
}

void executeMNHCommand(const char* cmd){
	string scmd(cmd);
	executor.ExecuteCommand(scmd);
}

void FFPutString(const char* mname, char* str){
	// NOT TO BE CALLED
}

void FFGetString(const char* mname, const char* str){
	string name(mname);
	string val(str);
	SimulationParameters::GetInstance()->setParameter(name, val);
}

void FFPutInt(const char* mname, int* n){
	string name(mname);
	*n = SimulationParameters::GetInstance()->getInt(name);
}

void FFGetInt(const char* mname, int* n){
	string name(mname);
	SimulationParameters::GetInstance()->setInt(name, *n);
}

void FFPutIntArray(const char* mname, int* x,
		size_t sizein, size_t sizeout){
	// TODO
}

void FFGetIntArray(const char* mname, double time
		, int* x, int sizein, int sizeout){
	// TODO
}

void FFPutDouble(const char* mname, double* x){
	string name(mname);
	*x = SimulationParameters::GetInstance()->getDouble(name);
}

void FFGetDouble(const char* mname, double* x){
	string name(mname); 
	SimulationParameters::GetInstance()->setDouble(name, *x);
}

void FFPutDoubleArray(const char* mname, double* x,
		size_t sizein, size_t sizeout){
	string tmpname(mname);
	// searching for concerned layer
 
 
	DataLayer<double>* myLayer = session->fd->getDataLayer(tmpname);



	if ( myLayer ){
		FFArray<double>* myMatrix;
		// getting the pointer
		myLayer->getMatrix(&myMatrix, executor.getTime());

		#ifdef MPI_COUPLING
			
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		
		if (tmpname == "BRatio"){
			double sumBR = myMatrix->sum();
			if (sumBR > 0.0){
				//	cout << world_rank <<" has "<< myMatrix->sum()<<" size M " <<myMatrix->getSize()<<" sizein "<< sizein << " sizeout "<< sizeout << endl;
			}
		
		}
		#endif

		myMatrix->copyDataToFortran(x);
	} else {
		cout<<"Error trying to put data from unknown layer "<<tmpname<<endl;
	}
}


void FFDumpDoubleArray(size_t nmodel, size_t nip, const char* mname, double t
		, double* x, size_t sizein, size_t ni, size_t nj, size_t nk, size_t sizeout){

	string tmpname(mname);
	ostringstream outputfile;
	double ct = executor.refTime + t;
	outputfile<<SimulationParameters::GetInstance()->getParameter("caseDirectory")<<'/'
			<<executor.outputDirs[nmodel-1]<<'/'
			<<SimulationParameters::GetInstance()->getParameter("outputFiles")
			<<"."<<nip<<"."<<tmpname;

	size_t niC = (size_t)(ni+0);
	size_t njC = (size_t)(nj+0);
	size_t nkC = (size_t)(nk+0);

	ofstream FileOut(outputfile.str().c_str(), ios_base::binary | ios::app);
	FileOut.write(reinterpret_cast<const char*>(&niC), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&njC), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nkC), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&ct), sizeof(double));


	try {
//	size_t indF = 0;
//		for ( indF = 0; indF < sizein; indF++ ) {
//                     FileOut.write(reinterpret_cast<const char*>(x+indF), sizeof(double));
//		}
		FileOut.write(reinterpret_cast<const char*>(x), sizein * sizeof(double));

	} catch (...) {
		cout << "Problem in passing a Fortran array to C array "
				<<tmpname<<endl;
	}

	FileOut.close();

}





}
