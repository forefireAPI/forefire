/**
 * @file FireNode.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef FIRENODE_H_
#define FIRENODE_H_

#include "FFPoint.h"
#include "FFVector.h"
#include "ForeFireAtom.h"
#include "Visitable.h"
#include "include/FFConstants.h"
#include "SimulationParameters.h"
#include "include/Futils.h"

using namespace std;

namespace libforefire{

class FireDomain;
class FireFront;
class FireNodeData;

/*! \class FireNode
 * \brief Object constituting the fire front
 *
 *  FireNode defines the lagrangian tracking particles
 *  for the fire front. Their properties are: the location,
 *  the normal to the front and the rate of spread.
 *  Pointers are setVal  to the nextInFront and previous fire nodes
 *  of the front. The next location of the node is
 *  computed in 'timeAdvance()' function overloaded from
 *  the ForeFireAtom class. The 'update()' function also
 *  overloads the virtual function from the ForeFireAtom
 *  class and setVal the new properties of the FireNode.
 */
class FireNode : public ForeFireAtom, Visitable {

	static const double Pi;
	static SimulationParameters* params;

	FFPoint location; /*!< location of the FireNode */
	FFVector velocity; /*!< velocity of the fire node */
	FFVector normal; /*!< local normal to the fire front at marker location */
	double speed; /*!< rate of spread of the fire front at marker location */
	double frontDepth; /*!< depth of the fire front at marker location */
	double curvature; /*!< curvature of the fire front at marker location */
	FFPoint nextloc; /*!< next location of the FireNode */
	FireDomain* domain; /*!< FireDomain where the FireNode evolves */
	FireFront* front; /*!< front containing the firenode */
	FireNode* nextInFront; /*!< pointer to the next FireNode in the FireFront */
	FireNode* previousInFront; /*!< pointer to the previous FireNode in the FireFront */
	FireNode* mergingNode; /*!< pointer to the firnode to be merged with */

	static const string altitude; /*!< string shortcut for altitude */
	static const string slope; /*!< string shortcut for slope */

	static bool fdepth; /*!< boolean for the computation of the front depth */
	static bool ccurvature; /*!< boolean for the computation of the curvature */

	static double smoothing; /*!< spatial smoothing for the velocity */
	static double relax; /*!< relaxation for the velocity */

	static double minSpeed; /*!< minimum speed allowed */
	static double minFrontDepth;

public:

	static bool outputs; /*! boolean for outputs */

	/*!  \brief state of the firenode */
	enum State {
		init = 0,
		moving = 1,
		merging = 2,
		splitting = 3,
		final = 4,
		link = 5
	} ;

	State currentState; /*!< current state of the firenode */

	// Definition of the state aliases
	typedef map<string,State> stringToState;
	typedef map<State,string> stateToString;
	/* A map of the strings to their State equivalent, and inverse one */

	static stringToState createStateMap(){
		stringToState m;
		m["init"] = init;
		m["moving"] = moving;
		m["splitting"] = splitting;
		m["merging"] = merging;
		m["final"] = final;
		m["link"] = link;
		return m;
	}
	static stateToString createStringMap(){
		stateToString m;
		m[init] = "init";
		m[moving] = "moving";
		m[splitting] = "splitting";
		m[merging] = "merging";
		m[final] = "final";
		m[link] = "link";
		return m;
	}

	static const stringToState strtost;
	stringToState::const_iterator istrtost;
	static const stateToString sttostr;
	stateToString::const_iterator isttostr;

	/*!  \brief normal scheme */
	enum NormalScheme {
		medians = 0,
		weightedMedians = 1,
		spline = 2
	} ;
	static NormalScheme nmlScheme; /*!< normal scheme */
	static void setNormalScheme(string);

	/*!  \brief curvature scheme */
	enum CurvatureScheme {
		circumradius = 0,
		angle = 1
	} ;
	static CurvatureScheme curvScheme; /*!< curvature scheme */
	static void setCurvatureScheme(string);

	/*!  \brief booleans for the computation of
	 * the local and global interface properties */
	static void setCurvatureComputation(const int&);
	static void setFrontDepthComputation(const int&);

	/*!  \brief smoothing in the speed computation */
	static void setSmoothing(double);
	static void setMinDepth(double);


	/*!  \brief relaxation in the speed computation */
	static void setRelax(double);

	/*!  \brief minimum speed allowed */
	static void setMinSpeed(double);

	/*! \brief Default constructor */
	FireNode(FireDomain* = 0);
	/*! \brief Destructor */
	virtual ~FireNode();

	/*!  \brief Accessors to the location of the FireNode  */
	FFPoint getLoc();
	double getX();
	double getY();
	double getZ();
	/*!  \brief Object initialization */
	void initialize(FFPoint&,  FFVector&, double&, double&, double = 0.
			, FireDomain* = 0, FireFront* = 0, FireNode* = 0);
	void initialize(FireNodeData*, FireDomain*
			, FireFront* = 0, FireNode* = 0);
	/*!  \brief Accessor to the next location of the FireNode  */
	FFPoint getNextLoc();
	/*!  \brief Accessors to the velocity of the FireNode  */
	FFVector getVel();
	double getVx();
	double getVy();
	double getVz();
	/*!  \brief Accessor to the local normal to the front  */
	FFVector getNormal();
	/*!  \brief Accessor to the rate of spread of the fire front  */
	double getSpeed();
	/*!  \brief Accessor to the fire front  */
	FireFront* getFront();
	/*!  \brief Accessor to the upper fire front  */
	FireFront* getContFront();
	/*!  \brief Accessor to the state of the firenode  */
	State getState();

	/*!  \brief string value of the state  */
	string getStateString(State);

	/*!  \brief Accessor to next FireNode in the fire front  */
	FireNode* getNext();
	/*!  \brief Accessor to previous FireNode in the fire front  */
	FireNode* getPrev();

	/*!  \brief front depth getter */
	double getFrontDepth();
	/*!  \brief curvature getter */
	double getCurvature();

	/*!  \brief Gives the position in the front relative to the current head  */
	size_t getPosInFront();

	/*!  \brief mutator of the state of the firenode  */
	void setState(State);
	/*!  \brief mutator of the pointer to the next FireNode  */
	void setNext(FireNode*);
	/*!  \brief mutator of the pointer to the previous FireNode  */
	void setPrev(FireNode*);
	/*!  \brief Mutator of the location of the FireNode  */
	void setLoc(FFPoint&);
	/*!  \brief Mutator of the next location of the FireNode  */
	void setNextLoc(FFPoint&);
	/*!  \brief Mutator of the velocity of the FireNode  */
	void setVel(FFVector);
	/*!  \brief Mutator of the curvature of the front at the marker's location  */
	void setCurvature(double);
	/*!  \brief Mutator of the merging node  */
	void setMergingNode(FireNode*);

	/*! \brief declaration of the containing 'FireDomain' */
	void setDomain(FireDomain*);

	/*! \brief declaration of the 'FireFront' */
	void setFront(FireFront*);

	/*!  \brief front depth setter */
	void setFrontDepth(const double&);

	/*! \brief trashing the node */
	void makeTrash();

	/*! input function (overloads 'input()' from 'ForeFireAtom') */
	void input();

	/*! updates the 'FireNode' properties
	 *  (overloads 'update()' from 'ForeFireAtom') */
	void update();

	/*! computes the next 'FireNode' properties
	 *  and the time of update.
	 *  overloads 'timeAdvance()' from 'ForeFireAtom') */
	void timeAdvance();

	/*! Output function */
	void output();

	/*! updating the firenode if new information in the halo */
	void haloUpdate(FireNodeData*, FireDomain*);

	/*! Visitor function */
	void accept(Visitor *);

	/*! inserting a firenode relative to an another one */
	void insertBefore(FireNode*);
	void insertAfter(FireNode*);

	/*! erasing topology of a firenode */
	void eraseTopology();

	/*! testing membership of a list */
	bool isInList(const list<FireNode*>&);

	/*! \brief computing the front properties at marker location */
	void computeLocalFrontProperties();

	/*! \brief asserting that local topology is compatible with properties' computation */
	bool assertCompatibleTopology();

	/*! \brief computing the normal at marker location */
	FFVector computeNormal();

	/*! \brief computing the curvature at marker location */
	double computeCurvature();

	/*! \brief split related procedures */
	void setSplitting();
	bool isSplitting();
	bool splitAllowed();

	/*! \brief checking if a firenode can merge with another one */
	void setMerging(FireNode*);
	bool isMerging();
	bool mergeAllowed();

	/*! \brief distance functions to another firenode */
	double distance(FireNode*);
	double distance(FFPoint);
	double distance2D(FireNode*);
	double distance2D(FFPoint);

	double getLowestNearby(double );

	/*! \brief approximated location at a given time */
	FFPoint locAtTime(double);

	string toString();
	string toShort();

};

}

#endif /* FIRENODE_H_ */
