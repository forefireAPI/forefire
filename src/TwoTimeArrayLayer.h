/**
 * @file TwoTimeArrayLayer.h
 * @brief TODO: add a brief description.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef TWOTIMEARRAYLAYER_H_
#define TWOTIMEARRAYLAYER_H_

#include "DataLayer.h"
#include "FFArrays.h"
#include "FireNode.h"
#include "SimulationParameters.h"
#include "include/Futils.h"

#include <sys/stat.h>
using namespace std;

namespace libforefire {

/*! \class TwoTimeArrayLayer
 * \brief Template data layer object with FFArray and history
 *
 *  TwoTimeArrayLayer defines a template data layer which data
 *  is available by the means of a FFArray, and remembers last
 *  value of the array
 */
template<typename T> class TwoTimeArrayLayer : public DataLayer<T> {

	FFPoint origin; /*!< coordinates of the origin */

	size_t nx; /*!< size of the array in the X direction */
	size_t ny; /*!< size of the array in the Y direction */
	size_t size; /*!< size of the array */

	double dx; /*!< resolution of the array in the X direction */
	double dy; /*!< resolution of the array in the Y direction */

	FFArray<T>* arrayt1; /*!< pointer to the FFArray containing at one time */
	FFArray<T>* arrayt2; /*!< pointer to the FFArray containing at the other time */
	double time1; /*!< current time of the data */
	double time2; /*!< other time of the data */

	FFArray<T>* tmpMatrix; /*! temporary matrix the size of mnh grid */

	SimulationParameters* params;

	

	/*! \brief extending the matrix of mnh size to an extended one */
	void copyDomainInformation(FFArray<T>*, FFArray<T>*);


	/*! \brief checking to see if indices are within bounds */
	bool inBound(const size_t&, const size_t& = 0);

	/*! \brief interpolation method: lowest order */
	FFPoint posToIndices(FFPoint);

	/*! \brief interpolation method: lowest order */
	T getNearestData(FFPoint, const double&);

	/*! \brief interpolation method: bilinear */
	T bilinearInterp(FFPoint, const double&);

public:
	/*! \brief Default constructor */
	TwoTimeArrayLayer(){};
	/*! \brief Constructor with all necessary information */
	TwoTimeArrayLayer(string name
			, FFArray<T>* matrix1, const double& t1
			, FFArray<T>* matrix2, const double& t2
			, FFPoint& swc, const double& ddx, const double& ddy)
	: DataLayer<T>(name), origin(swc), dx(ddx), dy(ddy)
	  , arrayt1(matrix1), arrayt2(matrix2)
	  , time1(t1), time2(t2) {
		nx = arrayt1->getDim("x");
		ny = arrayt1->getDim("y");
		size = arrayt1->getSize();
		
		tmpMatrix = new FFArray<T>("coreMatrix", 0., nx-2, ny-2);
	};
	/*! \brief Destructor */
	virtual ~TwoTimeArrayLayer(){
		delete arrayt1;
		delete arrayt2;
		delete tmpMatrix;
	}

	/*! \brief obtains the value at a given position in the array */
	T getValt1(size_t = 0, size_t = 0);

	/*! \brief obtains the value at a given position in the old array */
	T getValt2(size_t = 0, size_t = 0);

	/*! \brief computes the value at a given firenode */
	T getValueAt(FireNode*);
	/*! \brief computes the value at a given location and time */
	T getValueAt(FFPoint, const double&);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FireNode*, PropagationModel*, size_t);
	/*! \brief directly stores the desired values in a given array */
	size_t getValuesAt(FFPoint, const double&, FluxModel*, size_t);
	void setValueAt(FFPoint loc,  double tval, T value){};
	/*! \brief getter to the pointer on the FFArray */
	void getMatrix(FFArray<T>**, const double&);
	/*! \brief stores data from a fortran array into the FFArray */
	void setMatrix(string&, double*, const size_t&, size_t&, const double&);
	void loadMultiWindBin(string , double, size_t , size_t* , size_t* );
	/*! \brief print the related FFArray */
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);
			
	double getDx(){ return dx; };
	double getDy(){ return dy; };
	double getDz(){ return 0; };
	double getOriginX(	){ return origin.getX() ; };
	double getOriginY(){ return origin.getY(); };
	double getOriginZ(){ return origin.getZ(); };
	double getWidth(){ return dx*nx; };
	double getHeight(){ return dy*ny; };
	double getDepth(){ return 0 ; };

};

template<typename T>
T TwoTimeArrayLayer<T>::getValt1(size_t i, size_t j){
	return (*arrayt1)(i, j);
}

template<typename T>
T TwoTimeArrayLayer<T>::getValt2(size_t i, size_t j){
	return (*arrayt2)(i, j);
}

template<typename T>
T TwoTimeArrayLayer<T>::getValueAt(FireNode* fn){
	return bilinearInterp(fn->getLoc(), fn->getTime());
}

template<typename T>
T TwoTimeArrayLayer<T>::getValueAt(FFPoint loc, const double& time){
	return bilinearInterp(loc, time);
}

template<typename T>
size_t TwoTimeArrayLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	return 0;
}

template<typename T>
size_t TwoTimeArrayLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	return 0;
}

template<typename T>
void TwoTimeArrayLayer<T>::copyDomainInformation(
		FFArray<T>* inMatrix, FFArray<T>* exMatrix){
	size_t nnx = inMatrix->getDim("x");
	size_t nny = inMatrix->getDim("y");
	/* copying the data from the inner matrix */
	for ( size_t i = 0; i < nnx; i++ ){
		for ( size_t j = 0; j < nny; j++ ){
			(*exMatrix)(i+1,j+1) = (*inMatrix)(i,j);
		}
	}
	/* data in outer cells zero for now */
	for ( size_t i = 1; i < nnx+1; i++ ){
		(*exMatrix)(i,0) = 0.;
		(*exMatrix)(i,nny+1) = 0.;
	}
	for ( size_t j = 0; j < nny+2; j++ ){
		(*exMatrix)(0,j) = 0.;
		(*exMatrix)(nnx+1,j) = 0.;
	}
}


template<typename T>
bool TwoTimeArrayLayer<T>::inBound(const size_t& ii, const size_t& jj){
	return (ii >= 0) && (ii < nx)
			&& (jj >= 0) && (jj < ny);
}

template<typename T>
FFPoint TwoTimeArrayLayer<T>::posToIndices(FFPoint loc){
	return FFPoint((loc.getX()-origin.getX())/dx
			, (loc.getY()-origin.getY())/dy,0);
}

template<typename T>
T TwoTimeArrayLayer<T>::getNearestData(FFPoint loc, const double& time){
	size_t i = (size_t) (loc.getX()-origin.getX())/dx;
	size_t j = (size_t) (loc.getY()-origin.getY())/dy;
	double at = 0.;
	if ( time1 != time2 ) at = ( time - time1 )/(time2 - time1);
	return (1.-at)*getValt1(i,j) + at*getValt2(i,j);
}

template<typename T>
T TwoTimeArrayLayer<T>::bilinearInterp(FFPoint loc, const double& time){
	/* This method implements a bilinear interpolation in space
	 * and linear interpolation in time */

	T val1 = 0.;
	T val2 = 0.;

	/* searching the coordinates of the nodes around */
	FFPoint indices = posToIndices(loc);

	double ud = indices.getX() + EPSILONX;
	double vd = indices.getY() + EPSILONX;

	if ( ud < 3 ) return 0.;
	if ( ud > (int) nx - 3 ) return 0.;
	if ( vd < 3 ) return 0.;
	if ( vd > (int) ny - 3 ) return 0.;

	int uu = (int) ceil(ud-1);
	int vv = (int) ceil(vd-1);



	if ( uu < 0 ) uu = 0;
	if ( uu > (int) nx - 2 ) uu = (int) nx - 2;
	if ( vv < 0 ) vv = 0;
	if ( vv > (int) ny - 2 ) vv = (int) ny - 2;

	double udif = ud - uu;
	double vdif = vd - vv;

	double csw = (1.-udif) * (1.-vdif);
	double cse = udif * (1 - vdif);
	double cnw = (1 - udif) * vdif;
	double cne = udif * vdif;

	double tsw1 = getValt1(uu,vv);
	double tnw1 = getValt1(uu,vv+1);
	double tne1 = getValt1(uu+1,vv+1);
	double tse1 = getValt1(uu+1,vv);

	double tsw2 = getValt2(uu,vv);
	double tnw2 = getValt2(uu,vv+1);
	double tne2 = getValt2(uu+1,vv+1);
	double tse2 = getValt2(uu+1,vv);

	val1 = csw*tsw1 + cse*tse1 + cnw*tnw1 + cne*tne1;
	val2 = csw*tsw2 + cse*tse2 + cnw*tnw2 + cne*tne2;

	/* interpolation in time */
	if ( time1 != time2 ){
		double at = ( time2 - time )/(time2 - time1);
		return at*val1 + (1.-at)*val2;
	} else {
		return val2;
	}

}

template<typename T>
void TwoTimeArrayLayer<T>::getMatrix(FFArray<T>** matrix, const double& time){
	*matrix = arrayt2;
}

template<typename T> void TwoTimeArrayLayer<T>::loadMultiWindBin(string filePattern,double refTime,size_t numberOfDomains, size_t* startI, size_t* startJ){

	FFArray<double>* tmpArray = arrayt1;
  struct stat buffer;  

	for (size_t i = 0; i < numberOfDomains; ++i) {
		string domInName(filePattern+to_string(i+1)+"."+this->getKey());	 
        if(stat(domInName.c_str(), &buffer) == 0){
			ifstream FileIn(domInName.c_str(), ios_base::binary);
			
			tmpArray->loadBinAtLoc(FileIn,startI[i],startJ[i],buffer.st_size);
			FileIn.close();
		}
    }
	time1= time2;
	arrayt2 = tmpArray;
	time2 = refTime;
}

template<typename T>
void TwoTimeArrayLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& newTime){
				
	if ( tmpMatrix->getSize() == sizein ){
			/* Information concerning the whole domain */
			// pointing the current array to the future one
			FFArray<double>* tmpArray = arrayt1;
			arrayt1 = arrayt2;
			time1= time2;
			// pointing the future array to other memory
			arrayt2 = tmpArray;
			time2 = newTime;
			// copying data from atmospheric matrix
			tmpMatrix->copyDataFromFortran(inMatrix);
			copyDomainInformation(tmpMatrix, arrayt2);

	} else {
		//cout<<"Error while trying to retrieve data for two times array data layer "	<<this->getKey()<<", matrix size "<< tmpMatrix->getSize() <<" not matching "<<sizein  <<endl;
	}
}

template<typename T>
string TwoTimeArrayLayer<T>::print(){
	return arrayt2->print2D();
}


template<typename T>
void TwoTimeArrayLayer<T>::dumpAsBinary(string filename, const double& t
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){


/*   If outputs needs to be sync with outputs (no live debug)
    int timeInMillis =  (int)(t*1000);
	int snapLength = (int)(params->getDouble("outputsUpdate")*1000);

	if (timeInMillis%snapLength != 0)
		return;

	int tlocal = (int) t;
	*/

/*	T vals[nnx*nny];

	double ddx = (NEC.getX()-SWC.getX())/(nnx-1);
	double ddy = (NEC.getY()-SWC.getY())/(nny-1);

	FFPoint loc;
	loc.setX(SWC.getX());
	for ( size_t i = 0; i < nnx; i++ ) {
		loc.setY(SWC.getY());
		for ( size_t j = 0; j < nny; j++ ) {
			vals[i*nny+j] = getValueAt(loc, t);
			loc.setY(loc.getY()+ddy);
		}
		loc.setX(loc.getX()+ddx);
	}


	loc.setX(SWC.getX()+ddx);
	loc.setY(SWC.getY()+ddy);

	FFPoint loctl;
	loctl.setX(SWC.getX() +(ddx*(nnx-1)) );
	loctl.setY(SWC.getY() +(ddy*(nny-1)) );

	ostringstream outputfile;
	outputfile<<filename<<"."<<this->getKey();

	ofstream FileOut(outputfile.str().c_str(), ios_base::binary);

	FileOut.write(reinterpret_cast<const char*>(&nnx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&vals), sizeof(vals));
	FileOut.close();*/
}

}

#endif /* TWOTIMEARRAYLAYER_H_ */
