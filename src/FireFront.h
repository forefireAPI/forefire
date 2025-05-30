/**
 * @file FireFront.h
 * @brief Class describing a fire front
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef FIREFRONT_H_
#define FIREFRONT_H_

#include "ForeFireAtom.h"
#include "Visitable.h"
#include "FireNode.h"
#include "ParallelException.h"
#include "include/FFConstants.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire{

class FireDomain;

/*! \class FireFront
 * \brief Class describing a fire front
 *
 *  A FireFront implements a pointer to the so-called
 *  'headNode' and all its 'innerFronts' (also of type
 *  FireFront). Thanks to 'nextInFront' and 'previousInFront' pointers
 *  of the FireNode class all the FireNodes constituting
 *  the FireFront are linked. An iterator is setVal up in
 *  order to go through all the FireNodes of the
 *  FireFront.
 */
class FireFront: public ForeFireAtom, Visitable {

	size_t numFirenodes; /*!< number of firenodes in the front */
	FireNode* headNode; /*!< 'FireNode' of entry for the 'FireFront' */
	FireDomain* domain; /*!< domain containing the fire front */
	bool expanding; /*!< behavior of the fire front (expanding or contracting) */
	FireFront* containingFront; /*!< 'FireFront' containing this one */
	list<FireFront*> innerFronts; /*!< inner fire fronts */
	list<FireFront*>::iterator innerFront;

	/*!  \brief local variables for vertices storage */
	size_t nvert;
	double *vertx, *verty;

	/*!  \brief local variables in case of spline interpolation */
	size_t nspl;
	double *h, *x, *y, *a, *b, *c, *rx, *ry, *d2x, *d2y, *u, *z, *gamma;
    size_t max_inner_front_nodes_filter;
	static int frontNum;

	/*!  \brief common initailization for all constructors */
	void commonInitialization();

	/*!  \brief Gives local area from a given firenode  */
	double getLocalArea(FireNode*);

	/*!  \brief distance from the front */
	double distanceFromFront(const double&, const double&);

public:

	static bool outputs; /*! boolean for outputs */

	/*! \brief Default constructor, to be avoided */
	FireFront(FireDomain* = 0);
	/*! \brief Constructor for a front included
	 * in a 'FireDomain' and a containing front */
	FireFront(const double&, FireDomain*, FireFront* = 0);
	/*! \brief Destructor */
	virtual ~FireFront();

	/*! \brief Accesssor to the domain */
	FireDomain* getDomain();

	/*! \brief Accesssor to the containing firefront */
	FireFront* getContFront();

	/*! \brief Mutator of the containing firefront */
	void setContFront(FireFront*);

	/*! \brief Accesssor to the head FireNode */
	FireNode* getHead();

	/*! \brief Mutator of the head FireNode */
	void setHead(FireNode*);

	/*!< \brief Mutators of the inner fronts */
	void addInnerFront(FireFront*);
	void removeInnerFront(FireFront*);
	list<FireFront*> getInnerFronts();

	/*! input function (overloads 'input()' from 'ForeFireAtom') */
	void input();

	/*! updates the 'FireFront' properties
	 *  (overloads 'update()' from 'ForeFireAtom') */
	void update();

	/*! computes the next 'FireFront' properties
	 *  and the time of update.
	 *  (overloads 'timeAdvance()' from 'ForeFireAtom') */
	void timeAdvance();

	/*! Output function */
	void output();

	/*! \brief Visitor function */
	void accept(Visitor*);

	/*! \brief Initialize function */
	void initialize(double, FireFront*);

	/*! \brief behavior of th firefront */
	bool isExpanding();

	/*! \brief Area of the front */
	double getArea();

	/*! \brief accessor to the number of firenodes in the firefront */
	size_t getNumFN(FireNode*);
	size_t getNumFN();
	size_t getTotalNumFN();

	/*! \brief getter of the position of a firenode in the front */
	size_t getPositionInFront(FireNode*);

	/*! \brief getter of the number of inner fronts */
	int getNumInnerFronts();
	int getTotalNumInnerFronts();

	/*! \brief spline interpolation of the firefront */
	void splineInterp(FireNode*, FFVector&, double&);
	void solveTridiagonalSystem(double*, double*, double*, double*
			, double*, size_t&);

	/*! \brief adding a firenode in the firefront */
	void addFireNode(FireNode*, FireNode* = 0);

	/*! \brief erasing a firenode from the firefront */
	void dropFireNode(FireNode*);

	/*! \brief test to see if a marker belongs to this front */
	bool contains(FireNode*);

	/*! \brief extending the firefront in one direction */
	void extend();

	/*! \brief handling the possible splits in the firefront */
	void split(FireNode*, const double&);

	/*! \brief handling the possible merges in the firefront */
	void merge(FireNode*, FireNode*);
	void mergeInnerFronts(FireNode*, FireNode*);

	/*! \brief constructing arrays storing the coordinates
	 * of the vertices constituting the front */
	void storeVertices(double*, double*, size_t&);
	void constructVerticesVectors();
	void deleteVerticesVectors();

	/*! \brief checking the burning status of a given location */
	bool checkForBurningStatus(FFPoint&);

	/*! \brief computes a rectangle containing the entire front */
	void computeBoundingBox(FFPoint&, FFPoint&);

	/*! \brief managing the number of firenodes in the front */
	void increaseNumFN();
	void decreaseNumFN();

	/*! \brief making the front a trash one */
	void makeTrash();

	string toString();
	string print(int = 0);
};

}

#endif /* FIREFRONT_H_ */
