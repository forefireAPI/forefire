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

#ifndef PROPAGATIVELAYER_H_
#define PROPAGATIVELAYER_H_

#include "DataLayer.h"
#include "FDCell.h"
#include "FFArrays.h"
#include "DataBroker.h"

using namespace std;

namespace libforefire {

/*! \class PropagativeLayer
 * \brief PropagativeLayer gives access to the desired model of propagation
 *
 *  PropagativeLayer gives access to the desired model of propagation.
 */

template<typename T> class PropagativeLayer : public DataLayer<T> {

	/* Data for the map of the propagation models */

	int* propModelIndexMap; /*!< indexes of the models */

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



	SimulationParameters* params;

	/*! \brief obtains the position in the array for given location and time */
	size_t getPosInMap(FFPoint, const double);

public:
	/*! \brief Default constructor */
	PropagativeLayer() : DataLayer<T>() {};
	/*! \brief Constructor with no prop model map */
	PropagativeLayer(string name, const int& index)
	: DataLayer<T>(name) {

		mapSize = 1;
		propModelIndexMap = new int[mapSize];
		*propModelIndexMap = index;

		params = SimulationParameters::GetInstance();
	}
	/*! \brief Constructor with all necessary information */
	PropagativeLayer(string name, int* map, FFPoint& swc, double& t0
			, FFPoint& extent, double& timespan
			, size_t& nmx, size_t& nmy, size_t& nmz, size_t& nmt)
	: DataLayer<T>(name), mapStartTime(t0)
	  , mapNx(nmx), mapNy(nmy), mapNz(nmz), mapNt(nmt) {

		mapSize = (size_t) mapNx*mapNy*mapNz*mapNt;
		mapSWCornerX = swc.getX();
		mapSWCornerY = swc.getY();
		mapSWCornerZ = swc.getZ();
		mapNECornerX = mapSWCornerX + extent.getX();
		mapNECornerY = mapSWCornerY + extent.getY();
		mapNECornerZ = mapSWCornerZ + extent.getZ();
		mapEndTime = mapStartTime + timespan;
		propModelIndexMap = new int[mapSize];
		for ( size_t i = 0; i<mapSize; i++ ) propModelIndexMap[i] = map[i];

		mapDx = extent.getX()/mapNx;
		mapDy = extent.getY()/mapNy;
		mapDz = extent.getZ()/mapNz;
		mapDt = timespan/mapNt;

		params = SimulationParameters::GetInstance();
	};
	/*! \brief Destructor */
	virtual ~PropagativeLayer(){
		if ( propModelIndexMap != 0 ) delete [] propModelIndexMap;
	}

	/*! \brief gets the desired model index at a given location */
	int getModelIndexAt(FireNode*);
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

	/*! \brief print the related FFArray */
	string print2D(size_t, size_t);
	string print();
	void dumpAsBinary(string, const double&
			, FFPoint&, FFPoint&, size_t&, size_t&);
			
	double getDx(){ return mapDx; };
	double getDy(){ return mapDy; };
	double getDz(){ return mapDz; };
	double getOriginX(	){ return mapSWCornerX; };
	double getOriginY(){ return mapSWCornerY; };
	double getOriginZ(){ return mapSWCornerZ; };
	double getWidth(){ return getDx()*mapNx; };
	double getHeight(){ return getDy()*mapNy;; };
	double getDepth(){ return getDz()*mapNz;; };

};

template<typename T>
size_t PropagativeLayer<T>::getPosInMap(FFPoint loc, const double t){
	int i, j, k , tt;
	mapNx>1 ? i = ((int) (loc.getX()-mapSWCornerX)/mapDx) : i = 0;
	mapNy>1 ? j = ((int) (loc.getY()-mapSWCornerY)/mapDy) : j = 0;
	mapNz>1 ? k = ((int) (loc.getZ()-mapSWCornerZ)/mapDz) : k = 0;
	mapNt>1 ? tt = ((int) (t-mapStartTime)/mapDt) : tt = 0;

	if ( i < 0 or i > (int) mapNx-1 ){
		cout<<loc.getX()<<" is not within the 'x' domain"
				<<" of validity of layer "<<this->getKey()
				<<" ("<<mapSWCornerX<<"->"<<mapNECornerX<<")"<<endl;
		return 0;
	}
	if ( j < 0 or j > (int) mapNy-1 ){
		cout<<loc.getY()<<" is not within the 'y' domain"
				<<" of validity of layer "<<this->getKey()
				<<" ("<<mapSWCornerY<<"->"<<mapNECornerY<<")"<<endl;
		return 0;
	}
	if ( k < 0 or k > (int) mapNz-1 ){
		cout<<loc.getZ()<<" is not within the 'z' domain"
				<<" of validity of layer "<<this->getKey()
				<<" ("<<mapSWCornerZ<<"->"<<mapNECornerZ<<")"<<endl;
		return 0;
	}
	if ( tt < 0 or tt > (int) mapNt-1 ){
		cout<<t<<" is not within the 't' domain"
				<<" of validity of layer "<<this->getKey()
				<<" ("<<mapStartTime<<"->"<<mapEndTime<<")"<<endl;
		return 0;
	}
	return (size_t) i*mapNy*mapNz*mapNt + j*mapNz*mapNt + k*mapNt + tt;
}

template<typename T>
int PropagativeLayer<T>::getModelIndexAt(FireNode* fn){
	if ( mapSize > 1 ) return propModelIndexMap[getPosInMap(fn->getLoc(), fn->getTime())];
	return propModelIndexMap[0];
}

template<typename T>
T PropagativeLayer<T>::getValueAt(FireNode* fn){
	cout<<"WARNING: PropagativeLayer<T>::getValueAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
T PropagativeLayer<T>::getValueAt(FFPoint loc, const double& t){
	cout<<"WARNING: PropagativeLayer<T>::getValueAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
size_t PropagativeLayer<T>::getValuesAt(FireNode* fn
		, PropagationModel* model, size_t curItem){
	cout<<"WARNING: PropagativeLayer<T>::getValuesAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
size_t PropagativeLayer<T>::getValuesAt(FFPoint loc, const double& t
		, FluxModel* model, size_t curItem){
	cout<<"WARNING: PropagativeLayer<T>::getValuesAt() "
			<<"shouldn't have been called"<<endl;
	return 0;
}

template<typename T>
void PropagativeLayer<T>::getMatrix(FFArray<T>** lmatrix, const double& t){




	double res = 1;


		 
			int nnx = (mapNECornerX-mapSWCornerX)/res;
			int nx = 10;
			 
			int nny = (mapNECornerY-mapSWCornerY)/res;
			int ny = 10;//nny;

			cout<<"WARNING: PropagativeLayer<T>::getMatrix() "
					<<nx<<" "<<ny<<"  "<<(mapNECornerX-mapSWCornerX)<<endl;
			if(nny*nnx < 100*100 ){

				nnx = nx;
				nny = ny;
			
			}

			int nnz = 1;
			int nnt = 1;


			double vals[nnx*nny];

			FFPoint loc;
		//	loc.setX(mapSWCornerX+0.5*ddx);

			for ( int i = 0; i < nnx; i++ ) {
			//	loc.setY(mapSWCornerY+0.5*ddy);
				for ( int j = 0; j < nny; j++ ) {
					vals[i*nny+j] = i; //propModelIndexMap[getPosInMap(loc, 0)];
				//	loc.setY(loc.getY()+ddy);
				}
			//	loc.setX(loc.getX()+ddx);
			}
			*lmatrix = new FFArray<double>("RoS", 1, nnx, nny, nnz, nnt);
			(*lmatrix)->setVal(&vals[0]);


}

template<typename T>
void PropagativeLayer<T>::setMatrix(string& mname, double* inMatrix
		, const size_t& sizein, size_t& sizeout, const double& time){
	cout<<"WARNING: PropagativeLayer<T>::setMatrix() "
			<<"shouldn't have been called"<<endl;
}

template<typename T>
string PropagativeLayer<T>::print(){
	cout<<"WARNING: PropagativeLayer<T>::print() "
			<<"shouldn't have been called"<<endl;
	return "";
}

template<typename T>
string PropagativeLayer<T>::print2D(size_t i, size_t j){
	cout<<"WARNING: PropagativeLayer<T>::print2D() "
			<<"shouldn't have been called"<<endl;
	return "";
}

template<typename T>
void PropagativeLayer<T>::dumpAsBinary(string filename, const double& time
		, FFPoint& SWC, FFPoint& NEC, size_t& nnx, size_t& nny){
	cout<<"WARNING: PropagativeLayer<T>::dumpAsBinary() "
			<<"shouldn't have been called"<<endl;
}

} /* namespace libforefire */
#endif /* PROPAGATIVELAYER_H_ */
