#ifndef PLIBFOREFIRE_H
#define PLIBFOREFIRE_H

#define NETCDF_NOT_LEGACY

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

namespace py = pybind11;


typedef struct _SignedIntBuf
{
  int* data;
  int shape[2];
  int strides[2];
} SignedIntBuf;

#include <Command.h>
#include <FFArrays.h>
#include <Futils.h>
#include <SimulationParameters.h>
#include <CLibForeFire.h>

using namespace std;

class PLibForeFire {


public:
PLibForeFire();
std::string execute(char *);
void createDomain( int id
		,  int year,  int month
		,  int day,  double t
		,  double lat,  double lon
		,  int mdimx,  double* meshx
		,  int mdimy,  double* meshy
		,  int mdimz,  double* zgrid
		,  double dt);


void addScalarLayer(char *type,char *name, double x0 , double y0, double t0, double width , double height, double timespan, int nnx, int nny, int nnz, int nnl, py::array_t<double> values);
void addIndexLayer(char *type,char *name, double x0 , double y0, double t0, double width , double height, double timespan, int nnx, int nny, int nnz, int nnl, py::array_t<int> values);
void addLayer(char*, char* ,char*);
void setInt(char* name, int val);
int getInt(char* name );
void setDouble(char* name, double val);
double getDouble(char* name);
py::array_t<double> getDoubleArray(char* name);
py::array_t<double> getDoubleArray(char* name, double t);
void setString(char* name, char* val);
std::string getString(char* name);

};

#endif