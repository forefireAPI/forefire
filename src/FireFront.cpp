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

#include "FireFront.h"
#include "Visitor.h"

namespace libforefire{

// Static variables
int FireFront::frontNum = 1;
bool FireFront::outputs = false;

FireFront::FireFront(FireDomain* fd) : ForeFireAtom(0.), domain(fd) {
	getNewID(fd->getDomainID());
	containingFront = 0;
	commonInitialization();
}

FireFront::FireFront(const double& t, FireDomain* fd, FireFront* ff)
: ForeFireAtom(t), domain(fd) {
	getNewID(fd->getDomainID());
	if ( ff ){
		containingFront = ff;
		expanding = not ff->isExpanding();
	} else {
		containingFront = fd->getDomainFront();
		expanding = true;
	}
	containingFront->addInnerFront(this);
	domain->addNewAtomToSimulation(this);
	commonInitialization();
}

FireFront::~FireFront() {
	// deleting the inner fronts
	while( !innerFronts.empty() ){
		FireFront* tmpFront = innerFronts.back();
		delete tmpFront;
		innerFronts.pop_back();
	}
/*
	// deleting the firenodes in the firefront
	if ( headNode != NULL ){
		FireNode* curfn = headNode;
		FireNode* next;
		for ( size_t i = getNumFN()-1; i > 0; i-- ){
			next = curfn->getNext();
			delete curfn;
			if ( next == 0 ) break;
			curfn = next;
		}
	}
*/

	if ( h!=NULL ) delete [] h;
	if ( x!=NULL ) delete [] x;
	if ( y!=NULL ) delete [] y;
	if ( a!=NULL ) delete [] a;
	if ( b!=NULL ) delete [] b;
	if ( c!=NULL ) delete [] c;
	if ( rx!=NULL ) delete [] rx;
	if ( ry!=NULL ) delete [] ry;
	if ( d2x!=NULL ) delete [] d2x;
	if ( d2y!=NULL ) delete [] d2y;
	if ( u!=NULL ) delete [] u;
	if ( z!=NULL ) delete [] z;
	if ( gamma!=NULL ) delete [] gamma;
}

void FireFront::commonInitialization(){
	headNode = 0;
	numFirenodes = 0;
	vertx = 0;
	verty = 0;
	nspl = 0;
	h = 0;
	x = 0;
	y = 0;
	a = 0;
	b = 0;
	c = 0;
	rx = 0;
	ry = 0;
	d2x = 0;
	d2y = 0;
	u = 0;
	z = 0;
	gamma = 0;
	expanding = false;
	vertx = 0;
	verty = 0;
	nvert = 0;
	max_inner_front_nodes_filter = 	SimulationParameters::GetInstance()->getInt("max_inner_front_nodes_filter");
}

FireDomain* FireFront::getDomain(){
	return domain;
}

FireFront* FireFront::getContFront(){
	if ( containingFront == 0 ) {
		ostringstream oss;
		oss<<domain->getDomainID()<<": PROBLEM with "<<toString()
				<<" which doesn't have a  containing firefront"<<endl;
	}
	return containingFront;
}

void FireFront::setContFront(FireFront* ff){
	if ( ff ) {
		if ( containingFront ) containingFront->removeInnerFront(this);
		containingFront = ff;
		containingFront->addInnerFront(this);
		expanding = not ff->isExpanding();
	} else {
		containingFront = 0;
	}
}

FireNode* FireFront::getHead(){
	return headNode;
}

void FireFront::setHead(FireNode* fn){
	headNode = fn;
}

void FireFront::addInnerFront(FireFront* ff){
	innerFronts.push_back(ff);
}

void FireFront::removeInnerFront(FireFront* ff){
	innerFronts.remove(ff);
}

list<FireFront*> FireFront::getInnerFronts(){
	return innerFronts;
}

// input function
void FireFront::input(){
	// nothing to do
}

// Update function
void FireFront::update(){
	setTime(getUpdateTime());
}

// Advance in time function
void FireFront::timeAdvance(){
	setUpdateTime(numeric_limits<double>::infinity());
}

/*! Output function */
void FireFront::output(){
}

void FireFront::initialize(double t, FireFront* ff){
	getNewID(domain->getDomainID());
	setTime(t);
	setUpdateTime(t);
	headNode = 0;
	setContFront(ff);
	expanding = not ff->isExpanding();
	domain->addNewAtomToSimulation(this);
	numFirenodes = 0;
}
bool FireFront::isExpanding(){
	return expanding;
}

void FireFront::increaseNumFN(){
	numFirenodes++;
}

void FireFront::decreaseNumFN(){
	numFirenodes--;
}

size_t FireFront::getNumFN(FireNode* startfn){
	// TODO better getNumFN with counting

	if ( startfn == 0 ){
		return 0;
	} else {
		size_t numFN = 1;
		FireNode* fn = startfn;

		try {
			while ( fn->getNext() != startfn ){
				fn = fn->getNext();
				if ( fn == 0 or numFN > LOOPLIMIT ){
					throw logic_error( "PROBLEM in FireFront::getNumFN" );
				}
				numFN++;
			}
			numFirenodes = numFN;
			return numFN;
		} catch ( const logic_error & e ) {
			cout<<"Domain :"<<getDomainID()<<"error getting numFN from "<<startfn<<","<<startfn->toString()<<endl;
			if(startfn->getLoc().distance(FFPoint(0,0,0))<0.000001) {
				cout<<" last node from nowhere, returning no node in front - likely out of physical domain "<<endl;
				domain->addToTrashFronts(this);
				return 1;
			}

			size_t numfn = 1;
			FireNode* fn = startfn;
			list<FireNode*> alreadyVisitedNodes;
			list<FireNode*>::iterator node;
			while ( fn->getNext() != startfn ){
				if ( fn->getNext() == 0 ){
					cout<<toString()<<endl
							<<getDomainID()<<": At position "<<numfn<<": "<<fn->toString()
							<<" has no next ";
					if ( numfn == 1 ){
						domain->addToTrashFronts(this);
					} else {
						cout<<"throwing topological exception"<<endl;
						throw TopologicalException("", "FireFront::getNumFN()");
					}
				}
				if ( numfn > LOOPLIMIT ){
					cout<<toString()<<endl
							<<getDomainID()<<": infinite loop in the front";
					cout<<"throwing topological exception"<<endl;
					throw TopologicalException("", "FireFront::getNumFN()");
				}
				node = find(alreadyVisitedNodes.begin(), alreadyVisitedNodes.end(), fn);
				if ( node != alreadyVisitedNodes.end() ){
					cout<<toString()<<endl
							<<getDomainID()<<": problem at position "<<numfn
							<<" with an already present firenode: "<<fn->toString()<<endl;
					cout<<getDomainID()<<":"<<'\t'<<"previous is "<<fn->getPrev()->toShort()
							<<", startfn is "<<startfn->toShort();
					cout<<"throwing topological exception"<<endl;
					throw TopologicalException("", "FireFront::getNumFN()");
				} else {
					alreadyVisitedNodes.push_back(fn);
				}
				numfn++;
				fn = fn->getNext();
			}
			return numFirenodes;
		}
	}
}


size_t FireFront::getNumFN(){
    return getNumFN(headNode);
}


size_t FireFront::getTotalNumFN(){
	size_t numFNtot = getNumFN();
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ){
		numFNtot += (*innerFront)->getTotalNumFN();
	}
	return numFNtot;
}

size_t FireFront::getPositionInFront(FireNode* fn){
	int ct = 0;
	FireNode* tmpfn = fn;
	for ( size_t fncount = getNumFN(); fncount > 0; fncount-- ){
		if ( tmpfn == headNode ) return ct;
		tmpfn = tmpfn->getPrev();
		ct++;
	}
	return ct;
}

void FireFront::splineInterp(FireNode* ifn, FFVector& nml, double& kappa){
	/* Performs a parametric spline interpolation of the whole
	 * fire front and computes the related normal and curvature
	 * at given marker's location.
	 * The spline system leads to a cyclic tri-diagonal system.
	 * Its resolution is carried by a Sherman-Morrison formula
	 * as described in Numerical recipes in C++, p. 74. */

	size_t n = getNumFN();
	size_t i;

	if ( n < 3 ) return;

	if ( n != nspl ){
		// reallocating the vectors to fit the size
		nspl = n;
		if ( h!=0 ) delete [] h;
		h =  new double[nspl+2];
		if ( x!=0 ) delete [] x;
		x =  new double[nspl+2];
		if ( y!=0 ) delete [] y;
		y =  new double[nspl+2];
		if ( a!=0 ) delete [] a;
		a =  new double[nspl];
		if ( b!=0 ) delete [] b;
		b =  new double[nspl];
		if ( c!=0 ) delete [] c;
		c =  new double[nspl];
		if ( rx!=0 ) delete [] rx;
		rx =  new double[nspl];
		if ( ry!=0 ) delete [] ry;
		ry =  new double[nspl];
		if ( d2x!=0 ) delete [] d2x;
		d2x = new double[nspl];
		if ( d2y!=0 ) delete [] d2y;
		d2y = new double[nspl];
		if ( u!=0 ) delete [] z;
		u = new double[nspl];
		if ( z!=0 ) delete [] z;
		z = new double[nspl];
		if ( gamma!=0 ) delete [] gamma;
		gamma = new double[nspl];
	}

	/* computing the parametric variable as arc length */
	FireNode* fn = headNode;
	FFPoint p = fn->getPrev()->locAtTime(ifn->getTime());
	x[0] = p.getX();
	y[0] = p.getY();
	h[0] = p.distance2D(fn->locAtTime(ifn->getTime()));
	// filling the vectors necessary for the spline interpolation
	for ( i = 1; i < nspl+1; i++ ){
		p = fn->locAtTime(ifn->getTime());
		h[i] = p.distance2D(fn->getNext()->locAtTime(ifn->getTime()));
		x[i] = p.getX();
		y[i] = p.getY();
		fn = fn->getNext();
	}
	h[nspl+1] = h[1];
	x[nspl+1] = x[1];
	y[nspl+1] = y[1];

	// constructing the vectors representing the spline system
	a[0] = 0.;
	b[0] = 2.*(h[0]+h[1]);
	c[0] = h[1];
	for ( i = 1; i < nspl; i++ ){
		a[i] = h[i];
		b[i] = 2.*(h[i]+h[i+1]);
		c[i] = h[i+1];
	}
	c[nspl-1] = 0.;
	for ( i = 1; i < nspl+1; i++ ){
		rx[i-1] = 6.*((x[i+1]-x[i])/h[i] - (x[i]-x[i-1]/h[i-1]));
		ry[i-1] = 6.*((y[i+1]-y[i])/h[i] - (y[i]-y[i-1]/h[i-1]));
	}
	double alpha = h[nspl];
	double beta = h[nspl];

	/* solving the cyclic tri-diagonal system for x and y
	 * (see Numerical recipes in C++, p. 74.) */
	double gamma = -0.5*b[0];
	b[0] = b[0] - gamma;
	b[nspl-1] = b[nspl-1] - alpha*beta/gamma;

	// solve A*x = rx and A*y = ry
	solveTridiagonalSystem(a, b, c, rx, d2x, nspl);
	solveTridiagonalSystem(a, b, c, ry, d2y, nspl);
	// setup the vector u
	u[0] = gamma;
	for ( i = 1; i < nspl-1; i++ ) u[i] = 0.;
	u[nspl-1] = alpha;
	// solve A*z = u
	solveTridiagonalSystem(a, b, c, u, z, nspl);
	// form v*x/(1+v*z) for both x and y
	double factx = (d2x[0] + h[nspl]*d2x[nspl-1]/gamma)
			/(1.+z[0]+h[nspl]*z[nspl-1]/gamma);
	double facty = (d2y[0] + h[nspl]*d2y[nspl-1]/gamma)
			/(1.+z[0]+h[nspl]*z[nspl-1]/gamma);
	// Getting the solution
	for ( i = 0; i < nspl; i++ ){
		d2x[i] -= factx*z[i];
		d2y[i] -= facty*z[i];
	}

	// Finding the position of the firenode
	size_t pos = getPositionInFront(ifn);

	// Computing the firsts derivatives at the marker's location
	double dx, dy;
	if ( pos < n-1 ){
		dx = (x[pos+2]-x[pos+1])/h[pos+1] - (d2x[pos+1]+2.*d2x[pos])*h[pos+1]/6.;
		dy = (y[pos+2]-y[pos+1])/h[pos+1] - (d2y[pos+1]+2.*d2y[pos])*h[pos+1]/6.;
	} else {
		// pos = n-1, d2x[pos+1] = d2x[0]
		dx = (x[pos+2]-x[pos+1])/h[pos+1] - (d2x[0]+2.*d2x[pos])*h[pos+1]/6.;
		dy = (y[pos+2]-y[pos+1])/h[pos+1] - (d2y[0]+2.*d2y[pos])*h[pos+1]/6.;
	}

	// Computing the normal
	nml = FFVector(-dy, dx);
	nml.normalize();

	// computing the curvature
	kappa = (dx*d2y[pos]-dy*d2x[pos])/pow(dx*dx+dy*dy, 1.5);
}

void FireFront::solveTridiagonalSystem(double* a, double* b
		, double* c, double* r, double* u, size_t& n){
	/* solving a tri-diagonal system defined by vectors
	 * a, b, c and r of size n. The results is stored in u.
	 * (see Numerical recipes in C++, p. 51.) */

	double beta;
	if ( b[0] == 0. ){
		cout<<"PROBLEM: Spline interpolation for "<<toString()
			<<" resulted in an ill-posed linear problem"<<endl;
		return;
	}
	beta = b[0];
	u[0] = r[0]/beta;
	// Decomposition and forward substitution
	for ( size_t j = 1; j < n; j++ ){
		gamma[j] = c[j-1]/beta;
		beta = b[j]-a[j]*gamma[j];
		if ( beta == 0. ){
			cout<<"PROBLEM: Spline interpolation for "<<toString()
				<<" resulted in an ill-posed linear problem"<<endl;
			return;
		}
		u[j] = (r[j]-a[j]*u[j-1])/beta;
	}
	// Backward substitution
	for ( int j = n-2; j >= 0; j-- ){
		u[j] -= gamma[j+1]*u[j+1];
	}
}

int FireFront::getNumInnerFronts(){
	return innerFronts.size();
}

int FireFront::getTotalNumInnerFronts(){
	int numFF = innerFronts.size();
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ) {
		numFF += (*innerFront)->getTotalNumInnerFronts();
	}
	return numFF;
}

void FireFront::addFireNode(FireNode* fn, FireNode* prevNode){
	if ( headNode == 0 ) {
		// this is the head
		setHead(fn);
		fn->setPrev(fn);
		fn->setNext(fn);
	} else {
		if ( prevNode == 0 ){
			headNode->insertBefore(fn);
		} else {
			prevNode->insertAfter(fn);
		}
	}
	increaseNumFN();
}

void FireFront::dropFireNode(FireNode* fn){

	if ( getNumFN() > 2 ) {
		domain->addToTrashNodes(fn);
	} else {
		// the firefront won't have sufficient firenodes
		// deleting all the firenodes associated with it
		FireNode* tmpfn1 = headNode;
		FireNode* tmpfn2 = tmpfn1->getNext();
		domain->addToTrashNodes(tmpfn1);
		domain->addToTrashNodes(tmpfn2);
		headNode = 0;
	}
}

void FireFront::extend(){

	if ( !headNode ) return;

	if ( headNode->getFront() != this ){
		headNode->setFront(this);
		increaseNumFN();
	}

	FireFront* frontToBeTrashed = 0;
	FireNode* curfn = headNode;
	FireFront* oldFront = curfn->getFront();

	for ( int fncount = getNumFN()-1; fncount > 0; fncount-- ){
		curfn = curfn->getNext();
		if ( curfn->getFront() != this ){
			oldFront = curfn->getFront();
			if ( oldFront and oldFront != frontToBeTrashed ){
				// trashing the previous front to be trashed, if not null
				if ( frontToBeTrashed ) {
					frontToBeTrashed->setHead(0);
					domain->addToTrashFronts(frontToBeTrashed);
				}
				// keeping the front in memory for trashing
				frontToBeTrashed = oldFront;
			}
		}
		curfn->setFront(this);
		increaseNumFN();
	}
	if ( frontToBeTrashed ){
		frontToBeTrashed->setHead(0);
		domain->addToTrashFronts(frontToBeTrashed);
	}
}

bool FireFront::contains(FireNode* fn){
	FireNode* curfn = headNode;
	for ( size_t i = getNumFN(); i > 0; i-- ){
		if ( curfn == fn ) return true;
		curfn = curfn->getNext();
	}
	return false;
}

void FireFront::split(FireNode* fna, const double& t){

	/* splitting the firenode in two, this creates a new firenode.
	 * Care is taken to make sure that normal will not be greatly affected:
	 * if normal scheme is median, taking the middle of it and its next.
	 * Otherwise taking the middle of the arc passing through the two
	 * locations and with radius as the mean of the two curvature radius */

	if (outputs) cout<<domain->getDomainID()<<": split between "
			<<fna->toShort()<<" and "<<fna->getNext()->toShort()<<endl;

	/* Common part for all normal schemes */
	FireNode* fnb = fna->getNext();
	FFPoint splitLoc = 0.5*(fna->getLoc() + fnb->locAtTime(t));

	double meanRadius = 0.;
	if ( fna->getCurvature() != 0. and fnb->getCurvature() != 0. ) {
		meanRadius = 0.5*(fna->getCurvature()+fnb->getCurvature())
						/(fna->getCurvature()*fnb->getCurvature());
	} else if ( fna->getCurvature() != 0. ) {
		meanRadius = 1./fna->getCurvature();
	} else if ( fnb->getCurvature() != 0. ) {
		meanRadius = 1./fnb->getCurvature();
	} else {
		meanRadius = 0.;
	}

	FFVector meanVel = 0.5*(fna->getVel()+fnb->getVel());
	double meanDepth = 0.5*(fna->getFrontDepth()+fnb->getFrontDepth());
	double meanCurvature = meanRadius == 0 ? 0. : 1./meanRadius;

	/* If normal scheme is not 'medians', computing a better position */
	if ( fna->nmlScheme != FireNode::medians ){
		double dist = fna->getLoc().distance2D(fnb->locAtTime(t));
		if ( abs(meanRadius) > 0.5*dist ){
			FFPoint tt = fnb->locAtTime(t) - fna->getLoc();
			double a = 1./sqrt(tt.getX()*tt.getX()+tt.getY()*tt.getY());
			FFPoint n = FFPoint(-a*tt.getY(), a*tt.getX(),0.);
			double dx = abs(meanRadius) - sqrt(meanRadius*meanRadius-0.25*dist*dist);
			if ( meanRadius > 0. ){
				splitLoc = splitLoc + dx*n;
			} else {
				splitLoc = splitLoc - dx*n;
			}
		}
	}

	if ( !domain->withinPhysicalDomain(splitLoc) ){
		if (outputs) cout<<domain->getDomainID()
				<<": "<<'\t'<<"split is not physical"
				<<" (location is "<<splitLoc.print()<<")"<<endl;
		/* the split node shouldn't be created */
		//if (fna->getState() == FireNode::splitting ) fna->setState(FireNode::moving);
		return;
	}
	FireNode* newNode = domain->addFireNode(splitLoc, meanVel, t
			, meanDepth, meanCurvature, this, fna);
	if (fna->getState() == FireNode::splitting ) fna->setState(FireNode::moving);
	domain->firenodeBurningScan(newNode);
}

void FireFront::merge(FireNode* fna, FireNode* fnb){

	try {

		// Computing the region to be scanned if needed
		double minX = min(fna->getX(),fnb->getX()) - 2.*domain->getPerimeterResolution();
		double maxX = max(fna->getX(),fnb->getX()) + 2.*domain->getPerimeterResolution();
		double minY = min(fna->getY(),fnb->getY()) - 2.*domain->getPerimeterResolution();
		double maxY = max(fna->getY(),fnb->getY()) + 2.*domain->getPerimeterResolution();
		FFPoint swc = FFPoint(minX, minY,0.);
		FFPoint nec = FFPoint(maxX, maxY,0.);
		double t = fna->getTime();

		

		/* test to see if merging successive nodes */
		if ( fna == fnb->getNext() or fnb == fna->getNext() ){
			if (outputs) cout<<getDomainID()
					<<": merging successive nodes"<<endl;
			if ( fnb->getDomainID() != getDomainID() ){
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<fnb->toString()<<endl;
				domain->addToTrashNodes(fnb);
				fna->setState(FireNode::moving);
			} else {
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<fna->toString()<<endl;
				domain->addToTrashNodes(fna);
				fnb->setState(FireNode::moving);
			}
			// less than 5 nodes total... I need to trash my front, it is too small
			if ( getNumFN() < 5 ){
				if (outputs) cout<<getDomainID()
						<<": not enough nodes left in "<<toString()<<" ("
						<<getNumFN()<<"), trashing it"<<endl;
				FireNode* curfn = headNode;
				FireNode* next;
				for ( int numfn = getNumFN()-1; numfn > 0; numfn-- ){
					next = curfn->getNext();
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
					curfn = next;
				}
				if ( curfn != 0 ){
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
				}
				domain->addToTrashFronts(this);

				// Scanning the region for burning status
				domain->areaBurningScan(swc, nec, t);
			}
			return;
		}

		/* If there is only one node between the two
		 * merging nodes, I just trash this node */
		if ( fna->getPrev() == fnb->getNext() ){
			if (outputs) cout<<getDomainID()
					<<": merging quasi-successive nodes"<<endl;
			if ( outputs ) cout<<"trashing in FireFront::merge : "<<fna->getPrev()->toString()<<endl;
			domain->addToTrashNodes(fna->getPrev());
			if ( getNumFN() < 5 ){
				FireNode* curfn = headNode;
				FireNode* next;
				for ( int numfn = getNumFN()-1; numfn > 0; numfn-- ){
					next = curfn->getNext();
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
					curfn = next;
				}
				if ( curfn != 0 ){
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
				}
				domain->addToTrashFronts(this);
				// Scanning the region for burning status
				domain->areaBurningScan(swc, nec, t);
			}
			return;
		}
		if ( fna->getNext() == fnb->getPrev() ){
			if (outputs) cout<<getDomainID()
					<<": merging quasi-successive nodes"<<endl;
			if ( outputs ) cout<<"trashing in FireFront::merge : "<<fna->getNext()->toString()<<endl;
			domain->addToTrashNodes(fna->getNext());
			if ( getNumFN() < 5 ){
				FireNode* curfn = headNode;
				FireNode* next;
				for ( int numfn = getNumFN()-1; numfn > 0; numfn-- ){
					next = curfn->getNext();
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
					curfn = next;
				}
				if ( curfn != 0 ){
					if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
					domain->addToTrashNodes(curfn);
				}
				domain->addToTrashFronts(this);
				// Scanning the region for burning status
				domain->areaBurningScan(swc, nec, t);
			}
			return;
		}

                /* Two cases must be considered :
                     1 - merging inside of a "positive" (expanding) front
                     2 - merging inside of a "negative" (retracting) front*/
                double frontArea = getArea();
                /* Case 1 */
                if (frontArea > 0){
		/* Otherwise that means that i am merging with myself,
		 * with an inner front.I start by imagine that the
		 * node i'm merging with is in the outer, original front */
		if (outputs) cout<<getDomainID()
				<<": creating an inner front at "<<fna->getTime()<<endl;
		FireNode* pa = fna->getPrev();
		FireNode* b = fnb;
		FireNode* nb = fnb->getNext();
		FireNode* fnC;
		double mergeTime = fna->getUpdateTime();

		pa->setNext(nb);
		nb->setPrev(pa);

		fna->setPrev(fnb);
		fnb->setNext(fna);
		setHead(fnb);

		/* everyone is linked I can trash me now */
		if ( outputs ) cout<<"trashing in FireFront::merge : "<<fna->toString()<<endl;
		domain->addToTrashNodes(fna);
		fnb->setState(FireNode::moving);
                /* /!\ Debug  part /!\ */
                if ( outputs ){
                  cout<<"Debug part : "<<fnb->toString()<<endl;
                  cout<<"Previous : "<<fnb->getPrev()->toString()<<endl;
                  cout<<"Next : "<<fnb->getNext()->toString()<<endl;
                }
                /* /!\ /!\ /!\ /!\ /!\ */

		/* bad luck, the other node was inside.
		 * I decide to exchange the heads and carry on */
		double areaA = getLocalArea(fnb);
		double areaB = getLocalArea(nb);
		if( abs(areaA) < abs(areaB) ){
			if (outputs) cout<<getDomainID()
					<<": inverting the inner and outer fronts"<<endl;
			fnC = b;
			b = pa;
			pa = fnC;
			setHead(b);
		}

		/* If I have not enough nodes left I need to trash them */
		
		if ( getNumFN() < 5 ){
			cout<<getDomainID()
					<<": trashing "<<toString()<<" because of lack of nodes ("
					<<getNumFN()<<" nodes in the front)"<<endl;
			FireNode* curfn = headNode;
			FireNode* next;
			for ( int numfn = getNumFN()-1; numfn > 0; numfn-- ){
				next = curfn->getNext();
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
				domain->addToTrashNodes(curfn);
				curfn = next;
			}
			if ( curfn != 0 ){
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
				domain->addToTrashNodes(curfn);
			}
			domain->addToTrashFronts(this);

			// Scanning the region for burning status
			domain->areaBurningScan(swc, nec, t);
			return;
		}

		/* Otherwise I need to dispatch the nodes in a new inner front */
		if ( outputs ) cout<<getDomainID()
				<<": creating a new inner front"<<endl;
                FireFront* tmpFront = domain->addFireFront(mergeTime,this);
		fnC = pa;
		tmpFront->setHead(fnC);
		for ( size_t i = getNumFN(fnC); i > 0 ; i-- ){
			fnC->setFront(tmpFront);
			fnC = fnC->getNext();
		}
		/* If I have not enough nodes in the inner front I need to trash it */
		if ( tmpFront->getNumFN() < max_inner_front_nodes_filter ){
			if (outputs) cout<<getDomainID()
					<<": trashing inner front "<<tmpFront->toString()<<" because of lack of nodes ("
					<<tmpFront->getNumFN()<<" nodes in the front)"<<endl;
			FireNode* curfn = tmpFront->getHead();
			FireNode* next;
			for ( int numfn = tmpFront->getNumFN()-1; numfn > 0; numfn-- ){
				next = curfn->getNext();
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
				domain->addToTrashNodes(curfn);
				curfn = next;
			}
			if ( curfn != 0 ){
				if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
				domain->addToTrashNodes(curfn);
			}
			domain->addToTrashFronts(tmpFront);
		}
                }
                // Case 2
                else{
                  // Firstly, we do the same as in Case 1. This will split
                  // the polygon in 2 parts.
                  if (outputs) cout<<getDomainID()
                                   <<": creating an inner front at "<<fna->getTime()<<endl;
                  FireNode* pa = fna->getPrev();
                  FireNode* b = fnb;
                  FireNode* nb = fnb->getNext();
                  FireNode* fnC;
                  double mergeTime = fna->getUpdateTime();

                  pa->setNext(nb);
                  nb->setPrev(pa);

                  fna->setPrev(fnb);
                  fnb->setNext(fna);
                  setHead(fnb);

                  /* everyone is linked I can trash me now */
                  if ( outputs ) cout<<"trashing in FireFront::merge : "<<fna->toString()<<endl;
                  domain->addToTrashNodes(fna);
                  fnb->setState(FireNode::moving);

                  /* bad luck, the other node was inside.
                   * I decide to exchange the heads and carry on */
                  double areaA = getLocalArea(fnb);
                  double areaB = getLocalArea(nb);
                  if( abs(areaA) < abs(areaB) ){
                    if (outputs) cout<<getDomainID()
                                     <<": inverting the inner and outer fronts"<<endl;
                    fnC = b;
                    b = pa;
                    pa = fnC;
                    setHead(b);
                  }
                  // One of the 2 parts will belong to another FireFront
                  if ( outputs ) cout<<getDomainID()
                                     <<": creating a new inner front"<<endl;
                  FireFront* tmpFront = domain->addFireFront(mergeTime,this);
                  fnC = pa;
                  tmpFront->setHead(fnC);
                  for ( size_t i = getNumFN(fnC); i > 0 ; i-- ){
                    fnC->setFront(tmpFront);
                    fnC = fnC->getNext();
                  }

                  // Same for the second part
                  if ( tmpFront->getNumFN() < 5 ){
                    if (outputs) cout<<getDomainID()
                                     <<": trashing inner front "<<tmpFront->toString()<<" because of lack of nodes ("
                                     <<tmpFront->getNumFN()<<" nodes in the front)"<<endl;
                    FireNode* curfn = tmpFront->getHead();
                    FireNode* next;
                    for ( int numfn = tmpFront->getNumFN()-1; numfn > 0; numfn-- ){
                      next = curfn->getNext();
                      if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
                      domain->addToTrashNodes(curfn);
                      curfn = next;
                    }
                    if ( curfn != 0 ){
                      if ( outputs ) cout<<"trashing in FireFront::merge : "<<curfn->toString()<<endl;
                      domain->addToTrashNodes(curfn);
                    }
                    domain->addToTrashFronts(tmpFront);
                  }
                }

		// Scanning the region for burning status
		domain->areaBurningScan(swc, nec, t);

	} catch (...) {
		ostringstream oss;
		oss<<toString()<<endl<<getDomainID()<<": problem in merging "
				<<fna->toShort()<<" and "<<fnb->toShort();
		throw TopologicalException(oss.str(), "FireFront::merge()");
	}
}

void FireFront::mergeInnerFronts(FireNode* fna, FireNode* fnb){
	try {
		/* merging two inner firefronts, this should create
		 * a new firefront with all the firenodes */
		// affecting all the firenodes of front b to front a
		if (outputs) cout<<domain->getDomainID()<<": "<<"merging firenode "
				<<fna->toShort()<<" from front "<<fna->getFront()
				<<" ("<<fna->getFront()->getNumFN()<<" firenodes)"<<endl<<"with firenode "
				<<fnb->toShort()<<" with front "<<fnb->getFront()
				<<" ("<<fnb->getFront()->getNumFN()<< " firenodes)"<< endl;

		// Computing the region to be scanned if needed
		double minX = min(fna->getX(),fnb->getX()) - 2.*domain->getPerimeterResolution();
		double maxX = max(fna->getX(),fnb->getX()) + 2.*domain->getPerimeterResolution();
		double minY = min(fna->getY(),fnb->getY()) - 2.*domain->getPerimeterResolution();
		double maxY = max(fna->getY(),fnb->getY()) + 2.*domain->getPerimeterResolution();
		FFPoint swc = FFPoint(minX, minY,0.);
		FFPoint nec = FFPoint(maxX, maxY,0.);
		double t = fna->getTime();

		FireFront* frontb = fnb->getFront();
		FireNode* frontbHead = frontb->getHead();
		frontbHead->setFront(fna->getFront());
		// changing the nexts and prevs
		if ( fna->getPrev() and fnb->getNext() ){
			fna->getPrev()->setNext(fnb->getNext());
			fnb->getNext()->setPrev(fna->getPrev());
		}
		if ( fna->getNext() and fnb->getPrev() ){
			fna->getNext()->setPrev(fnb->getPrev());
			fnb->getPrev()->setNext(fna->getNext());
		}
		if ( fna->getFront()->getHead() == fna ){
			fna->getFront()->setHead(fna->getNext());
		}
		FireFront* tmpFront = fna->getFront();
		tmpFront->extend();
		// adding the merging firenodes to the trash nodes
		fna->setFront(0);
		if (outputs) cout<<domain->getDomainID()
				<<": FireFront::mergeInnerFronts -> ";
		if ( outputs ) cout<<"trashing in FireFront::mergeInnerFronts : "<<fna->toString()<<endl;
		domain->addToTrashNodes(fna);
		fnb->setFront(0);
		if (outputs) cout<<domain->getDomainID()
				<<": FireFront::mergeInnerFronts -> ";
		if ( outputs ) cout<<"trashing in FireFront::mergeInnerFronts : "<<fnb->toString()<<endl;
		domain->addToTrashNodes(fnb);

		// Scanning the region for burning status
		domain->areaBurningScan(swc, nec, t);


	} catch (...) {
		ostringstream oss;
		oss<<toString()<<endl<<getDomainID()<<":  problem in merging inner fronts of "
				<<fna->toShort()<<" and "<<fnb->toShort();
		throw TopologicalException(oss.str(), "FireFront::mergeInnerFronts()");
	}
}

double FireFront::getLocalArea(FireNode* fn){
	FireNode* a = fn;
	FireNode* b = a->getNext();
    if ( b==0 or b==a ) return 0;
    size_t numfn = getNumFN(fn);
	double area = (a->getX()+b->getX()) * (a->getY()-b->getY());
	for ( size_t k = 1; k < numfn; k++ ){
		a=b;
		b=b->getNext();
		area += (a->getX()+b->getX()) * (a->getY()-b->getY());
	}
	if ( !b->getNext() ) area += (b->getX()+fn->getX()) * (b->getY()-fn->getY());
	area*=0.5;
	return area;
}

double FireFront::getArea(){
	return getLocalArea(headNode);
}

bool FireFront::checkForBurningStatus(FFPoint& loc){
	bool burning = false;
	if ( this != domain->getDomainFront() ){
		if ( loc.pointInPolygon(nvert, vertx, verty) ) burning = not burning;
	}
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ){
		if ( (*innerFront)->checkForBurningStatus(loc) ) burning = not burning;
	}
	return burning;
}

// Visitor function
void FireFront::accept(Visitor* v) {
	if ( this != domain->getDomainFront() ) v->increaseLevel();
	if ( headNode != 0 ) {
		v->visit(this);
		FireNode* fntmp = headNode;
		if ( fntmp == 0 ) {
			cout<<getDomainID()
					<<": PROBLEM in FireFront::accept, no headnode found for front"
					<<toString()<<endl;
			return;
		}
		fntmp->accept(v);
		for ( size_t fncount = getNumFN()-1; fncount > 0; fncount-- ){
			if ( fntmp->getFront() != this ){
				cout<<getDomainID()
						<<": PROBLEM with the front of a node in FireFront::accept"<<endl;
				cout<<getDomainID()<<": "<<fntmp->toString()
						<<" has front "<<fntmp->getFront()->toString()<<fntmp->getFront()
						<<" instead of "<<this->toString()<<endl;
				fntmp->setFront(this);
			}
			if (fntmp->getNext() == fntmp){
				cout<<getDomainID()<<": PROBLEM with the next of node "
						<<fntmp->toString()<<" which is itself"<<endl;
			}
			fntmp = fntmp->getNext();
			if ( fntmp == 0 ){
				cout<<getDomainID()
						<<": PROBLEM in FireFront::accept"
						<<", Incomplete front with no next for a firenode in front"
						<<toString()<<endl;
				return;
			}
			fntmp->accept(v);
		}
	}
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ) {
		(*innerFront)->accept(v);
	}
	if ( this != domain->getDomainFront() ) v->decreaseLevel();
}

void FireFront::computeBoundingBox(FFPoint& swc, FFPoint& nec){
	try {

		FireNode* fn = headNode;
		double xmin = fn->getX();
		double xmax = fn->getX();
		double ymin = fn->getY();
		double ymax = fn->getY();
		fn = fn->getNext();
		// Loop on the nodes of the front
		for ( size_t fncount = getNumFN()-1; fncount > 0; fncount-- ){
			if ( fn->getNext() == 0 )
				throw logic_error( "Pb in FireFront::computeBoundingBox" );
			if ( fn->getX() < xmin ) xmin = fn->getX();
			if ( fn->getX() > xmax ) xmax = fn->getX();
			if ( fn->getY() < ymin ) ymin = fn->getY();
			if ( fn->getY() > ymax ) ymax = fn->getY();
			fn = fn->getNext();
		}

		swc.setX(xmin);
		swc.setY(ymin);
		nec.setX(xmax);
		nec.setY(ymax);

	} catch ( const logic_error & e ) {
		FireNode* fn = headNode;
		ostringstream oss;
		int numfn = 1;
		numfn++;
		// Loop on the nodes of the front
		for ( size_t fncount = getNumFN()-1; fncount > 0; fncount-- ){
			if ( fn->getNext() == 0 ) {
				oss<<fn->toShort()<<endl
						<<getDomainID()<<":"<<'\t'<<"at position "<<numfn
						<<" this node has no next";
				throw TopologicalException(oss.str(), "FireFront::computeBoundingBox");
			}
			numfn++;
			fn = fn->getNext();
		}
	} catch (...) {
		cout<<"PROBLEM in FireFront::computeBoundingBox"<<endl;
	}
}

void FireFront::constructVerticesVectors(){
	if ( this != domain->getDomainFront() ){
		nvert = getNumFN();
		vertx = new double[nvert];
		verty = new double[nvert];
		storeVertices(vertx, verty, nvert);
	}
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ){
		(*innerFront)->constructVerticesVectors();
	}
}

void FireFront::storeVertices(double* vx, double* vy, size_t& n){
	FireNode* fn = headNode;
	for ( size_t i = 0; i < n; i++ ){
		vx[i] = fn->getX();
		vy[i] = fn->getY();
		fn = fn->getNext();
	}
}

void FireFront::deleteVerticesVectors(){
	if ( this != domain->getDomainFront() ){
		if ( vertx != 0 ) delete [] vertx;
		if ( verty != 0 ) delete [] verty;
	}
	for ( innerFront = innerFronts.begin();
			innerFront != innerFronts.end(); ++innerFront ){
		(*innerFront)->deleteVerticesVectors();
	}
}

void FireFront::makeTrash(){
	if ( innerFronts.size() != 0 )
		cout<<"WARNING: trashing a fire front with inner fronts"<<endl;
	headNode = 0;
	numFirenodes = 0;
	if ( containingFront != 0 ){
		containingFront->removeInnerFront(this);
		containingFront = 0;
	}
	setUpdateTime(numeric_limits<double>::infinity());
}

string FireFront::toString(){
	ostringstream oss;
	oss << "FireFront[id="<< getShortID()<<";domain="
			<<getDomainID()<<";t="<<getTime()<<"]";
	return  oss.str();
}

string FireFront::print(int level){
	ostringstream oss, tabs;
	for ( int k = 0; k<level; k++ ){
		tabs<<'\t';
	}
	if ( this != domain->getDomainFront() ){
		if ( headNode ){
			oss <<tabs.str()<< toString() << endl;
			FireNode* fntmp = headNode;
			oss<<tabs.str()<<'\t'<<fntmp->toString()<< endl;
			for ( int fncount = getNumFN()-1; fncount > 0; fncount-- ){
				fntmp = fntmp->getNext();
				oss<<tabs.str()<<'\t'<<fntmp->toString()<< endl;
			}
			for ( innerFront = innerFronts.begin();
					innerFront != innerFronts.end(); ++innerFront ){
				oss << (*innerFront)->print(level+1);
			}
		} else {
		}
	} else {
		for ( innerFront = innerFronts.begin();
				innerFront != innerFronts.end(); ++innerFront ){
			oss << (*innerFront)->print();
		}
	}
	return  oss.str();
}

}
