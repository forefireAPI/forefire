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

#ifndef FLUXLAYER_H_
#define FLUXLAYER_H_

#include "DataLayer.h"
#include "FDCell.h"
#include "FFArrays.h"
#include "DataBroker.h"

using namespace std;

namespace libforefire {

/*! \class FluxLayer
 * \brief FluxLayer gives access to the desired flux model
 *
 *  FluxLayer gives access to the desired flux model.
 */

template<typename T> class FluxLayer : public DataLayer<T> {

	/* Data for fluxes on atmospheric cells */
	FFPoint SWCorner, NECorner;
	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the X direction */
	size_t size; /*!< size of the array */

	double dx; /*!< increment in the X direction */
	double dy; /*!< increment in the Y direction */

	FFArray<T>* flux; /*!< pointer to the array of fluxes */

	FDCell** cells; /*!< pointers to the atmospheric cells */

	/* Data for the map of the functions to compute the fluxes */

	int* fluxModelIndexMap; /*!< pointer to the array of burning ratios */

	double mapSWCornerX; /*!< origin of the map in the X direction */
	double mapSWCornerY; /*!< origin of the map in the Y direction */
	double mapSWCornerZ; /*!< origin of the map in the Z direction */
	double mapStartTime; /*!< origin of the map for time */

	double mapNECornerX; /*!< end of the map in the X direction */
	double mapNECornerY; /*!< end of the map in the Y direction */
	double mapNECornerZ; /*!< end of the map in the Z direction */
	double mapEndTime; /*!< end of the map for time */

	size_t mapNx; /*!< size of the map in the X direction */
	size_t mapNy; /*!< size of the map in the Y direction */
	size_t mapNz; /*!< size of the map in the Z direction */
	size_t mapNt; /*!< size of the map in the T direction */
	size_t mapSize; /*!< size of the map */

	double mapDx; /*!< increment in the X direction */
	double mapDy; /*!< increment in the Y direction */
	double mapDz; /*!< increment in the Z direction */
	double mapDt; /*!< increment in the T direction */

	double latestCallGetMatrix; /*!< time of the latest call to getMatrix() */
	double latestCallInstantaneousFlux; /*!< time of the latest call to getInstantaneousFlux() */


	SimulationParameters* params;

	/*! \brief obtains the position in the array for given location and time */
	size_t getPosInMap(FFPoint&, const double&);

	T getNearestData(FFPoint, double);

public:
	/*! \brief Default constructor */
	FluxLayer() : DataLayer<T>() {};
	/*! \brief Constructor with no flux model map */
	FluxLayer(string name, FFPoint& atmoSWCorner, FFPoint& atmoNECorner
			, const size_t& nnx, const size_t& nny, FDCell** FDcells, const int& index)
	: DataLayer<T>(name), SWCorner(atmoSWCorner), NECorner(atmoNECorner)
	  	  , nx(nnx), ny(nny), cells(FDcells) {

		size = nx*ny;
		flux = new FFArray<T>(name, 0., nx, ny);

		mapSize = 1;
		fluxModelIndexMap = new int[mapSize];
		*fluxModelIndexMap = index;

		mapSWCornerX = atmoSWCorner.getX();
		mapSWCornerY = atmoSWCorner.getY();
		mapSWCornerZ = atmoSWCorner.getZ();
		mapNECornerX = atmoNECorner.getX();
		mapNECornerY = atmoNECorner.getY();
		mapNECornerZ = atmoNECorner.getZ();
		mapEndTime = 1.;

		dx = (NECorner.getX()-SWCorner.getX())/nx;
		dy = (NECorner.getY()-SWCorner.getY())/ny;
		mapDx = 1;
		mapDy = 1;
		mapDz = 1;
		mapDt = 1;

		latestCallGetMatrix = -1.;
		latestCallInstantaneousFlux = -1.;

		params = SimulationParameters::GetInstance();
	}
	/*! \brief Constructor with all necessary information */
	FluxLayer(string name, FFPoint& atmoSWCorner, FFPoint& atmoNECorner
			, const size_t& nnx, const size_t& nny, FDCell** FDcells
			, int* map, FFPoint& swc, double& t0
			, FFPoint& extent, double& timespan
			, size_t& nmx, size_t& nmy, size_t& nmz, size_t& nmt)
	: DataLayer<T>(name), SWCorner(atmoSWCorner), NECorner(atmoNECorner)
	  	  , nx(nnx), ny(nny), cells(FDcells), mapStartTime(t0)
	  	  , mapNx(nmx), mapNy(nmy), mapNz(nmz), mapNt(nmt) {

		size = nx*ny;
		flux = new FFArray<T>(name, 0., nx, ny);

		mapSize = (size_t) mapNx*mapNy*mapNz*mapNt;
		mapSWCornerX = swc.getX();
		mapSWCornerY = swc.getY();
		mapSWCornerZ = swc.getZ();
		mapNECornerX = mapSWCornerX + extent.getX();
		mapNECornerY = mapSWCornerY + extent.getY();
		mapNECornerZ = mapSWCornerZ + extent.getZ();
		mapEndTime = mapStartTime + timespan;
		fluxModelIndexMap = new int[mapSize];
		for ( size_t i = 0; i<mapSize; i++ ) fluxModelIndexMap[i] = map[i];

		dx = (NECorner.getX()-SWCorner.getX())/nx;
		dy = (NECorner.getY()-SWCorner.getY())/ny;
		mapDx = extent.getX()/mapNx;
		mapDy = extent.getY()/mapNy;
		mapDz = extent.getZ()/mapNz;
		mapDt = timespan/mapNt;

		latestCallGetMatrix = -1.;
		latestCallInstantaneousFlux = -1.;

		params = SimulationParameters::GetInstance();
	};
	/*! \brief Destructor */
	virtual ~FluxLayer(){
		if ( fluxModelIndexMap != 0 ) delete [] fluxModelIndexMap;
		delete flux;
	}

	/*! \brief gets the desired function index at a given location */
	int getFunctionIndexAt(FFPoint&, const double&);
	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);
	void setValueAt(FFPoint loc,  double tval, T value){};
	/*! \brief initialize latestCallGetMatrix for the beginning of the simulation */
	void setFirstCall(const double&);
	/*! \brief getter to the pointer on the FFArray */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a fortran array into the FFArray */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);

	/*! \brief getter to the pointer on the FFArray */
	void getInstantaneousFlux(FFArray<T>**, const double&);

	/*! \brief fill an aray with active bmap cells */
	int computeActiveMatrix(const double& , int*);

	/*! \brief print the related FFArray */
	string print2D(size_t, size_t);
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);
			
	double getDx(){ return getWidth()/nx; };
	double getDy(){ return getHeight()/ny; };
	double getDz(){ return 0; };
	double getOriginX(	){ return SWCorner.getX(); };
	double getOriginY(){ return SWCorner.getY(); };
	double getOriginZ(){ return SWCorner.getZ(); };
	double getWidth(){ return NECorner.getX()-SWCorner.getX(); };
	double getHeight(){ return NECorner.getY()-SWCorner.getY(); };
	double getDepth(){ return 0; };
		

};

template<typename T>
size_t FluxLayer<T>::getPosInMap(FFPoint& loc, const double& t){
	int i, j, k , tt;
	mapNx>1 ? i = ((int) (loc.getX()-mapSWCornerX)/mapDx) : i = 0;
	mapNy>1 ? j = ((int) (loc.getY()-mapSWCornerY)/mapDy) : j = 0;
	mapNz>1 ? k = ((int) (loc.getZ()-mapSWCornerZ)/mapDz) : k = 0;
	mapNt>1 ? tt = ((int) (t-mapStartTime)/mapDt) : tt = 0;

	if ( i < 0 or i > (int) mapNx-1 ){

		return 0;
	}
	if ( j < 0 or j > (int) mapNy-1 ){

		return 0;
	}
	if ( k < 0 or k > (int) mapNz-1 ){

		return 0;
	}
	if ( tt < 0 or tt > (int) mapNt-1 ){

		return 0;
	}
	return (size_t) i*mapNy*mapNz*mapNt + j*mapNz*mapNt + k*mapNt + tt;
}

template<typename T>
int FluxLayer<T>::getFunctionIndexAt(FFPoint& loc, const double& t){
	if ( mapSize > 1 ) return fluxModelIndexMap[getPosInMap(loc, t)];
	return fluxModelIndexMap[0];
}

template<typename T>
T FluxLayer<T>::getValueAt(FireNode* fn){
	return getNearestData(fn->getLoc(), fn->getTime());
}

template<typename T>
T FluxLayer<T>::getValueAt(FFPoint loc, const double& t){
	return getNearestData(loc, t);
}

template<typename T>
T FluxLayer<T>::getNearestData(FFPoint loc, double t){
	if ( loc.getX() < SWCorner.getX() or loc.getX() > NECorner.getX() ){

		return 0;
	}
	if ( loc.getY() < SWCorner.getY() or loc.getY() > NECorner.getY() ){

		return 0;
	}

	size_t i, j;

	nx > 1 ? i = ((size_t) (loc.getX()-SWCorner.getX())/dx) : i = 0;
	ny > 1 ? j = ((size_t) (loc.getY()-SWCorner.getY())/dy) : j = 0;

	return (*flux)(i, j);
}

template<typename T>
size_t FluxLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	cout<<"WARNING: FluxLayer<T>::getValuesAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
size_t FluxLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	cout<<"WARNING: FluxLayer<T>::getValuesAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
void FluxLayer<T>::setFirstCall(const double& t){
	latestCallGetMatrix = t;
}

template<typename T>
int FluxLayer<T>::computeActiveMatrix(const double& t, int* modelCount){
   int totalCount = 0;
		string fluxName = this->getKey();
		for ( size_t i = 0; i < nx; i++ ){
			for ( size_t j = 0; j < ny; j++ ){
				totalCount += cells[i][j].activeModelsOnBmap(fluxName,  t, modelCount);
			}
		}
    return totalCount;
}

template<typename T>
void FluxLayer<T>::getMatrix(FFArray<T>** matrix, const double& t){

	if ( t != latestCallGetMatrix ){

		// TODO computing active area
		string fluxName = this->getKey();
		int numFluxModelsMax = 50;
		int modelCount[numFluxModelsMax];

	   	for (int i = 0; i < numFluxModelsMax; i++) modelCount[i]= 0;
	   	int totalcount = 0;
	   	totalcount = 0;

	   	for ( size_t i = 0; i < nx; i++ ){
			for ( size_t j = 0; j < ny; j++ ){
				(*flux)(i,j) = cells[i][j].applyModelsOnBmap(fluxName, latestCallGetMatrix, t, modelCount);
			}
		}

	    for (int i = 0; i < numFluxModelsMax; i++) {
	    	string modelName=cells[0][0].getFluxModelName(i);
	    	if (!modelName.empty()){
	    		params->setDouble(modelName+".activeArea",modelCount[i]*cells[0][0].getBmapElementArea());
	    		totalcount += modelCount[i];
	    	}
	    }

	   	for ( size_t i = 0; i < nx; i++ ){
			for ( size_t j = 0; j < ny; j++ ){
				(*flux)(i,j) = cells[i][j].applyModelsOnBmap(fluxName, latestCallGetMatrix, t, modelCount);
			}
		}

	    params->setDouble(fluxName+".activeArea",totalcount*cells[0][0].getBmapElementArea());
		latestCallGetMatrix = t;

	}
	// Affecting the computed matrix to the desired array
	*matrix = flux;
	if ( params->getInt("surfaceOutputs") != 0 ) {
		// dumping in a binary file for output
		FFPoint plotOrigin = FFPoint();
		ostringstream oss;
		oss<<params->getParameter("ffOutputsPattern");
		dumpAsBinary(oss.str(), t, plotOrigin, plotOrigin, nx, ny);
	}
}

template<typename T>
void FluxLayer<T>::getInstantaneousFlux(FFArray<T>** matrix, const double& t){
	string fluxName = this->getKey();
	int numFluxModelsMax = 50;
	int modelCount[numFluxModelsMax];
   	int totalcount = 0;
   	for (int i = 0; i < numFluxModelsMax; i++) modelCount[i]= 0;

	if ( t != latestCallInstantaneousFlux ){
		// computing the instantaneous flux
		string fluxName = this->getKey();
		for ( size_t i = 0; i < nx; i++ ){
			for ( size_t j = 0; j < ny; j++ ){
				(*flux)(i,j) = cells[i][j].applyModelsOnBmap(fluxName, t, t, modelCount);
			}
		}

		for (int i = 0; i < numFluxModelsMax; i++) {
			    	string modelName=cells[0][0].getFluxModelName(i);
			    	if (!modelName.empty()){
			    		params->setDouble(modelName+".activeArea",modelCount[i]*cells[0][0].getBmapElementArea());
			    		totalcount += modelCount[i];
			    	}
			    }
			    params->setDouble(fluxName+".activeArea",totalcount*cells[0][0].getBmapElementArea());

		latestCallInstantaneousFlux = t;
	}
	// Affecting the computed matrix to the desired array
	*matrix = flux;
}

template<typename T>
void FluxLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	if ( flux->getSize() == sizein ){
		flux->copyDataFromFortran(inMatrix);
	} else {
		cout<<"Error while trying to retrieve "<<mname<<"data for data layer "
				<<this->getKey()<<" at "<<time<<" size "<<sizeout<<endl;
	
	}
}

template<typename T>
string FluxLayer<T>::print(){
	return print2D(0,0);
}

template<typename T>
string FluxLayer<T>::print2D(size_t i, size_t j){
	return flux->print2D(i,j);
}

template<typename T>
void FluxLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){
	/* writing the matrix in a binary file */
	if(nnx*nny==0){
		cout<<"trying to dump empty field at "<<time<<" loc"<<SWC.getX()<<":"<<SWC.getY()<<" "<<NEC.getX()<<":"<<NEC.getY()<<endl;
	}
	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();
	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);
	FileOut.write(reinterpret_cast<const char*>(&nnx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(flux->getData()), flux->getSize()*sizeof(T));
	FileOut.close();
}

} /* namespace libforefire */
#endif /* FLUXLAYER_H_ */
