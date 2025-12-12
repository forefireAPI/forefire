/**
 * @file FDCell.cpp
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FDCell.h"
#include "Visitor.h"
#include "FluxLayer.h"

namespace libforefire {

const double FDCell::infinity = numeric_limits<double>::infinity();
bool FDCell::outputs = false;

FDCell::FDCell(FireDomain* fd, size_t nx, size_t ny) :
		domain(fd), mapSizeX(nx), mapSizeY(ny) {
	FFPoint defaultPoint = FFPoint();
	SWCorner = defaultPoint;
	NECorner = defaultPoint;
	int defaultIndex = -1;
	globalI = defaultIndex;
	globalJ = defaultIndex;
	mapSize = mapSizeX*mapSizeY;
	arrivalTimes = NULL;
	allocated = false;
	hasPassedCom = false;
	toDumpDomainID = 0;
	allDumped = false;
	
	}
FDCell::~FDCell() {
	if ( allocated ) delete arrivalTimes;
}

int operator==(const FDCell& left, const FDCell& right){
	return (left.SWCorner==right.SWCorner)&&(left.NECorner==right.NECorner);
}
int operator!=(const FDCell& left, const FDCell& right){
	return (left.SWCorner!=right.SWCorner)||(left.NECorner!=right.NECorner);
}

void FDCell::setCorners(const FFPoint& sw, const FFPoint& ne){
	SWCorner = sw;
	NECorner = ne;
	dx = (NECorner.getX()-SWCorner.getX())/mapSizeX;
	dy = (NECorner.getY()-SWCorner.getY())/mapSizeY;
}

void FDCell::setDomain(FireDomain* fd){
	domain = fd;
}

double FDCell::getArea(){
	return  (NECorner.getX()-SWCorner.getX())*(NECorner.getY()-SWCorner.getY());
}
double FDCell::getBmapElementArea(){
	return  dx*dy;
}

void FDCell::setGlobalCoordinates(const size_t& i, const size_t& j){
	globalI = i;
	globalJ = j;
}

void FDCell::setMatrixSize(const size_t& nx, const size_t& ny){
	mapSizeX = nx;
	mapSizeY = ny;
	mapSize = mapSizeX*mapSizeY;
}

void FDCell::setArrivalTime(const size_t& i, const size_t& j, double time){
	if ( !allocated ){
		arrivalTimes = new BurningMap(SWCorner, NECorner, mapSizeX, mapSizeY);
		allocated = true;
	}
	if ( time < (*arrivalTimes)(i,j) ) (*arrivalTimes)(i,j) = time;
}

double FDCell::getArrivalTime(const size_t& i, const size_t& j){
	if ( arrivalTimes == 0 ) return infinity;
	return (*arrivalTimes)(i,j);
}
void FDCell::loadBin(std::ifstream&  FileIn){
	if ( !allocated ){
		arrivalTimes = new BurningMap(SWCorner, NECorner, mapSizeX, mapSizeY);
		allocated = true;
	}
				
	arrivalTimes->loadBin(FileIn);
}

void FDCell::setBMapValues(const double* newVals){
	if ( !allocated ){
		arrivalTimes = new BurningMap(SWCorner, NECorner, mapSizeX, mapSizeY);
		allocated = true;
	}
				
	arrivalTimes->getMap()->setVal(newVals);
}

FireDomain* FDCell::getDomain(){
	return domain;
}

FFArray<double>* FDCell::getBurningMatrix(){
	return arrivalTimes->getMap();
}

size_t FDCell::getBMapSizeX(){
	return arrivalTimes->getSizeX();
}

size_t FDCell::getBMapSizeY(){
	return arrivalTimes->getSizeY();
}

BurningMap* FDCell::getBurningMap(){
	return arrivalTimes;
}

size_t FDCell::getI(){
	return globalI;
}

bool FDCell::isActive(){
	return allocated;
}

bool FDCell::isActiveForDump(){
	if (!allocated) return false;
	if (allDumped) return false;
	return true;
}
void FDCell::setIfAllDumped(){
	if (!allocated) return ;
	if (allDumped) return ;
	if(arrivalTimes->maxTime()<numeric_limits<double>::infinity()){
		allDumped = true;
	}
}



size_t FDCell::getJ(){
	return globalJ;
}

FFPoint& FDCell::getSWCorner(){
	return SWCorner;
}

FFPoint& FDCell::getNECorner(){
	return NECorner;
}

size_t FDCell::getNumFN(){
	return fireNodes.size();
}

bool FDCell::hasFiredInHalo(){
	return hasPassedCom;
}
void FDCell::setFiredInHalo(bool val){
	 hasPassedCom = val;
	 }

void FDCell::addFireNode(FireNode* fn){
	for ( ifn = fireNodes.begin(); ifn != fireNodes.end(); ++ifn ){
		if ( *ifn == fn ) return;
	}
	fireNodes.push_back(fn);
}

void FDCell::removeFireNode(FireNode* fn){
	fireNodes.remove(fn);
}

double FDCell::getBurningRatio(const double& t){
	/* if the burning map is not allocated */

	//if ( arrivalTimes == 0 ) return 0.;
	
	/* else getting the ratio and checking
	 * that there is still something burning*/
	double numBurningCells = 0;
	FFPoint center;
	center.setX(SWCorner.getX()+0.5*dx);
	
	for ( size_t i=0; i<mapSizeX; i++ ){
		center.setY(SWCorner.getY()+0.5*dy);
		for ( size_t j=0; j<mapSizeY; j++ ){
			if ( domain->isBurning(center, t) ) numBurningCells += 1;
			center.setY(center.getY()+dy);
		}
		center.setX(center.getX()+dx);
	}

	return numBurningCells/mapSize;
}

double FDCell::getMaxSpeed(const double& t){

	FFPoint center;
	center.setX(SWCorner.getX()+0.5*dx);
	for ( size_t i=0; i<mapSizeX; i++ ){
		center.setY(SWCorner.getY()+0.5*dy);
		for ( size_t j=0; j<mapSizeY; j++ ){
			center.setY(center.getY()+dy);
		}
		center.setX(center.getX()+dx);
	}

	FireNode fnode(domain);
	FFVector spL(1,1,0);
	fnode.setLoc(center);
	fnode.setVel(spL);

/*	if ( numBurningCells == 0 ) delete arrivalTimes;*/
	return domain->getPropagationSpeed(&fnode);;
}

int FDCell::activeModelsOnBmap(string layername,const double& t, int* modelCount){
	/* if the burning map is not allocated */

	if ( arrivalTimes == 0 ) return 0.;

	/* loading the flux layer */
	FluxLayer<double>* layer = domain->getFluxLayer(layername);

	/* else getting the ratio and checking
	 * that there is still something burning*/

	FFPoint center;
	int modelIndex;
	center.setX(SWCorner.getX()+0.5*dx);
	int value = 0;

	for ( size_t i = 0; i < mapSizeX; i++ ){
		center.setY(SWCorner.getY()+0.5*dy);
		for ( size_t j = 0; j < mapSizeY; j++ ){

			if ( (*arrivalTimes)(i,j) < t ) {
				modelIndex = layer->getFunctionIndexAt(center, t);
				if(modelIndex>-1){
					modelCount[modelIndex] = modelCount[modelIndex]+1;
				    value++;
				}
			}
			center.setY(center.getY()+dy);
		}
		center.setX(center.getX()+dx);
	}

	return value;
}


double FDCell::applyModelsOnBmap(string layername, const double& bt, const double& et,int* modelCount){
	/* if the burning map is not allocated */
	if ( arrivalTimes == 0 ){ 
		return 0.;
	}
	 
	/* loading the flux layer */
	FluxLayer<double>* layer = domain->getFluxLayer(layername);

	 
	/* else getting the ratio and checking
	 * that there is still something burning*/
	double cellFlux = 0.;
	FFPoint center;
	int modelIndex;
	center.setX(SWCorner.getX()+0.5*dx);
	double arrivalTime, value;
	 
	for ( size_t i = 0; i < mapSizeX; i++ ){
		center.setY(SWCorner.getY()+0.5*dy);
		for ( size_t j = 0; j < mapSizeY; j++ ){
			arrivalTime = (*arrivalTimes)(i,j);
			if ( arrivalTime < et ) {
				modelIndex = layer->getFunctionIndexAt(center, bt);
				// Return 0 if no model defined in the area
				value = modelIndex<0?0:domain->getModelValueAt(modelIndex, center, bt, et, arrivalTime);
				if(!std::isnan(value)){
					cellFlux += value;
					if (value > 0)
						modelCount[modelIndex] = modelCount[modelIndex]+1;
				}
			}
			center.setY(center.getY()+dy);
		}
		center.setX(center.getX()+dx);
	}
	/*	if ( cellFlux == 0. ) delete arrivalTimes;*/
	/* taking the mean */
	return cellFlux/mapSize;
}

void FDCell::interpolateArrivalTimes(Array2DdataLayer<double>* bmap
		, const int& year, const int& day, const int& time){
	if ( arrivalTimes != 0 ) delete arrivalTimes;
	arrivalTimes = new BurningMap(SWCorner, NECorner, mapSizeX, mapSizeY);
	bool relevant = false;
	double stubTime = 0.;
	for ( size_t i = 0; i < mapSizeX; i++ ){
		for ( size_t j = 0; j < mapSizeY; j++ ){
			(*arrivalTimes)(i,j) = bmap->getValueAt(arrivalTimes->getCenter(i, j), stubTime);
			if ( (*arrivalTimes)(i,j) != infinity ) relevant = true;
		}
	}
	if ( !relevant ){
		delete arrivalTimes;
		arrivalTimes = 0;
	}
}

FireNode* FDCell::getFirenodeByID(const long& sid){
	/* Searching for a firenode by ID */
	// searching in the cell
	list<FireNode*>::iterator tfn;
	for ( tfn = fireNodes.begin(); tfn != fireNodes.end(); ++tfn ){
		if ( (*tfn)->getID() != 0 and (*tfn)->getID() == sid ) return *tfn;
	}
	// No firenode was found with this ID
	return 0;
}

FireNode* FDCell::getFirenodeByID(const double& dsid){
	/* Searching for a firenode by ID */
	long lsid = domain->getIDfromDouble(dsid);
	return getFirenodeByID(lsid);
}

void FDCell::validateTopology(string call){
	/* validating the topology of each firenode */
	list<FireNode*>::iterator tfn;
	list<FireNode*> toBeTrashed;
	bool relevantPrev = true;
	bool relevantNext = true;
	ostringstream debugOutput;
	for ( tfn = fireNodes.begin(); tfn != fireNodes.end(); ++tfn ){
		/* testing the previous */
		if ( (*tfn)->getPrev()==0 ){
			debugOutput<<domain->getDomainID()<<": "
					<<(*tfn)->toShort()<<" has no previous"<<endl;
			relevantPrev = false;
		} else if ( (*tfn)->getPrev()->getNext() != *tfn ){
			debugOutput<<domain->getDomainID()<<": "
						<<(*tfn)->toShort()<<" has previous "
						<<(*tfn)->getPrev()->toShort()<<" whose next is ";
			debugOutput<<((*tfn)->getPrev()->getNext()==0?"0"
						:(*tfn)->getPrev()->getNext()->toShort())<<endl;
			relevantPrev = false;
		}

		/* testing the next */
		if ( (*tfn)->getNext()==0 ){
			debugOutput<<domain->getDomainID()<<": "
					<<(*tfn)->toShort()<<" has no next"<<endl;
			relevantNext = false;
		} else if ( (*tfn)->getNext()->getPrev() != *tfn ){
			debugOutput<<domain->getDomainID()<<": "
						<<(*tfn)->toShort()<<" has next "
						<<(*tfn)->getNext()->toShort()<<" whose next is ";
			debugOutput<<((*tfn)->getNext()->getPrev()==0?"0"
						:(*tfn)->getNext()->getPrev()->toShort())<<endl;
			relevantNext = false;
		}

		/* treating possible problems */
		if ( !relevantPrev and !relevantNext ) {
			// just trashing it
			debugOutput<<domain->getDomainID()<<": "<<'\t'<<"trashing "<<(*tfn)->toShort()
				<<" as no relevant next nor previous were found"<<endl;
			toBeTrashed.push_back(*tfn);
		} else if ( !relevantPrev ) {
			if ( (*tfn)->getPrev()->getNext() != (*tfn)->getNext() ){
				debugOutput<<(*tfn)->toString()<<endl<<domain->getDomainID()<<": "<<'\t'
						<<" the next of its previous is not himself";
				if ( call == "parallel" ){
					throw ParallelException(debugOutput.str(), "FDCell::validateTopology()");
				} else {
					throw TopologicalException(debugOutput.str(), "FDCell::validateTopology()");
				}
			} else {
				debugOutput<<domain->getDomainID()<<": "<<'\t'<<"trashing "<<(*tfn)->toShort()
					<<" as no relevant previous was found"<<endl;
				toBeTrashed.push_back(*tfn);
			}
		} else if ( !relevantNext ) {
			if ( (*tfn)->getNext()->getPrev() != (*tfn)->getPrev() ){
				debugOutput<<(*tfn)->toString()<<endl<<domain->getDomainID()<<": "<<'\t'
						<<" the previous of its next is not himself";
				if ( call == "parallel" ){
					throw ParallelException(debugOutput.str(), "FDCell::validateTopology()");
				} else {
					throw TopologicalException(debugOutput.str(), "FDCell::validateTopology()");
				}
			} else {
				debugOutput<<domain->getDomainID()<<": "<<'\t'<<"trashing "<<(*tfn)->toShort()
					<<" as no relevant next was found"<<endl;
				toBeTrashed.push_back(*tfn);
			}
		}
	}
	while ( !toBeTrashed.empty() ) {
		toBeTrashed.back()->setFront(0);
		domain->addToTrashNodes(toBeTrashed.back());
		toBeTrashed.pop_back();
	}
	if ( outputs ) cout<<debugOutput.str();
}

void FDCell::makeTrash(){
	arrivalTimes = 0;
	SWCorner = FFPoint(0,0,0);
	NECorner = FFPoint(100,100,0);
	globalI = 123456789;
	globalJ = 987654321;
}

string FDCell::toString(){
	ostringstream oss;
	oss<<"cell ("<<globalI<<","<<globalJ<<") ";
	return oss.str();
}


string FDCell::getFluxModelName(int fluxModelIndice){

	return domain->getFluxModelName(fluxModelIndice);
}

}
