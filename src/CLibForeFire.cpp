/*

Copyright (C) 2012 ForeFire Team, SPE, Universit� de Corse.

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

#include "CLibForeFire.h"
#include "SimulationParameters.h"

#ifdef MPI_COUPLING
#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdint>

#endif


using namespace std;

#ifdef MPI_COUPLING
// Dimensions des matrices
const int SPEED_ROWS = 100;
const int SPEED_COLS = 200;
const int BMAP_ROWS = 200;
const int BMAP_COLS = 400;

int step =0;
#endif



namespace libforefire {

Command executor;
static Command::Session* session = &(executor.currentSession);
    int world_rank;
    int world_size;



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

	std::cout<<"Initing Parallel MNH"<<id<< std::endl;
	

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

            // Initialiser les matrices
            std::vector<double> SPEED(SPEED_ROWS * SPEED_COLS, 0.0);
            std::vector<int32_t> BMAP(BMAP_ROWS * BMAP_COLS, 0);

            // Remplir SPEED et BMAP avec des valeurs initiales (par exemple, incrémentales)
            for (int i = 0; i < SPEED_ROWS * SPEED_COLS; ++i) {
                SPEED[i] = static_cast<double>(i);
            }

            for (int i = 0; i < BMAP_ROWS * BMAP_COLS; ++i) {
                BMAP[i] = static_cast<int32_t>(i);
            }

            // Attendre que tous les rangs aient envoyé leurs données (si nécessaire)
            // Ici, les messages sont déjà reçus dans la boucle précédente
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
	}
    // TODO	createNcFile(params->getParameter("ForeFireDataDirectory"), mdimx, mdimy, mdimz, meshx, meshy, zgrid);
	// Reading all the information on the initialization of ForeFire
	ostringstream initfile;
	if ( SimulationParameters::GetInstance()->getInt("parallelInit") != 1 ) {
		std::cout << "****************\n******rank "<< id <<" with "<<SimulationParameters::GetInstance()->getInt("parallelInit")<< std::endl ;
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
	// Managing the communication matrices for parallel computation
	if ( session->fd->parallel ) session->fd->createFirenodesMatrices();

 
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
		if ( !session->fd->addFluxLayer(tmpname) ){
			cout<<"WARNING: layer for "<<tmpname
					<<" could not be found within ForeFire framework, "
					<<"this should cause serious problems when coupling"<<endl;
		}
	}
}

void MNHStep(double dt){
	#ifdef MPI_COUPLING
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		//#   on a dans l ordre 
		//#  CALL SEND_GROUND_WIND_n(XUT, XVT, IKB, IINFO_ll)
		//#  CALL COUPLING_FOREFIRE_n(XTSTEP, ZSFTH, ZSFTQ, ZSFTS)
		//#    qui appelle CALL MNH_TO_FF_STEP(DT)
		//#    donc.. en gros 
		//#    la fonction FFGetDoubleArray(  est appelée n fois avant le step, pour tous les ranks
		//#
	//	std::cout << "*********** stepping **********"<<std::endl;
		if (world_rank == 0) {
		
			std::vector<double> SPEED(SPEED_ROWS * SPEED_COLS, 0.0);
			std::vector<int32_t> BMAP(BMAP_ROWS * BMAP_COLS, 0);
			for (int i = 0; i < SPEED_ROWS * SPEED_COLS; ++i) {
				SPEED[i] = static_cast<double>(i);
			}
			for (int i = 0; i < BMAP_ROWS * BMAP_COLS; ++i) {
				BMAP[i] = static_cast<int32_t>(i);
			}
			if(step++%25 == 0){
				cout<<"at "<<step*dt<<endl;
			}

			
		//	std::cout << "Rank " << world_rank <<" session "<<session->fdp->getDomainID()<< " : time " << step*dt <<" size "<<world_size<<"\n";
            
            std::vector<double> SPEED_right_half(SPEED_ROWS * (SPEED_COLS / 2));
            for (int i = 0; i < SPEED_ROWS; ++i) {
                for (int j = 0; j < SPEED_COLS / 2; ++j) {
                    SPEED_right_half[i * (SPEED_COLS / 2) + j] = SPEED[i * SPEED_COLS + (j + SPEED_COLS / 2)];
                }
            }

            // Extraire la portion de BMAP (200x200)
            std::vector<int32_t> BMAP_slice(BMAP_ROWS * (BMAP_COLS / 2));
            for (int i = 0; i < BMAP_ROWS; ++i) {
                for (int j = 0; j < BMAP_COLS / 2; ++j) {
                    BMAP_slice[i * (BMAP_COLS / 2) + j] = BMAP[i * BMAP_COLS + j];
                }
            }
			for (int nr = 1; nr < world_size; ++nr) {
				MPI_Send(BMAP_slice.data(), BMAP_ROWS * (BMAP_COLS / 2), MPI_INT32_T, nr, 0, MPI_COMM_WORLD);
			}
 
    }
    else if (world_rank >= 1) {
            std::vector<int32_t> BMAP_received(BMAP_ROWS * (BMAP_COLS / 2));
            MPI_Recv(BMAP_received.data(), BMAP_ROWS * (BMAP_COLS / 2), MPI_INT32_T, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    }

	#endif
	ostringstream cmd;
	cmd << "step[dt=" << dt <<"]";
	string scmd = cmd.str();
	executor.ExecuteCommand(scmd);
}


void FFGetDoubleArray(const char* mname, double t
		, double* x, size_t sizein, size_t sizeout){
	string tmpname(mname);
	double ct = executor.refTime + t;
	// searching for the layer to put data
	//cout<<session->fd->getDomainID()<<" is getting "<<tmpname<<endl;

	DataLayer<double>* myLayer = session->fd->getDataLayer(tmpname);
	if ( myLayer ){
		myLayer->setMatrix(tmpname, x, sizein, sizeout, ct);
		#ifdef MPI_COUPLING
		FFArray<double>* t2;
		myLayer->getMatrix(&t2,0);
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		//cout<<world_rank<<" sett "<< world_size <<" ing "<<sizein<<" bytes for "<<tmpname<<endl;
		if ( tmpname == "windU"  or tmpname == "windV" ){

			

			
			if (world_rank == 0){
				DataLayer<double>* myMasterLayer = session->fdp->getDataLayer(tmpname);
				//if (params->getParameter("runmode") != "masterMNH") return;x
				//session->fdp fait un set de la matrice reçue à la position 0
				FFArray<double>* fullMatrix;
				myMasterLayer->getMatrix(&fullMatrix,0);


				FireDomain::distributedDomainInfo* DM = session->fdp->getParallelDomainInfo(1);
				//cout<<"rank 0 "<<DM->atmoNX<<"  "<<DM->atmoNY+2<<"  "<<DM->refNX<<"  "<<DM->refNY<<endl;

				fullMatrix->setDataAtLoc(t2->getData(),DM->atmoNX+2,DM->atmoNY+2,DM->refNX,DM->refNY,DM->ID);

				for (int nr = 1; nr < world_size; ++nr) {
					FireDomain::distributedDomainInfo* DR = session->fdp->getParallelDomainInfo(nr+1);
					size_t dsize = (DR->atmoNX+2)*(DR->atmoNY+2);
					std::vector<double> data_processed(dsize);
					
					MPI_Recv(data_processed.data(),dsize, MPI_DOUBLE, nr, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					fullMatrix->setDataAtLoc(data_processed.data(),DR->atmoNX+2,DR->atmoNY+2,DR->refNX,DR->refNY,DR->ID);
				//	fdp->loadPartialArray(data_processed,nr,t2->getSize());
					//session->fdp fait un set de à la position des ranks
				
				}
			}else{
				// need to send the matrix here !!!
			//	cout<<world_rank<<" "<<tmpname<<" D1M:"<< t2->getSize()<<" vs READSIN:"<< sizein<<" vs READSout:"<<endl;
				
				MPI_Send(t2->getData(), t2->getSize(), MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
			
			}
		
		}

		#endif 

	} else {
		cout<<"Error trying to get data for unknown layer "<<tmpname<<endl;
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

	//cout<<session->fd->getDomainID()<<" is putting "<<tmpname<<endl;
	DataLayer<double>* myLayer = session->fd->getDataLayer(tmpname);

	if ( myLayer ){
		FFArray<double>* myMatrix;
		// getting the pointer
		myLayer->getMatrix(&myMatrix, executor.getTime());

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

	size_t indF = 0;
	try {
		for ( indF = 0; indF < sizein; indF++ ) {
                     FileOut.write(reinterpret_cast<const char*>(x+indF), sizeof(double));
		}
	} catch (...) {
		cout << "Problem in passing a Fortran array to C array "
				<<tmpname<<endl;
	}

	FileOut.close();

}


void saveNcRecord(int rec){cout << "CLibforefire:: saveNcRecord " << " newCDF Not Implemented" << endl;}
void createNcFile(string filename
		, const int& consted_ni, const int& consted_nj, const int& consted_nk
		, const double* meshx, const double* meshy, const double* zgrid){
			cout << "CLibforefire:: createNcFile " << " newCDF Not Implemented" << endl;
		}


}
