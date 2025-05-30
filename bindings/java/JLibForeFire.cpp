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

This code is a JNI interface to the ForeFire solver. It allows to execute commands and get data from the solver.
It has not been updated since version 1, and is not maintained. It is provided as is, and is not guaranteed to work with the latest version of ForeFire.

*/

#ifdef _JNI_IMPLEMENTATION
#include <stdio.h>

#include "JLibForeFire.h"
#include "../../src/Command.h"
#include <string.h>

using namespace std;
using namespace libforefire;

Command* executor = 0;
/*
 * Class:     fprop_coupling_CCoupling
 * Method:    executeCommand
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */

JNIEXPORT jstring JNICALL Java_fprop_coupling_CCoupling_executeCommand  (JNIEnv *penv, jclass jc, jstring jstr)
{
 	const char * msg = penv->GetStringUTFChars(jstr,0);

	if ( executor == 0 ) {
		executor = new Command();
	}

	ostringstream stringOut;
	executor->setOstringstream(&stringOut);


	string smsg(msg);
	executor->ExecuteCommand(smsg);

	penv->ReleaseStringUTFChars(jstr,msg);
	const string& tmpString(stringOut.str());
	const char* const tmpchar = tmpString.c_str();
	return penv->NewStringUTF(tmpchar);

}
/*
 * Class:     fprop_coupling_CCoupling
 * Method:    getMatrix
 * Signature: (Ljava/lang/String;)[[F
 */
JNIEXPORT jobjectArray JNICALL Java_fprop_coupling_CCoupling_getMatrix  (JNIEnv *penv, jclass jc, jstring jstr)
{
	int i,j,k;
	jboolean isCopy;


	const char * msg = penv->GetStringUTFChars(jstr,0);
	string smsg(msg);

	if ( executor == 0 ) {
		executor = new Command();
	}

	/* Testing to see if the wanted matrix is a flux */
	FluxLayer<double>* myFluxLayer = executor->getDomain()->getFluxLayer(msg);
	if ( myFluxLayer ){

		//executor->getDomain()->getDataBroker()->computeActiveSurfacesFlux(40000);
		FFArray<double>* srcD;
		// getting the pointer
		myFluxLayer->getMatrix(&srcD, executor->getTime());
//		myFluxLayer->getInstantaneousFlux(&srcD, executor->getTime());

		if (srcD == 0) return NULL;
		int nx =   srcD->getDim("x");
		int ny =   srcD->getDim("y");
		int nz =   srcD->getDim("z");

		jdoubleArray elemProto = penv->NewDoubleArray(nz);
		jdoubleArray elem = penv->NewDoubleArray(nz);
		jobjectArray row = penv->NewObjectArray(ny,penv->GetObjectClass(elem), NULL);

		jobjectArray resultDD = penv->NewObjectArray(nx,penv->GetObjectClass(row), NULL);

		penv->DeleteLocalRef(row);
		row = NULL;
		penv->DeleteLocalRef(elem);
		elem = NULL;

		for (i = 0; i < nx; i++) {
			jobjectArray row = penv->NewObjectArray(ny,penv->GetObjectClass(elemProto), NULL);

			for (j = 0; j < ny; j++) {
				elem = penv->NewDoubleArray(nz);
				jdouble* elemElems = penv->GetDoubleArrayElements(elem, &isCopy);
				for (k = 0; k < nz; k++) {
					elemElems[k] =  (*srcD)(i,j,k);
				}
				penv->SetDoubleArrayRegion( elem, 0, nz, elemElems);
				penv->SetObjectArrayElement(row, j, elem);
				if (isCopy == JNI_TRUE) {
					penv->ReleaseDoubleArrayElements( elem, elemElems, JNI_ABORT);
				}

				penv->DeleteLocalRef(elem);
				elem = NULL;
			}
			penv->SetObjectArrayElement(resultDD, i, row);



			penv->DeleteLocalRef(row);
			row = NULL;
		}

		penv->DeleteLocalRef(elemProto);
		elemProto = NULL;

		return resultDD;

	} else {
		/* If not, treating it as a generic data layer */
		DataLayer<double>* myLayer = executor->getDomain()->getDataLayer(msg);
		if ( myLayer ){
			FFArray<double>* srcD;
			// getting the pointer
			myLayer->getMatrix(&srcD, executor->getTime());

			if (srcD == 0) return NULL;
			int nx =   srcD->getDim("x");
			int ny =   srcD->getDim("y");
			int nz =  1;// srcD->getDim("z");

			jdoubleArray elemProto = penv->NewDoubleArray(nz);
			jdoubleArray elem = penv->NewDoubleArray(nz);
			jobjectArray row = penv->NewObjectArray(ny,penv->GetObjectClass(elem), NULL);

			jobjectArray resultDD = penv->NewObjectArray(nx,penv->GetObjectClass(row), NULL);

			penv->DeleteLocalRef(row);
			row = NULL;
			penv->DeleteLocalRef(elem);
			elem = NULL;

			double dx = (executor->getDomain()->getNECorner().getX() - executor->getDomain()->getSWCorner().getX())/nx;
			double dy = (executor->getDomain()->getNECorner().getY() - executor->getDomain()->getSWCorner().getY())/ny;

			FFPoint p = FFPoint(executor->getDomain()->getSWCorner().getX(),executor->getDomain()->getSWCorner().getY(),0);


			for (i = 0; i < nx; i++) {
				jobjectArray row = penv->NewObjectArray(ny,penv->GetObjectClass(elemProto), NULL);

				for (j = 0; j < ny; j++) {
					elem = penv->NewDoubleArray(nz);
					jdouble* elemElems = penv->GetDoubleArrayElements(elem, &isCopy);
					for (k = 0; k < nz; k++) {
						elemElems[k] =  myLayer->getValueAt(p,0);//(*srcD)(i,j,k);

					}
					penv->SetDoubleArrayRegion( elem, 0, nz, elemElems);
					penv->SetObjectArrayElement(row, j, elem);
					if (isCopy == JNI_TRUE) {
						penv->ReleaseDoubleArrayElements( elem, elemElems, JNI_ABORT);
					}
					p.y += dy;
					penv->DeleteLocalRef(elem);
					elem = NULL;
				}
				penv->SetObjectArrayElement(resultDD, i, row);
				p.x += dx;
				p.y = executor->getDomain()->getSWCorner().getY();

				penv->DeleteLocalRef(row);
				row = NULL;
			}

			penv->DeleteLocalRef(elemProto);
			elemProto = NULL;

			return resultDD;

		}
		return NULL;
	}
}


/*
* Class:     fprop_coupling_CCoupling
* Method:    setMatrix
* Signature: (Ljava/lang/String;[[F)V
*/
JNIEXPORT void JNICALL Java_fprop_coupling_CCoupling_setMatrix
	(JNIEnv *penv, jobject caller, jstring key, jobjectArray values){


	}



#endif
