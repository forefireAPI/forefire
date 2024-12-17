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

#include "Halo.h"
#include "FireDomain.h"

namespace libforefire {

// Static variables
const string Halo::FireNodesId = "FireNodesId";
const string Halo::FireNodesPosX = "FireNodesPosX";
const string Halo::FireNodesPosY = "FireNodesPosY";
const string Halo::FireNodesVelX = "FireNodesVelX";
const string Halo::FireNodesVelY = "FireNodesVelY";
const string Halo::FireNodesTime = "FireNodesTime";

bool Halo::outputs = false;

Halo::Halo() {
}

Halo::Halo(string hname, FireDomain* fd, list<FDCell*>& haloCells
		, const size_t& istart, const size_t& jstart
		, const size_t& iend, const size_t& jend
		, const size_t& iinc, const size_t& jinc)
: name(hname) ,domain(fd), iComStart(istart), jComStart(jstart)
, iComEnd(iend), jComEnd(jend), iComIncrement(iinc), jComIncrement(jinc) {
	cells = haloCells;
	iCurrent = iComStart;
	jCurrent = jComStart;
	kCurrent = 0;
	limSup = -100000000000;
	limInf = 100000000000;
	chainComSpread = std::sqrt(cells.front()->getArea())/1.5;
	init = true;
	isActive = true;
	hasTriggered = false;
	verticalHalo=true;
	
}

Halo::~Halo() {
	// nothing to do
}
void Halo::setLimits(double inf, double sup){
	limSup = sup;
	limInf = inf;
}
void Halo::writeHaloNodeList(const double& endChain, const double& endCom){
 	if(hasTriggered) return ;
	list<FireNodeData*> haloList;
	double dpid, dnid;
	
	/* clearing the preceding list and memory space */
	while ( !nodeDataList.empty() ){
		delete nodeDataList.back();
		nodeDataList.pop_back();
	}

	/*-------------------------------*/
	/* constructing the list of data */
	/*-------------------------------*/

	list<FDCell*>::iterator cell;
	list<FireNode*>::iterator ifn;
	double maxX = -1000000000;
	double minX = 1000000000;
	double maxY = -1000000000;
	double minY = 1000000000;
	double newlimInf = limInf;
	double newlimSup = limSup;

	for ( cell = cells.begin(); cell != cells.end(); ++cell ){
		for ( ifn = (*cell)->fireNodes.begin(); ifn != (*cell)->fireNodes.end(); ++ifn ) {
			(*ifn)->getPrev() == 0 ? dpid = 0. : dpid = (*ifn)->getPrev()->getIDtoDouble();
			(*ifn)->getNext() == 0 ? dnid = 0. : dnid = (*ifn)->getNext()->getIDtoDouble();
			maxX = std::max(maxX,(*ifn)->getX());
			minX = std::min(minX,(*ifn)->getX());
			maxY = std::max(maxY,(*ifn)->getY());
			minY = std::min(minY,(*ifn)->getY());
			//
			haloList.push_back(new FireNodeData((*ifn)->getIDtoDouble()
					, (*ifn)->getX(), (*ifn)->getY(), (*ifn)->getVx(), (*ifn)->getVy()
					, (*ifn)->getTime(), dpid, dnid));
		}
			
	
	}

	bool haloReady = false;
	
	if ( this == domain->southInnerHalo ) verticalHalo = false;
	if ( this == domain->northInnerHalo ) verticalHalo = false;
	if ( this == domain->southOuterHalo ) verticalHalo= false;
	if ( this == domain->northOuterHalo ) verticalHalo = false;
	// je desactive les cells qui ont déjà fourni... et puis il faut checker qua c'est assez grand.. max x/max y supérieur à la largeur ou longeur de la taille de cell, un fois envoyé je
	if (haloList.size() > 0){
			if (verticalHalo){
				if((maxX - minX ) > chainComSpread){
					if ((minY > newlimSup) || (maxY < newlimInf)) {
								newlimSup = std::max(maxY,newlimSup);
								newlimInf = std::min(minY,newlimInf);
							//	cout<<"Vertical write " << domain->getDomainID()<< " at time  "<<domain->getTime()<<" spread "<<  maxX - minX <<" loc "<<  minY <<":"<<maxY <<" lim  "<<  limInf<<":"<<limSup <<endl; 
								haloReady = true;
						}
					}
			}else{
				if((maxY - minY ) > chainComSpread){
					if ((minX > newlimSup) || (maxX < newlimInf)) {
								newlimSup = std::max(maxX,newlimSup);
								newlimInf = std::min(minX,newlimInf);
							//	cout<<"Horizontal write " << domain->getDomainID()<< " at time  "<<domain->getTime()<<" spread "<<  maxY - minY <<" loc "<<  minX <<":"<<maxX <<" lim  "<<  limInf<<":"<<limSup <<endl; 
								haloReady = true;
						}
					}
			}
		
	 
			
				
	}

	if(!haloReady) {haloList.clear();}
 
	/* Arranging the list of data following the chains */
	list<FireNodeData*> currentChain;
	list<FireNodeData*>::iterator ifnd;
	list<FireNodeData*>::iterator newifnd;
	FireNodeData* tmpdata;
	 while ( !haloList.empty() ){

		currentChain.clear();

		/* choosing a seed for the chain */
		FireNodeData* seed = haloList.front(); 
		tmpdata = seed;
		currentChain.push_front(tmpdata);
		haloList.remove(tmpdata);
		/* Populating the list with next
		 * nodes within incoming ones */
		tmpdata = domain->idInList(seed->nid, haloList);
		while ( tmpdata ){
			currentChain.push_back(tmpdata);
			haloList.remove(tmpdata);
			tmpdata = domain->idInList(tmpdata->nid, haloList);
		}
		/* Populating the list with previous
		 * nodes within incoming ones */
		tmpdata = domain->idInList(seed->pid, haloList);
		while ( tmpdata ){
			currentChain.push_front(tmpdata);
			haloList.remove(tmpdata);
			tmpdata = domain->idInList(tmpdata->pid, haloList);
		}

		/* Handling of the link nodes */
		FireNode* firstOfChain = domain->getFireNodeByIDNeighborCells(
				currentChain.front()->id, domain->getCell(currentChain.front()));
		if ( firstOfChain != 0 ){
			FireNode* prevOfChain = firstOfChain->getPrev();
			currentChain.push_front(new FireNodeData(
					prevOfChain->getIDtoDouble()
					, prevOfChain->getX(), prevOfChain->getY()
					, prevOfChain->getVx(), prevOfChain->getVy()
					, prevOfChain->getTime()));
		}

		FireNode* lastOfChain = domain->getFireNodeByIDNeighborCells(
					currentChain.back()->id, domain->getCell(currentChain.back()));
		if ( lastOfChain != 0 ){
			FireNode* nextOfChain = lastOfChain->getNext();
			currentChain.push_back(new FireNodeData(
					nextOfChain->getIDtoDouble()
					, nextOfChain->getX(), nextOfChain->getY()
					, nextOfChain->getVx(), nextOfChain->getVy()
					, nextOfChain->getTime()));
		}

		/* pushing the current chain in the arranged halo list.
		 * If it is initialization, sending just the valid chains */ 
		if ( isValid(currentChain) ){
				
			for ( newifnd = currentChain.begin();
					newifnd != currentChain.end(); ++newifnd ){
				nodeDataList.push_back(*newifnd);
			}
			/* pushing the marker for the end of the chain */
			nodeDataList.push_back(new FireNodeData(
					0, 0, 0, 0, 0, endChain));
		}else{
				cout<<domain->getDomainID()<<" WARNING: not valid chain at time"<<domain->getTime()<<" nothing wrote"<<endl;
			
		}

	}
	 
	if ( init ) init = false;

	/* Loading the communication matrices */
	FFArray<double>* id;
	domain->getDataLayer(FireNodesId)->getMatrix(&id, 0.);
	FFArray<double>* posX;
	domain->getDataLayer(FireNodesPosX)->getMatrix(&posX, 0.);
	FFArray<double>* posY;
	domain->getDataLayer(FireNodesPosY)->getMatrix(&posY, 0.);
	FFArray<double>* velX;
	domain->getDataLayer(FireNodesVelX)->getMatrix(&velX, 0.);
	FFArray<double>* velY;
	domain->getDataLayer(FireNodesVelY)->getMatrix(&velY, 0.);
	FFArray<double>* time;
	domain->getDataLayer(FireNodesTime)->getMatrix(&time, 0.);

	list<FireNodeData*>::iterator haloNode;
	size_t i, j, k;

	initializePositionInMatrix();
	getNewPositionInMatrix(i, j, k);

	/* writing in the matrices, just before copy data to fortran and ATMO*/
	for ( haloNode = nodeDataList.begin();
			haloNode != nodeDataList.end(); ++haloNode ){
		(*posX)(i,j,k) = (*haloNode)->posX;
		(*posY)(i,j,k) = (*haloNode)->posY;
		(*velX)(i,j,k) = (*haloNode)->velX;
		(*velY)(i,j,k) = (*haloNode)->velY;
		(*time)(i,j,k) = (*haloNode)->time;
		(*id)(i,j,k) = (*haloNode)->id;
		getNewPositionInMatrix(i, j, k);
	}
	(*time)(i,j,k) = endCom;
	if (nodeDataList.size()>0){
 
	
				if ( this == domain->southOuterHalo ) cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in southOuterHalo"<<endl;
				if ( this == domain->southInnerHalo ) cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in southInnerHalo"<<endl;
				if ( this == domain->westOuterHalo )  cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in westOuterHalo"<<endl;
				if ( this == domain->westInnerHalo ) cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in westInnerHalo"<<endl;
				if ( this == domain->northOuterHalo )  cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in northOuterHalo"<<endl;
				if ( this == domain->northInnerHalo ) cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in northInnerHalo"<<endl;
				if ( this == domain->eastOuterHalo )  cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in eastOuterHalo"<<endl;
				if ( this == domain->eastInnerHalo ) cout<<domain->getDomainID()<<" wrote "<<nodeDataList.size()<<" in eastInnerHalo"<<endl;
				this->setLimits(newlimInf,newlimSup);
				if ( this == domain->southInnerHalo ) domain->southOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->westInnerHalo ) domain->westOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->northInnerHalo ) domain->northOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->eastInnerHalo ) domain->eastOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->southOuterHalo ) domain->southInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->westOuterHalo ) domain->westInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->northOuterHalo ) domain->northInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->eastOuterHalo ) domain->eastInnerHalo->setLimits(newlimInf,newlimSup);
	 
	}
}

// reading from ATMO to create FNode info
void Halo::readHaloNodeList(const double& endCom, const double& noCom){
  	/* Loading the communication matrices */
	FFArray<double>* id;
	domain->getDataLayer(FireNodesId)->getMatrix(&id, 0.);
	FFArray<double>* posX;
	domain->getDataLayer(FireNodesPosX)->getMatrix(&posX, 0.);
	FFArray<double>* posY;
	domain->getDataLayer(FireNodesPosY)->getMatrix(&posY, 0.);
	FFArray<double>* velX;
	domain->getDataLayer(FireNodesVelX)->getMatrix(&velX, 0.);
	FFArray<double>* velY;
	domain->getDataLayer(FireNodesVelY)->getMatrix(&velY, 0.);
	FFArray<double>* time;
	domain->getDataLayer(FireNodesTime)->getMatrix(&time, 0.);

	double tmpx, tmpy, tmpvx, tmpvy, t, sid;

	size_t i, j, k;
	FireNodeData* tmpData;
	double maxX = -1000000000;
	double minX = 1000000000;
	double maxY = -1000000000;
	double minY = 1000000000;
	double newlimInf = limInf;
	double newlimSup = limSup;

	/* clearing the preceding list and memory space */
	nodeDataList.clear();
	initializePositionInMatrix();
	getNewPositionInMatrix(i, j, k);

	if ( (*time)(i,j,k) == noCom ) {
		isActive = false;
		if ( this == domain->southOuterHalo ) domain->southInnerHalo->isActive = false;
		if ( this == domain->westOuterHalo ) domain->westInnerHalo->isActive = false;
		if ( this == domain->northOuterHalo ) domain->northInnerHalo->isActive = false;
		if ( this == domain->eastOuterHalo ) domain->eastInnerHalo->isActive = false;
		return;
	}
	 	
	bool haloReady = false;

	if ( this == domain->southInnerHalo ) verticalHalo = false;
	if ( this == domain->northInnerHalo ) verticalHalo = false;
	if ( this == domain->southOuterHalo ) verticalHalo= false;
	if ( this == domain->northOuterHalo ) verticalHalo = false;
	int countNode = 0;
	
	while ( (*time)(i,j,k) > endCom ) {
		// getting the data
		sid = (*id)(i,j,k);
		tmpx = (*posX)(i,j,k);
		tmpy = (*posY)(i,j,k);
		tmpvx = (*velX)(i,j,k);
		tmpvy = (*velY)(i,j,k);
		t = (*time)(i,j,k);
		
		
		tmpData = new FireNodeData(sid, tmpx, tmpy, tmpvx, tmpvy, t);

		maxX = std::max(maxX,tmpx);
		minX = std::min(minX,tmpx);
		maxY = std::max(maxY,tmpy);
		minY = std::min(minY,tmpy);
		nodeDataList.push_back(tmpData);

		getNewPositionInMatrix(i, j, k)	; 
		countNode++;		
		
	} 

			if (verticalHalo){
				if((maxX - minX ) > chainComSpread){
					if ((minY > newlimSup) || (maxY < newlimInf)) {
								newlimSup = std::max(maxY,newlimSup);
								newlimInf = std::min(minY,newlimInf);
							//	cout<<"Vertical read " << domain->getDomainID()<< " at time  "<<domain->getTime()<<" spread "<<  maxX - minX <<" loc "<<  minY <<":"<<maxY <<" lim  "<<  limInf<<":"<<limSup <<endl; 
								haloReady = true;
						}
					}
			}else{
				if((maxY - minY ) > chainComSpread){
					if ((minX > newlimSup) || (maxX < newlimInf)) {
								newlimSup = std::max(maxX,newlimSup);
								newlimInf = std::min(minX,newlimInf);
								//cout<<"Horizontal read " << domain->getDomainID()<< " at time  "<<domain->getTime()<<" spread "<<  maxY - minY <<" loc "<<  minX <<":"<<maxX <<" lim  "<<  limInf<<":"<<limSup <<endl; 
								haloReady = true;
						}
					}
			}

 	nodeDataList.push_back(new FireNodeData(0, 0, 0, 0, 0, endCom));
	if ((haloReady)&&(countNode>0)){		
	
			if ( this == domain->southOuterHalo ) cout<<domain->getDomainID()<<" read "<<countNode<<" in southOuterHalo"<<endl;
			if ( this == domain->southInnerHalo ) cout<<domain->getDomainID()<<" read "<<countNode<<" in southInnerHalo"<<endl;
			if ( this == domain->westOuterHalo )  cout<<domain->getDomainID()<<" read "<<countNode<<" in westOuterHalo"<<endl;
			if ( this == domain->westInnerHalo ) cout<<domain->getDomainID()<<" read "<<countNode<<" in westInnerHalo"<<endl;
			if ( this == domain->northOuterHalo )  cout<<domain->getDomainID()<<" read "<<countNode<<" in northOuterHalo"<<endl;
			if ( this == domain->northInnerHalo ) cout<<domain->getDomainID()<<" read "<<countNode<<" in northInnerHalo"<<endl;
			if ( this == domain->eastOuterHalo )  cout<<domain->getDomainID()<<" read "<<countNode<<" in eastOuterHalo"<<endl;
			if ( this == domain->eastInnerHalo ) cout<<domain->getDomainID()<<" read "<<countNode<<" in eastInnerHalo"<<endl;
 
				this->setLimits(newlimInf,newlimSup);
				if ( this == domain->southInnerHalo ) domain->southOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->westInnerHalo ) domain->westOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->northInnerHalo ) domain->northOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->eastInnerHalo ) domain->eastOuterHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->southOuterHalo ) domain->southInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->westOuterHalo ) domain->westInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->northOuterHalo ) domain->northInnerHalo->setLimits(newlimInf,newlimSup);
				if ( this == domain->eastOuterHalo ) domain->eastInnerHalo->setLimits(newlimInf,newlimSup);
		 
		
	}else{
		nodeDataList.clear();
	}
	

}

FireNode* Halo::getFireNodebyID(const double& sid){
	long lsid = domain->getIDfromDouble(sid);
	list<Halo*>::iterator halo;
	list<FDCell*>::iterator cell;
	FireNode* tmpfn = 0;
	for ( cell = cells.begin(); cell != cells.end(); ++cell ){
		tmpfn = (*cell)->getFirenodeByID(lsid);
		if ( tmpfn ) return tmpfn;
	}
	return 0;
}

void Halo::initializePositionInMatrix(){
	iCurrent = iComStart;
	jCurrent = jComStart;
	kCurrent = 0;
}

void Halo::getNewPositionInMatrix(size_t& i, size_t& j, size_t& k){

	i = iCurrent;
	j = jCurrent;
	k = kCurrent;

	/* incrementing for next call */
	iCurrent += iComIncrement;
	jCurrent += jComIncrement;
	if ( iComIncrement == 1 and iCurrent > iComEnd ){
		kCurrent++;
		iCurrent = iComStart;
	}
	if ( jComIncrement == 1 and jCurrent > jComEnd ){
		kCurrent++;
		jCurrent = jComStart;
	}
	if(kCurrent> 30){
		cout<<domain->getDomainID()<<"large K in Halo to FireNode data - will create com problems!!!"<<kCurrent<<endl;
		kCurrent = 0;
	}
}

bool Halo::isValid(list<FireNodeData*>& chain){
	/* asserting that the given chain is valid
	 * to be transmitted to other processors */

	/* If it is initialization, everything is valid */
	if ( init ) return true;
 //if ( isActive) return true;
	// Booleans to assert valid state
	bool crossingInnerHalo = false;

	// Iteration variables on the chain
	list<FireNodeData*>::iterator cdata;
	list<FireNodeData*>::iterator next = chain.begin();
	++next;
	list<FireNodeData*>::iterator last = chain.end();
	--last;
	string slink = "link";
	const char* link = slink.c_str();

	/* Scanning the chain for possible defects */
	for ( cdata = chain.begin(); cdata != last; ++cdata ){

		/* Checking that the chain crosses the inner halo */
		if ( !crossingInnerHalo ){
			if ( domain->getDomainID((*cdata)->id) == domain->getDomainID() )
				crossingInnerHalo = true;
			if ( domain->getDomainID((*next)->id) == domain->getDomainID() )
				crossingInnerHalo = true;
			FFPoint inter = domain->findIntersectionWithInnerFrontiers(*cdata, *next);
			if ( inter.getX() != numeric_limits<double>::infinity() ) crossingInnerHalo = true;
		}

		/* Checking the distance between the markers */
		if ( (*cdata)->distance(*next) > 4.*domain->getPerimeterResolution()
				and (*cdata)->state != link and (*next)->state != link ){
			if ( outputs ){
				cout<<domain->getDomainID()
					<<": Not taking into account the preceding chain as "<<(*cdata)->toString()
					<<endl<<'\t'<<"and "<<(*next)->toString()<<" are too far away "
					<<"("<< (*cdata)->distance(*next)<<" vs "
					<<4.*domain->getPerimeterResolution()<<")"<<endl;
				list<FireNodeData*>::iterator dat;
				for ( dat = chain.begin(); dat != chain.end(); ++dat ){
					cout<<domain->getDomainID()<<":"<<'\t'<<(*dat)->toString()<<endl;;
				}
			}
			return false;
		}

		++next;
	}

	if ( !crossingInnerHalo ){
		if ( outputs ){
			cout<<domain->getDomainID()
					<<": Not taking into account the preceding chain as "
					<<"no intersection with the inner halo was found"<<endl;
		}
		return false;
	}

	/* If no defect is found, the chain is considered valid */
	return true;
}


string Halo::getName(){
	return name;
}

}
