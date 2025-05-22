/**
 * @file FFArrays.h
 * @brief 4d arrays of type T values for LibForeFire
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#ifndef FFARRAYS_H_
#define FFARRAYS_H_

#include "include/Futils.h"

using namespace std;

namespace libforefire {

/*! \class FFArray
 * \brief 4d arrays of type T values for LibForeFire
 *
 *  FFArray defines 4d arrays and methods associated with it.
 */
template<typename T> class FFArray {
	size_t nx; /*!< dimension along x axis */
	size_t ny; /*!< dimension along y axis */
	size_t nz; /*!< dimension along z axis */
	size_t nt; /*!< dimension along t axis */
	size_t size; /*!< total size of the array */
	T* data; /*!< contained data */
	string name; /*!< name of the array */
public:
	/*! \brief Constructor with value */
	FFArray(string aname, T val = 0., size_t ni = 1
			, size_t nj = 1, size_t nk = 1, size_t nl = 1) :
		nx(ni), ny(nj), nz(nk), nt(nl), name(aname)	{
		size = nx*ny*nz*nt;
			
		try {
			// allocation of enough storage
			data = new T[size];
			for ( size_t i=0; i < size; i++ ) {
				data[i] = val;
			}
		} catch ( const bad_alloc& ) {
			cout << "Problem in the copy of an array with default value: not enough space !!" << endl;
		}
	}
	/*! \brief Constructor with imposed matrix */
	FFArray(string aname, T* matrix, const size_t& ni = 1,const  size_t& nj = 1
			, const size_t& nk = 1, const size_t& nl = 1) :
		nx(ni), ny(nj), nz(nk), nt(nl), name(aname)	{
		size = nx*ny*nz*nt;
		try {
			// allocation of enough storage
			data = new T[size];
			for ( size_t i=0; i < size; i++ ) {
				data[i] = matrix[i];
			}
		} catch ( const bad_alloc& ) {
			cout << "Problem in the copy of an array with default value: not enough space !!" << endl;
		}
	}
	/*! \brief Destructor */
	~FFArray(){
		delete [] data;
	}

	// Setters and Getters
	/*!  \brief accessor of the data contained at position (i,j,k)  */
	T operator() (size_t, size_t = 0, size_t = 0, size_t = 0) const;
	/*!  \brief mutator of the data contained at position (i,j,k)  */
	T& operator() (size_t, size_t = 0, size_t = 0, size_t = 0);
	/*!  \brief mutator of the entire data  */
	void setVal(const double*);
	/*!  \brief accessor to the pointer of the first element  */
	T* getData();
	/*!  \brief accessor to the size of the array  */
	size_t getSize();
	/*!  \brief accessor to the dimensions of the array  */
	size_t getDim(string = "total");
	/*!  \brief resizing the array with a given value */
	void resize(size_t, size_t = 1, size_t = 1, size_t = 1);

	// Interfacing with Fortran arrays
	/*!  \brief copying contained data to a Fortran array  */
	void copyDataToFortran(T*);
	/*!  \brief copying contained data from a Fortran array  */
	void copyDataFromFortran(const T*);
	void loadBin(std::ifstream&  );
	void loadBinAtLoc(std::ifstream&  , size_t , size_t ,size_t);
	void setDataAtLoc(T* , size_t , size_t ,size_t , size_t ,size_t );
	void dumpBin(std::ofstream&  );
	// Printing functions
	string print2D(size_t = 0, size_t = 0);
	/*! \brief Sum all elements in the array */
	T sum() const;
	T min() const;
	T max() const;
};

template<typename T>
T FFArray<T>::operator ()(size_t i, size_t j, size_t k, size_t l) const {
	if ( i > nx or j > ny or k > nz or l > nt ){
		cout<<"WARNING: Array "<<name<<"("<<i<<","<<j<<","<<k<<","<<l
				<<") out of bounds, size "
				<<nx<<":"<<ny<<":"<<nz<<":"<<nt<<endl;
             int im = i>nx?nx-1:i;
             int jm = j>ny?ny-1:j;
             int km = k>nz?nz-1:k;
             int lm = l>nt?nt-1:l;
             im=im<0?0:im;
             jm=jm<0?0:jm;
             km=km<0?0:km;
             lm=lm<0?0:lm;

	     return data[im*ny*nz*nt + jm*nz*nt + km*nt + lm];
	}
	return data[i*ny*nz*nt + j*nz*nt + k*nt + l];
 

}
template<typename T>
T FFArray<T>::sum() const {
	T total = T();
	for (size_t i = 0; i < size; ++i) {
		total += data[i];
	}
	return total;
} 
template<typename T>
T FFArray<T>::min() const {
    if (size == 0) {
        // No elements: return default-constructed T or handle as needed
        return T();
    }
    T m = data[0];
    for (size_t i = 1; i < size; ++i) {
        if (data[i] < m) {
            m = data[i];
        }
    }
    return m;
}
template<typename T>
T FFArray<T>::max() const {
    if (size == 0) {
        // No elements: return default-constructed T or handle as needed
        return T();
    }
    T m = data[0];
    for (size_t i = 1; i < size; ++i) {
        if (data[i] > m) {
            m = data[i];
        }
    }
    return m;
}
template<typename T>
T& FFArray<T>::operator ()(size_t i, size_t j, size_t k, size_t l){
	if ( i >= nx or j >= ny or k >=nz or l >= nt ){
		cout<<"WARNING: Array "<<name<<"("<<i<<","<<j<<","<<k<<","<<l
				<<") out of bounds, size "
				<<nx<<":"<<ny<<":"<<nz<<":"<<nt<<endl;
	}

             int im = i>nx?nx-1:i;
             int jm = j>ny?ny-1:j;
             int km = k>nz?nz-1:k;
             int lm = l>nt?nt-1:l;

             im=im<0?0:im;
             jm=jm<0?0:jm;
             km=km<0?0:km;
             lm=lm<0?0:lm;
	
	return data[im*ny*nz*nt + jm*nz*nt + km*nt + lm];
}

template<typename T>
void FFArray<T>::setVal(const double* tab){
	for ( size_t i = 0; i < size; i++){
		data[i] = tab[i];
	}
}

template<typename T>
T* FFArray<T>::getData(){
	return data;
}

template<typename T>
size_t FFArray<T>::getSize(){
	return size;
}

template<typename T>
size_t FFArray<T>::getDim(string dim){
	if ( dim == "x" ) return nx;
	if ( dim == "y" ) return ny;
	if ( dim == "z" ) return nz;
	if ( dim == "t" ) return nt;
	return size;
}

template<typename T>
void FFArray<T>::dumpBin(std::ofstream&  FileOut){
	FileOut.write(reinterpret_cast<const char*>(&nx), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&ny), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nz), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(&nt), sizeof(size_t));
	FileOut.write(reinterpret_cast<const char*>(data), nx*ny*nz*nt*sizeof(T));
}

template<typename T>
void FFArray<T>::loadBin(std::ifstream&  FileIn){
	size_t nnx;
	size_t nny;
	size_t nnz;
	size_t nnt;
	FileIn.read((char *)&nnx, sizeof(size_t));
	FileIn.read((char *)&nny, sizeof(size_t));
	FileIn.read((char *)&nnz, sizeof(size_t));
	FileIn.read((char *)&nnt, sizeof(size_t));
	if((nnx != nx) ||(nny != ny)) {
		cout << "LOADING  NOT good dimentions in FARRAY "<<(nx*ny)<<"::"<<(nnx*nny)<<" read "<<nnx<<":"<<nny<<":"<<nnz<<":"<<nnt<<endl;
		return;
	}
	FileIn.read((char *)data, nx*ny*nz*nt*sizeof(T));
}

template<typename T>
void FFArray<T>::loadBinAtLoc(std::ifstream&  FileIn, size_t startI, size_t startJ,size_t domFsize){

	size_t nnx;
	size_t nny;
	size_t nnz;
	size_t nnt;
	
	if(domFsize <= 5*sizeof(size_t)) {
		cout << "LOADING EMPTY wind dimentions in FARRAY "<<domFsize<<" bytes"<<endl;
		return;
	}
	FileIn.read((char *)&nnx, sizeof(size_t));
	FileIn.read((char *)&nny, sizeof(size_t));
	FileIn.read((char *)&nnz, sizeof(size_t));
	FileIn.read((char *)&nnt, sizeof(size_t));
	//cout <<"at "<<startI<<" - "<<startJ<< " nx:"<<nx<<" nnx"<<nnx<< " ny:"<<ny<<" nny"<<nny<<endl;
	size_t nelems = domFsize/sizeof(T);
	if(nnx*nny*nnz*nnt + 4 != nelems) {
		cout << "LOADING NOT good wind dimentions in FARRAY "<<nelems<<" and read "<<nnx<<":"<<nny<<":"<<nnz<<":"<<nnt<<":"<<endl;
		return;
	}
	FileIn.ignore((nny+2)*sizeof(T));
	for (size_t i = startI+1; i < startI+nnx; i++ ){
		FileIn.read((char *)&(data[i*ny + startJ+2]), (nny-3)*sizeof(T));
		FileIn.ignore(3*sizeof(T));
	}
}



template<typename T>
void FFArray<T>::setDataAtLoc(T* matrix, size_t nnx, size_t nny,size_t startI, size_t startJ,size_t domFsize){

	size_t mi =(nny+2);
	//cout <<"UUUUat "<<startI<<" - "<<startJ<< " nx:"<<nx<<" nnx"<<nnx<< " ny:"<<ny<<" nny"<<nny<<endl;
	//FileIn.ignore((nny+2)*sizeof(T));
	// for (size_t i = startI+1; i < startI+nnx; i++ ){
	//	FileIn.read((char *)&(data[i*ny + startJ+2]), (nny-3)*sizeof(T));
	//	FileIn.ignore(3*sizeof(T));
	//}
	for (size_t i = startI+1; i < startI+nnx; i++ ){
		for (size_t j = 0; j < (nny-3); j++ ){
			data[i*ny + startJ+j+2] = matrix[mi+j];
		}
		mi += nny;
	}
			
/*	

	FileIn.read((char *)&nnx, sizeof(size_t));
	FileIn.read((char *)&nny, sizeof(size_t));
	FileIn.read((char *)&nnz, sizeof(size_t));
	FileIn.read((char *)&nnt, sizeof(size_t));

	size_t nelems = domFsize/sizeof(T);
	if(nnx*nny*nnz*nnt + 4 != nelems) {
		cout << "LOADING NOT good wind dimentions in FARRAY "<<nelems<<" and read "<<nnx<<":"<<nny<<":"<<nnz<<":"<<nnt<<":"<<endl;
		return;
	}
	FileIn.ignore((nny+2)*sizeof(T));
	for (size_t i = startI+1; i < startI+nnx; i++ ){
		FileIn.read((char *)&(data[i*ny + startJ+2]), (nny-3)*sizeof(T));
		FileIn.ignore(3*sizeof(T));
	}*/
}

	
//}
template<typename T>
void FFArray<T>::resize(size_t ni, size_t nj, size_t nk, size_t nl){
	nx = ni;
	ny = nj;
	nz = nk;
	nt = nl;
	size = nx*ny*nz*nt;
	try {
		// erasing previous data
		delete [] data;
		// allocation of enough storage
		data = new T[size];
		// initialization to zero
		for ( int i=0; i < size; i++ ) {
			data[i] = 0.;
		}
	} catch ( const bad_alloc& ) {
		cout << "Problem in the resizing array "
				<<name<<": not enough space !!" << endl;
	}
}

template<typename T>
void FFArray<T>::copyDataToFortran(T* x) {
	if ( nt > 1 ){
		cout<<"WARNING: trying to copy 4D data "
				<<name<<" to Fortran !!"<<endl;
		return;
	}
	size_t indC, indF;
	size_t ii, jj, kk, rest;
	try {
		for ( indC = 0; indC < size; indC++ ) {
			/* first compute the indices
			in the C representation of the array*/
			ii = indC/(ny*nz);
			rest = indC - ii*ny*nz;
			jj = rest/nz;
			kk = rest%nz;
			/* then compute the corresponding index
			in Fortran representation */
			indF = kk*nx*ny + jj*nx + ii;
			x[indF] = data[indC];

		}
	} catch (...) {
		cout << "PROBLEM in passing C array "
				<<name<<" to Fortran !!" << endl;
	}
}

template<typename T>
void FFArray<T>::copyDataFromFortran(const T* x) {
	if ( nt > 1 ){
		cout<<"WARNING: trying to copy 4D data "
				<<name<<" from Fortran !!"<<endl;
		return;
	}
	size_t indC, indF;
	size_t ii, jj, kk, rest;
	try {
		for ( indF = 0; indF < size; indF++ ) {
			/* first compute the indices
			in the Fortran representation of the array*/
			kk = indF/(nx*ny);
			rest = indF - kk*nx*ny;
			jj = rest/nx;
			ii = rest%nx;
			/* then compute the corresponding index
			in C representation */
			indC = ii*ny*nz + jj*nz + kk;
			data[indC] = x[indF];
		}
	} catch (...) {
		cout << "Problem in passing a Fortran array to C array "
				<<name<<endl;
	}
}

template<typename T>
string FFArray<T>::print2D(size_t k, size_t l) {
	ostringstream oss;
	if ( k<nz and l<nt ) {
		for (size_t j = 0; j < ny ; j++ ){
			for (size_t i = 0; i < nx; i++ ){
				oss << data[i*ny*nz*nt + j*nz*nt + k*nt + l] << " ";
			}
			oss<<endl;
		}
	} else {
		cout << "Array subscript (0,0,"<<k<<","<<l
				<<") out of bounds in print function for array "
				<<name<< endl;
	}
	return oss.str();
}

}
#endif /* FFARRAYS_H_ */
