/**
 * @file FireDomain_nc.cpp
 * @brief NetCDF arrival-time load and save for FireDomain. Compiled only when FOREFIRE_ENABLE_NETCDF is ON; netcdf_stubs.cpp stands in otherwise.
 * @copyright Copyright (C) 2025 ForeFire, Fire Team, SPE, CNRS/Universita di Corsica.
 * @license This program is free software; See LICENSE file for details. (See LICENSE file).
 * @author Jean‑Baptiste Filippi — 2025
 */

#include "FireDomain.h"

#include <netcdf>

using namespace netCDF;
using namespace netCDF::exceptions;

namespace libforefire{

	 void FireDomain::loadArrivalTimeNC(string fname){
			 if (getDomainID()!=0) return;
 
				 try
						 {
								 NcFile dataFile(fname.c_str(), NcFile::read);
								 
								 NcVar atime = dataFile.getVar("arrival_time_of_front");
								 
								 size_t   FSPACE_DIM1 		= atime.getDim(1).getSize();	// Nb De lignes au total
								 size_t   FSPACE_DIM2 		= atime.getDim(0).getSize();	// 	Nb De colonnes au total
 
								 if((globalBMapSizeX!=FSPACE_DIM1)||(globalBMapSizeY!=FSPACE_DIM2)){
													 cout<<getDomainID()<<":ERROR: number of cells of the burning matrices ("
													 <<globalBMapSizeX<<"x"<<globalBMapSizeY<<")"
													 <<" not compatible with read data "
													 <<FSPACE_DIM1<<"x"<<FSPACE_DIM2<<endl;
													 dataFile.close();
													 return;
											  
								 }
 
								 NcVar dom = dataFile.getVar("domain"); 
				 
								 double Xorigin = 0;
								 dom.getAtt("SWx").getValues(&Xorigin);
								 double Yorigin = 0;
								 dom.getAtt("SWy").getValues(&Yorigin);
								 double Xlen = 0;
								 dom.getAtt("Lx").getValues(&Xlen);
								 Xlen = Xlen;
								 double Ylen = 0;
								 dom.getAtt("Ly").getValues(&Ylen);
								 Ylen=Ylen;
								 int pyear = 0;
								 dom.getAtt("refYear").getValues(&pyear);
								 int pday = 0;
								 dom.getAtt("refDay").getValues(&pday);
								 refYear = pyear;
								 params->setInt("refYear", pyear);
								 refDay = pday;
								 params->setInt("refDay", pday);
 
								 double max_time = params->getDouble("InitTime");
		 // Dynamically allocate the matrix on the heap
  
								 double* allDataAtime = new double[globalBMapSizeY * globalBMapSizeX]; 
								 
								 atime.getVar(allDataAtime);
 
								 double tmpval = 0;    
								 for (size_t i = 0; i < globalBMapSizeX; i++) {
									 for (size_t j = 0; j < globalBMapSizeY; j++) {
										 tmpval = allDataAtime[j * globalBMapSizeX + i]; 
										 if ((tmpval < max_time) && (tmpval > -9999)) {
											 this->setArrivalTime(i , j,tmpval );
										 }
									 }
								 }
								 dataFile.close();
								 cout<<getDomainID()<<": Read "<< fname<<"("<<globalBMapSizeX<<"x"<<globalBMapSizeY<<")" <<FSPACE_DIM1<<"x"<<FSPACE_DIM2<<endl;
								 delete[] allDataAtime;
 
 
					 }
					 catch (std::exception const & e)
					 {
						 cout << "Exception: " << e.what() << endl;
					 }
					 catch (...)
					 {
						 cout << "Error: unknown error." << endl;
					 } 
						 
			 
			  
		 }
 
   void FireDomain::saveArrivalTimeNC(){
	 
	try {
		 // Setup file paths and identifiers
		 ostringstream oss;
		 oss << params->getParameter("caseDirectory") << '/'
			 << params->getParameter("fireOutputDirectory") << '/'
			 << params->getParameter("experiment") << "." << getDomainID() << ".nc";
 
		 // Create and configure the NetCDF file
		 NcFile dataFile(oss.str(), NcFile::replace);
		 NcDim xDim = dataFile.addDim("DIMX", globalBMapSizeX); // Width
		 NcDim yDim = dataFile.addDim("DIMY", globalBMapSizeY); // Height
		 vector<NcDim> dims = {yDim, xDim}; // Order is important for visualization
		 NcVar atime = dataFile.addVar("arrival_time_of_front", ncDouble, dims);
		 atime.setCompression(true, true, 6);
  
		 double* matrix = new double[globalBMapSizeY * globalBMapSizeX];
 
		 for (size_t i = 0; i < globalBMapSizeX; i++) {
			 for (size_t j = 0; j < globalBMapSizeY; j++) {
				 double tmpval = this->getArrivalTime(i , j );
				 if (std::isinf(tmpval)) { 
					 matrix[j * globalBMapSizeX + i] =  -9999 ;
				 }else{
					 matrix[j * globalBMapSizeX + i] =  tmpval; 
				 }
			 }
		 }
		 
		 // Write the arrival time data
		 atime.putVar(&matrix[0]);
		 // Add domain and reference attributes
		 NcDim domdim = dataFile.addDim("domdim", 1);
		 NcVar dom = dataFile.addVar("domain", ncChar, domdim);
		 dom.putAtt("SWx", NC_DOUBLE, SWCorner.getX());
		 dom.putAtt("SWy", NC_DOUBLE, SWCorner.getY());
		 dom.putAtt("Lx", NC_DOUBLE, NECorner.getX() - SWCorner.getX());
		 dom.putAtt("Ly", NC_DOUBLE, NWCorner.getY() - SWCorner.getY());
		 dom.putAtt("Lz", NC_DOUBLE, 0.);
		 dom.putAtt("refYear", NC_INT, int(refYear));
		 dom.putAtt("refDay", NC_INT, int(refDay));
		 // Close the file
		 dataFile.close();
 
 
		 delete[] matrix;
 
	 } 
	 catch (std::exception const & e)
	 {
		 cout << "Exception in creating a_time: " << e.what() << endl;
	 }
	 catch (...)
	 {
		 cout << "Error: unknown error." << endl;
	 } 
	 }
 
 

}
