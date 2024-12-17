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

#include "FireNode.h"
#include "Visitor.h"
#include <math.h>

namespace libforefire{

// Static variables initialization
const double FireNode::Pi = 3.141592653589793;
const FireNode::stringToState FireNode::strtost = FireNode::createStateMap();
const FireNode::stateToString FireNode::sttostr = FireNode::createStringMap();
const string FireNode::altitude = "altitude";
const string FireNode::slope = "slope";
SimulationParameters* FireNode::params = SimulationParameters::GetInstance();
bool FireNode::outputs = false;
bool FireNode::fdepth = false;
bool FireNode::ccurvature = false;
FireNode::NormalScheme FireNode::nmlScheme = FireNode::medians;
FireNode::CurvatureScheme FireNode::curvScheme = FireNode::circumradius;
double FireNode::smoothing = 1;
double FireNode::relax = 0.1;
double FireNode::minSpeed = -1;
double FireNode::minFrontDepth = 0.001;

// default constructor
FireNode::FireNode(FireDomain* fd) : ForeFireAtom(0.), location()
, velocity(), normal(), speed(), frontDepth(), curvature() {
	getNewID(fd->getDomainID());
	setState(init);
	nextloc = location;
	front = 0;
}

// destructor
FireNode::~FireNode(){
}

// initialisation
void FireNode::initialize(FFPoint& loc,  FFVector& vel, double& t
		, double& fDepth, double kappa, FireDomain* fd, FireFront* ff
		, FireNode* prevNode){
	domain = fd;
	
	getNewID(fd->getDomainID());
	setTime(t);
	setUpdateTime(t);
	
	setLoc(loc);
	
	domain->addFireNodeInCell(this);
	velocity = vel;
	speed = velocity.norm();
	normal = velocity.normed();
	setState(init);
	nextloc = location;
	setFront(ff);
	
	if ( ff != 0 ) ff->addFireNode(this, prevNode);
	setFrontDepth(fDepth);
	setCurvature(kappa);
	mergingNode = 0;
}

void FireNode::initialize(FireNodeData* node, FireDomain* fd
		, FireFront* ff, FireNode* prevNode){
	domain = fd;
	long lid = getIDfromDouble(node->id);
	setID(lid);
	setTime(node->time);
	setUpdateTime(node->time);
	FFPoint loc = FFPoint(node->posX, node->posY);
	setLoc(loc);
	domain->addFireNodeInCell(this);
	velocity.setVx(node->velX);
	velocity.setVy(node->velY);
	speed = velocity.norm();
	normal = velocity.normed();
	frontDepth = node->fdepth;
	setCurvature(node->curvature);
	if ( node->state == "final" ){
		setState(final);
	} else if ( node->state == "link" ) {
		setState(link);
	} else {
		setState(init);
	}
	nextloc = location;
	setFront(ff);
	if ( ff != 0 ) ff->addFireNode(this,prevNode);
	mergingNode = 0;
}

// input function
void FireNode::input(){
}

// update function
void FireNode::update(){

	switch( currentState ){

	case moving:
	{
		// storing old values for later treatment
		FFPoint oldLoc = location;
		double oldTime = getTime();
		// updating the position of the firenode in the cells
		domain->updateFireNodeInCells(this);

		// updating the position of the firenode
		setTime(getUpdateTime());
		setLoc(nextloc);
		// telling the FireDomain that the node is to be updated
		domain->hasMoved(this,oldLoc,oldTime);

		if ( currentState == moving ){
			// Storing the new state in the burning matrix
			domain->firenodeBurningScan(this);
			// checking the topology around the firenode
			domain->checkTopology(this);
		}
	}
		break;

	case init:
		setState(moving);
		break;

	case merging:
		domain->merge(this, mergingNode);
		if ( currentState != final ) setState(moving);
		break;

	case splitting:
		front->split(this, getTime());
		setState(moving);
		break;

	case final:
		break;

	case link:
		break;

	}

}

// Advance in time function
void FireNode::timeAdvance(){

	if ( currentState == moving ){

		// Space-step of the firenodes
		double ds = domain->getSpatialIncrement();
		double dtMax = domain->getMaxTimeStep();

		if ( getDomainID() == domain->getDomainID() ){
			if( assertCompatibleTopology() ){
				computeLocalFrontProperties();
			} else {
				if ( outputs ){
					cout<<domain->getDomainID()
							<<": PROBLEM, bad configuration for normal computing with:"<<endl;
					getPrev() != 0 ? cout<<'\t'<<getPrev()->toShort() : cout<<'\t'<<getPrev();
					cout<<"->"<<toShort()<<"->";
					getNext() != 0 ? cout<<'\t'<<getNext()->toShort() : cout<<'\t'<<getNext();
					cout<<endl;
				}
			}
			if ( fdepth ) {
				double newFrontDepth = domain->computeFrontDepth(this);
				if ( frontDepth > EPSILONX ) {
					frontDepth = (1.-relax)*frontDepth + relax*newFrontDepth;
				} else {
					frontDepth = newFrontDepth;
				}
			}
			// obtaining the speed from the propagation model
			double localSpeed = domain->getPropagationSpeed(this);
			double newSpeed = localSpeed;
			/*if(newSpeed > minSpeed){
				double prevSpeed, nextSpeed;
				getPrev()->getState() == moving ? prevSpeed = getPrev()->getSpeed() : prevSpeed = 0;
				getNext()->getState() == moving ? nextSpeed = getNext()->getSpeed() : nextSpeed = 0;
				newSpeed = ( prevSpeed + smoothing*localSpeed + nextSpeed )/(smoothing+2.);
				}*/
			if ( speed > EPSILONV ) {
				speed = (1.-relax)*speed + relax*newSpeed;
			} else {
				speed = newSpeed;
			}
			// computing the velocity
			velocity = speed*normal;

		}

		if (( speed > minSpeed )and(!fdepth or(frontDepth > minFrontDepth))){

			double dt = ds/speed;
			if ( dt > dtMax ){
				ds = dtMax*speed;
				dt = dtMax;
			}
			setUpdateTime(getTime()+dt);
			// computing the spatial increment
			FFVector nds = ds*normal;
			// computing the next location of the firenode
			nextloc = location + nds.toPoint();
		} else {
			setState(final);
		}

	/*	if ( domain->isInOuterHalo(nextloc)
				and !domain->isInActiveOuterHalo(nextloc) ){
			setNextLoc(location);
			if (outputs) cout<<domain->getDomainID()
					<<": stopping "<<toShort()
					<<" at limit of a non-active outer halo"<<endl;
			setState(final);
		}*/

	}

	if ( currentState == final ){
		setUpdateTime(numeric_limits<double>::infinity());
	}

	if ( currentState == link ){
		cout<<"WARNING: A link node has been advanced in time !! Its address is "<<this<<endl;
		setUpdateTime(numeric_limits<double>::infinity());
	}

}

/*! Output function */
void FireNode::output(){
}


// Accessors
FireFront* FireNode::getFront(){
	return front;
}
FireFront* FireNode::getContFront(){
	if ( front == 0 ) {
		ostringstream oss;
		oss<<domain->getDomainID()<<": PROBLEM with "<<toShort()
				<<" which doesn't have a firefront"<<endl;
		if ( getPrev() != 0 and  getPrev()->getFront() != 0 ){
			oss<<domain->getDomainID()<<": taking the same front as its previous"<<endl;
			setFront(getPrev()->getFront());
		} else if ( getNext() != 0 and  getNext()->getFront() != 0 ){
				oss<<domain->getDomainID()<<": taking the same front as its previous"<<endl;
				setFront(getNext()->getFront());
		} else {
			oss<<domain->getDomainID()<<": even its neighbors has no front"<<endl;
			return 0;
		}
	}
	return front->getContFront();
}

size_t FireNode::getPosInFront(){
	return getFront()->getPositionInFront(this);
}

string FireNode::getStateString(FireNode::State state){
	isttostr = sttostr.find(state);
	if ( isttostr == sttostr.end() ) {
		cout << "unknown state." << endl;
		return "unknown state";
	} else {
		return isttostr->second;
	}
}

FireNode::State FireNode::getState(){
	return currentState;
}

FireNode* FireNode::getNext(){
	return nextInFront;
}
FireNode* FireNode::getPrev(){
	return previousInFront;
}

FFPoint FireNode::getLoc(){
	return location;
}
double FireNode::getX(){
	return location.getX();
}
double FireNode::getY(){
	return location.getY();
}
double FireNode::getZ(){
	return location.getZ();
}
FFPoint FireNode::getNextLoc(){
	return nextloc;
}
FFVector FireNode::getVel(){
	return velocity;
}
double FireNode::getVx(){
	return velocity.getVx();
}
double FireNode::getVy(){
	return velocity.getVy();
}
double FireNode::getVz(){
	return velocity.getVz();
}
FFVector FireNode::getNormal(){
	return normal;
}
double FireNode::getSpeed(){
	return speed;
}
double FireNode::getFrontDepth(){
	return frontDepth;
}
double FireNode::getCurvature(){
	return curvature;
}

void FireNode::setNormalScheme(string scheme){
	if ( scheme == "medians" or scheme == "Medians" ) nmlScheme = medians;
	if ( scheme == "weightedMedians" ) nmlScheme = weightedMedians;
	if ( scheme == "splines" or scheme == "Splines" ) nmlScheme = spline;
}

void FireNode::setCurvatureScheme(string scheme){
	if ( scheme == "circumradius" ) curvScheme = circumradius;
	if ( scheme == "angle" ) curvScheme = angle;
}

void FireNode::setFrontDepthComputation(const int& cfd){
	fdepth = false;
	if ( cfd != 0 ) fdepth = true;
}

void FireNode::setCurvatureComputation(const int& cc){
	ccurvature = false;
	if ( cc != 0 ) ccurvature = true;
}

void FireNode::setMinDepth(double mdepth){
	minFrontDepth = mdepth;
}
void FireNode::setSmoothing(double smooth){
	smoothing = smooth;
}

void FireNode::setRelax(double alpha){
	relax = alpha;
}

void FireNode::setMinSpeed(double u){
	minSpeed = u;
}

// Mutators
void FireNode::setState(State newState){
	isttostr = sttostr.find(newState);
	currentState = newState;
}
void FireNode::setNext(FireNode* node){
	nextInFront = node;
}
void FireNode::setPrev(FireNode* node){
	previousInFront = node;
}
void FireNode::setLoc(FFPoint& p){
	location.setX(p.getX());
	location.setY(p.getY());
	
	location.setZ(domain->getDataLayer(altitude)->getValueAt(p, getTime()));
	if ( location.getZ() == 0. ) location.setZ(p.getZ());

}
void FireNode::setNextLoc(FFPoint& p){
	nextloc.setX(p.getX());
	nextloc.setY(p.getY());
	nextloc.setZ(domain->getDataLayer(altitude)->getValueAt(p, getUpdateTime()));
}
void FireNode::setVel(FFVector v){
	velocity = v;
	normal = velocity.normed();
	speed = velocity.norm();
}
void FireNode::setCurvature(double kappa){
	curvature = kappa;
}

void FireNode::setDomain(FireDomain* fd){
	domain = fd;
}

void FireNode::setFront(FireFront* ff){
	if ( getFront() != ff ){
		if ( getFront() != 0 ) getFront()->decreaseNumFN();
		if ( ff != 0 ) ff->increaseNumFN();
		front = ff;
	}
//	if ( ff ) domain->firenodeBurningScan(this);
}

void FireNode::setFrontDepth(const double& val){
	frontDepth = val;
}

void FireNode::makeTrash(){
	eraseTopology();
    nextloc.setLoc(0,0,0);
    location.setLoc(0,0,0);
    normal.setVec(0,0,0);
    velocity.setVec(0,0,0);
    speed=0;
    setState(final);
    setTime(numeric_limits<double>::infinity());
	setUpdateTime(numeric_limits<double>::infinity());
}

// updating in the halo
void FireNode::haloUpdate(FireNodeData* fnd, FireDomain* fd){
	domain = fd;
	// handling the timing
	domain->deleteAtomOfSimulation(this);
	setTime(fnd->time);
	setUpdateTime(fnd->time);
	domain->addNewAtomToSimulation(this);
	// handling the location
	domain->removeFireNodeInCell(this);
	FFPoint loc = FFPoint(fnd->posX, fnd->posY);
	setLoc(loc);
	nextloc = location;
	domain->addFireNodeInCell(this);
	// handling the rest
	velocity.setVx(fnd->velX);
	velocity.setVy(fnd->velY);
	velocity.setVz(0.);
	speed = velocity.norm();
	normal = velocity.normed();
	setState(init);
}

// Visitor function
void FireNode::accept(Visitor *v) {
	v->increaseLevel();
	v->visit(this);
	v->decreaseLevel();
}

void FireNode::insertBefore(FireNode* fn){
	if (getPrev() != 0) getPrev()->setNext(fn);
	fn->setNext(this);
	fn->setPrev(getPrev());
	setPrev(fn);
	fn->setFront(getFront());
}

void FireNode::insertAfter(FireNode* fn){
	fn->setNext(getNext());
	fn->setPrev(this);
	if (getNext()!=0) getNext()->setPrev(fn);
	setNext(fn);
	fn->setFront(getFront());
}

void FireNode::eraseTopology(){
	if ( getFront() != 0 ){
		getFront()->decreaseNumFN();
		if ( getPrev() != 0 ){
			getPrev()->setNext(getNext());
		}
		if ( getNext() != 0 ){
			getNext()->setPrev(getPrev());
		}
		if ( this == getFront()->getHead() ) {
			if ( getPrev() != 0 and getPrev() != this ){
				getFront()->setHead(getPrev());
			} else if ( getNext() != 0 and getNext() != this ){
				getFront()->setHead(getNext());
			}
		}
		setFront(0);
	}
	setPrev(0);
	setNext(0);
}

bool FireNode::isInList(const list<FireNode*>& nodeList){
	list<FireNode*>::const_iterator ifn;
	for ( ifn = nodeList.begin(); ifn != nodeList.end(); ++ifn ){
		if ( *ifn == this ) return true;
	}
	return false;
}

bool FireNode::assertCompatibleTopology(){

	// Checking the values of the previous and next markers
	if ( getNext() == 0 or getNext() == this ){
		setState(final);
		return false;
	}
	if ( getPrev() == 0 or getPrev() == this ){
		setState(final);
		return false;
	}
	// Checking the positions of the previous and next markers
	if ( location == getPrev()->locAtTime(getTime()) or
			getNext()->locAtTime(getTime()) == location ){
		setState(final);
		return false;
	}
	// Else, everything fine
	return true;

}

void FireNode::computeLocalFrontProperties(){
	/* Computing the front properties */

	// Computing both normal and curvature by spline interpolation
	if ( nmlScheme == spline ){
		front->splineInterp(this, normal, curvature);
		return;
	}

	// Computing the normal
	normal = computeNormal();

	// Computing the curvature (if needed)
	if ( ccurvature ) curvature = computeCurvature();
}
double FireNode::getLowestNearby(double distanceNearby){
	double lowest = location.getZ();
	FireNode* current = getNext();
	int maxcount = 0;
	double totalDistance=0;
	while(current != NULL && current != this && maxcount<1000 && totalDistance < distanceNearby/2){
		totalDistance+= current->distance(current->getNext());
		if (lowest > current->getLoc().getZ()) lowest = current->getLoc().getZ();
		current = current->getNext();


		maxcount++;
	}

	current = getPrev();
	while(current != NULL && current != this && maxcount<1000 && totalDistance < distanceNearby){
		totalDistance+= current->distance(current->getPrev());
		if (lowest > current->getLoc().getZ()) lowest = current->getLoc().getZ();
		current = current->getPrev();

		maxcount++;
	}

	return location.getZ()>0?lowest/location.getZ():1;
}

FFVector FireNode::computeNormal(){
	FFPoint pl = location - getPrev()->locAtTime(getTime());
	FFPoint pr = getNext()->locAtTime(getTime()) - location;
	FFVector tl = FFVector(pl.getX(),pl.getY());
	FFVector tr = FFVector(pr.getX(),pr.getY());
	FFVector nml;

	if ( nmlScheme == medians ){
		// Medians scheme
		tl.normalize();
		tr.normalize();
		nml = FFVector(-tl.getVy()-tr.getVy()
				, tl.getVx()+tr.getVx());
		nml.normalize();

	} else if ( nmlScheme == weightedMedians ) {
		// weighted medians scheme
		double norml = tl.norm();
		double normr = tr.norm();
		if((norml == 0) or (normr ==0)){
			setState(final);
			return normal;
		}
		double beta = normr/(norml+normr);
		tl.normalize();
		tr.normalize();
		FFVector t = beta*tl + (1.-beta)*tr;
		nml = FFVector(-t.getVy(), t.getVx());
	} else {
		return normal;
	}

	normal = nml;

	/* Taking the slope into account */
	double fnslope = domain->getDataLayer(slope)->getValueAt(this);
	nml.setVz(fnslope);
	nml.normalize();
	return nml;
}

double FireNode::computeCurvature(){

	FFPoint prevPos = previousInFront->locAtTime(getTime());
	FFPoint tl = getLoc()-prevPos;
	FFPoint nextPos = nextInFront->locAtTime(getTime());
	FFPoint tr = nextPos - getLoc();

	if ( curvScheme == circumradius ){
		/* Computing the circumradius of the triangle composed of the
		 * three locations of the marker and its neighbors. The sign
		 * is given by the orientation of the vector product. */
		/* checking that points are not aligned */
		if ( abs((tl.crossProduct(tr)).getZ()) < 1.e-10 ) return 0.;
		/* if not, computing the circumradius */
		double al, bl, ar, br, xc;
		double dx, dy;
		if ( tl.getY() == 0. ){
			/* left median of form x = b, right y = ax + b */
			ar = -tr.getX()/tr.getY();
			br = 0.5*(-ar*(getX()+nextPos.getX())
					+ getY()+nextPos.getY());
			xc = 0.5*(getX()+prevPos.getX());
			dx = nextPos.getX() - xc;
			dy = nextPos.getY() - (ar*xc+br);
		} else if ( tr.getY() == 0. ) {
			/* right median of form x = b, left y = ax + b */
			al = -tl.getX()/tl.getY();
			bl = 0.5*(-al*(getX()+prevPos.getX())
					+ getY()+prevPos.getY());
			xc = 0.5*(getX()+nextPos.getX());
			dx = prevPos.getX() - xc;
			dy = prevPos.getY() - (al*xc+bl);
		} else {
			/* both lines of the form y=a*x+b */
			al = -tl.getX()/tl.getY();
			bl = 0.5*(-al*(getX()+prevPos.getX())
					+ getY()+prevPos.getY());
			ar = -tr.getX()/tr.getY();
			br = 0.5*(-ar*(getX()+nextPos.getX())
					+ getY()+nextPos.getY());
			xc = (bl-br)/(ar-al);
			dx = prevPos.getX() - xc;
			dy = prevPos.getY() - (al*xc+bl);
		}

		double kappa = pow(dx*dx+dy*dy,-0.5);
		if ( tl.getY()*tr.getX()-tl.getX()*tr.getY() < 0 ) return -kappa;
		return kappa;

	} else if ( curvScheme == angle ) {

		/* Computing the angle between segments thanks to Al-Kashi */
		double a = getLoc()      .distance2D(prevPos);
		double b = getLoc().distance2D(nextPos);
		double c = nextPos.distance2D(prevPos);
		double dalpha = Pi - acos((a*a+b*b-c*c)/(2.*a*b));

		/* computing the curvature as kappa = dalpha/ds (parametric representation) */
		double kappa = 2.*dalpha/(a+b);
		if ( tl.getY()*tr.getX()-tl.getX()*tr.getY() < 0 ) return -kappa;
		return kappa;

	}

	return curvature;

}

void FireNode::setSplitting(){
	setState(splitting);
}

bool FireNode::isSplitting(){
	if ( currentState == splitting ) return true;
	return false;
}

bool FireNode::splitAllowed(){
	if ( currentState == init ) return false;
	if ( currentState == merging ) return false;
	if ( currentState == link ) return false;
	return true;
}

void FireNode::setMerging(FireNode* fn){
	setState(merging);
	
	setMergingNode(fn);
}

void FireNode::setMergingNode(FireNode* fn){
	mergingNode = fn;
}

bool FireNode::isMerging(){
	if ( currentState == merging ) return true;
	return false;
}

bool FireNode::mergeAllowed(){
	//if ( getDomainID() != domain->getDomainID() ) return false;
	if ( currentState == moving ) return true;
	if ( currentState == final ) return true;
	return false;
}

double FireNode::distance(FireNode* fn){
	return location.distance(fn->getLoc());
}

double FireNode::distance2D(FireNode* fn){
	return location.distance2D(fn->getLoc());
}

double FireNode::distance(FFPoint p){
	return location.distance(p);
}

double FireNode::distance2D(FFPoint p){
	return location.distance2D(p);
}

FFPoint FireNode::locAtTime(double t){
	if ( currentState == final or currentState == init
			or getTime() == getUpdateTime() ) return location;
	// first order interpolant
	double beta = (t-getTime())/(getUpdateTime()-getTime());
	return (1.-beta)*location + beta*nextloc;
}

string FireNode::toString(){
	ostringstream oss;
	//oss.precision(numeric_limits<double>::digits10);
	long frontId = ( getFront() == 0 ? 0 : getFront()->getShortID() );
	oss << "FireNode[domain="<<getDomainID()<<";id="<<getShortID()
		<<";fdepth="<<frontDepth<<";kappa="<<curvature
		<<";loc=" << location.print()<<";vel="<< velocity.print()<<";t="<<getTime()
		<<";state="<<getStateString(getState())<<";frontId="<<frontId<<"]";
	return  oss.str();
}

string FireNode::toShort(){
	ostringstream oss;
	oss << "FireNode[id="<<getShortID()<<";loc="
			<< location.print()<<";t="<< getTime()<<"]";
	return  oss.str();
}

}
