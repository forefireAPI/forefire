#include "_pyforefire.h"

namespace py = pybind11;

using namespace std;
using namespace libforefire;
#include <iostream>

Command* pyxecutor;
Command::Session* session;
SimulationParameters* params;


PLibForeFire::PLibForeFire() {
	pyxecutor = new Command();
	session = &(pyxecutor->currentSession);
	params = session->params;
}

void PLibForeFire::createDomain( int id
		,  int year,  int month
		,  int day,  double t
		,  double lat,  double lon
		,  int mdimx,  double* meshx
		,  int mdimy,  double* meshy
		,  int mdimz,  double* zgrid
		,  double dt){


	/* Defining the Fire Domain */
		if (session->fd) delete session->fd;

		session->fd = new FireDomain(id, year, month, day, t, lat, lon
				, mdimx, meshx, mdimy, meshy, mdimz, dt);

		// pyxecutor->getDomain() = session->fd; // FIXME

		// A FireDomain has been created, the level is increased
		pyxecutor->increaseLevel();
		session->ff = session->fd->getDomainFront();
		// Defining the timetable of the events to be be in the domain
		if (session->tt) delete session->tt;
		session->tt = new TimeTable();
		// Associating this timetable to the domain
		session->fd->setTimeTable(session->tt);
		// Defining the simulator
		if (session->sim) delete session->sim;
		session->sim = new Simulator(session->tt, session->fd->outputs);


		session->outStrRep = new StringRepresentation(pyxecutor->getDomain());
		if ( SimulationParameters::GetInstance()->getInt("outputsUpdate") != 0 ){
			session->tt->insert(new FFEvent(session->outStrRep));
		}

		double deltaT = session->fd->getSecondsFromReferenceTime(year, month, day, t);

		pyxecutor->setReferenceTime(deltaT);
		pyxecutor->setStartTime(deltaT);
}

void PLibForeFire::addLayer(char *type, char* layername, char* keyname){

	pyxecutor->getDomain()->addLayer(string(type),string(layername),string(keyname));
}

void PLibForeFire::setInt(char* name, int val){
	string lname(name);
	params->setInt(lname,val);
}

int PLibForeFire::getInt(char* name ){
	string lname(name);
	return params->getInt(lname);
}

void PLibForeFire::setDouble(char* name, double val){
	string lname(name);
	params->setDouble(lname,val);
}

double PLibForeFire::getDouble(char* name ){
	string lname(name);
	return params->getDouble(lname);
}

void PLibForeFire::setString(char* name, char* val){
	string lname(name);
	string lval(val);
		params->setParameter(lname,val);
}

string PLibForeFire::getString(char *name)
{

	return params->getParameter(string(name));
}

string PLibForeFire::execute(char *command)
{
	ostringstream stringOut;
	pyxecutor->setOstringstream(&stringOut);
	string smsg(command);
	pyxecutor->ExecuteCommand(smsg);
	return stringOut.str();
}


void PLibForeFire::addScalarLayer(char *type, char *name, double x0 , double y0, double t0, double width , double height, double timespan, int nnx, int nny, int nnz, int nnl, py::array_t<double> values){

	FFPoint *p0 = new FFPoint(x0,y0,0);
	FFPoint *pe = new FFPoint(width,height,0);
	string lname(name);
	string ltype(type);

	size_t ni = nnx;
	size_t nj = nny;
	size_t nk = nnz;
	size_t nl = nnl;

 	pyxecutor->getDomain()->addScalarLayer(type, lname, x0, y0, t0, width, height, timespan, ni, nj, nk, nl, values.mutable_data());

}

void PLibForeFire::addIndexLayer(char *type, char *name, double x0 , double y0, double t0, double width , double height, double timespan, int nnx, int nny, int nnz, int nnl, py::array_t<int> values){
	FFPoint *p0 = new FFPoint(x0,y0,0);
	FFPoint *pe = new FFPoint(width,height,0);
	string lname(name);
	string ltype(type);

	size_t ni = nnx;
	size_t nj = nny;
	size_t nk = nnz;
	size_t nl = nnl;

 	pyxecutor->getDomain()->addIndexLayer(ltype, lname, x0, y0, t0, width, height, timespan, ni, nj, nk, nl, values.mutable_data());
}

py::array_t<double> PLibForeFire::getDoubleArray(char* name){
	double lTime = pyxecutor->getDomain()->getSimulationTime();

	return PLibForeFire::getDoubleArray(name, lTime);
}

py::array_t<double> PLibForeFire::getDoubleArray(char* name, double t){
	string lname(name);
	FluxLayer<double>* myFluxLayer = pyxecutor->getDomain()->getFluxLayer(lname);

		if ( myFluxLayer ){
			FFArray<double>* srcD;
			myFluxLayer->getMatrix(&srcD, t);
			double* data = srcD->getData();
			int nnx = srcD->getDim("x");
			int nny = srcD->getDim("y");
			int nnz = srcD->getDim("z");
			int nnt = srcD->getDim("t");
            constexpr size_t stride_size = sizeof(double);
            size_t total_size = static_cast<size_t>(nnx) * nny * nnz * nnt;
             
             // Temporary vector to hold reshaped data
             std::vector<double> reshaped_data(total_size);
             
             // Reshape data from C to Fortran order
             for (int t = 0; t < nnt; ++t) {
                 for (int z = 0; z < nnz; ++z) {
                     for (int y = 0; y < nny; ++y) {
                         for (int x = 0; x < nnx; ++x) {
                             size_t c_index = x + nnx * (y + nny * (z + nnz * t)); // C order index
                             size_t fortran_index = t + nnt * (z + nnz * (y + nny * x)); // Fortran order index
                             reshaped_data[c_index] = data[fortran_index];
                         }
                     }
                 }
             }
             
             // Create py::array_t from the reshaped data
             py::array_t<double> arr(
                 {nnt, nnz, nny, nnx}, // shape as requested
                 reshaped_data.data() // Now directly use reshaped data
             );
			return arr;
		}

	DataLayer<double>* myDataLayer = pyxecutor->getDomain()->getDataLayer(lname);

		if ( myDataLayer ){
			FFArray<double>* srcD;
			myDataLayer->getMatrix(&srcD, t);
			double* data = srcD->getData();
			int nnx = srcD->getDim("x");
			int nny = srcD->getDim("y");
			int nnz = srcD->getDim("z");
			int nnt = srcD->getDim("t");
            constexpr size_t stride_size = sizeof(double);
            size_t total_size = static_cast<size_t>(nnx) * nny * nnz * nnt;
             
             // Temporary vector to hold reshaped data
             std::vector<double> reshaped_data(total_size);
             
             // Reshape data from C to Fortran order
             for (int t = 0; t < nnt; ++t) {
                 for (int z = 0; z < nnz; ++z) {
                     for (int y = 0; y < nny; ++y) {
                         for (int x = 0; x < nnx; ++x) {
                             size_t c_index = x + nnx * (y + nny * (z + nnz * t)); // C order index
                             size_t fortran_index = t + nnt * (z + nnz * (y + nny * x)); // Fortran order index
                             reshaped_data[c_index] = data[fortran_index];
                         }
                     }
                 }
             }
             
             // Create py::array_t from the reshaped data
             py::array_t<double> arr(
                 {nnt, nnz, nny, nnx}, // shape as requested
                 reshaped_data.data() // Now directly use reshaped data
             );
             
             return arr;
			
			
			
		}

		double* data = NULL;
		py::array_t<double> arr = py::array_t<double>(
			{0, 0, 0}, // shape
			{8, 8, 8},
			data
		);
		return arr;
}

PYBIND11_MODULE(_pyforefire, m) {
    m.doc() = "pybind11 pyforefire plugin"; // optional module docstring

    py::class_<PLibForeFire>(m, "ForeFire")
        .def(py::init())
		.def("createDomain", &PLibForeFire::createDomain)
        .def("addLayer", &PLibForeFire::addLayer)
		.def("setInt", &PLibForeFire::setInt)
		.def("getInt", &PLibForeFire::getInt)
		.def("getInt", &PLibForeFire::getInt)
		.def("setDouble", &PLibForeFire::setDouble)
		.def("getDouble", &PLibForeFire::getDouble)
		.def("setString", &PLibForeFire::setString)
		.def("getString", &PLibForeFire::getString)
		.def("execute", &PLibForeFire::execute)
		.def("addScalarLayer", [](PLibForeFire& self, char *type, char *name, double x0 , double y0, double t0, double width , double height, double timespan, py::array_t<double> values) {
            size_t nn[] = {1, 1, 1, 1};
            const long* shape = values.shape();
        
            for (ssize_t i = 0; i < values.ndim(); i += 1) {
                nn[i] = (size_t)*shape;
                ++shape;
            }
        
            // Assuming nn contains the dimensions in Fortran (column-major) order
            size_t nx = nn[3], ny = nn[2], nz = nn[1], nnt = nn[0];
            size_t size = nnt * nz * ny * nx;
            auto dataC = std::vector<double>(size); // C-style array
        
            auto r = values.unchecked<4>(); // Assuming values is a 4D array; use unchecked for read-only access
        
            for (size_t ll = 0; ll < nnt; ++ll) {
                for (size_t kk = 0; kk < nz; ++kk) {
                    for (size_t jj = 0; jj < ny; ++jj) {
                        for (size_t ii = 0; ii < nx; ++ii) {
                            // Convert Fortran index to C index on the fly
                            size_t indF = ll * (nx * ny * nz) + kk * (nx * ny) + jj * nx + ii;
                            size_t indC = ii * (ny * nz * nnt) + jj * (nz * nnt) + kk * nnt + ll;
        
                            // No temporary array; direct assignment
                            dataC[indC] = r(ll, kk, jj, ii); // Use the indices according to Fortran order in r()
                        }
                    }
                }
            }
        
            // Pass the reshaped data to the original addIndexLayer function
            // Need to convert dataC back to a format that addIndexLayer expects (e.g., py::array)
            py::array_t<double> dataC_py = py::array(size, dataC.data());
            return self.addScalarLayer(type, name, x0, y0, t0, width, height, timespan, nx, ny, nz, nnt, dataC_py);
                                                                                        
 
		})
		.def("addIndexLayer", [](PLibForeFire& self, char *type, char *name, double x0 , double y0, double t0, double width , double height, double timespan, py::array_t<int> values) {
            size_t nn[] = {1, 1, 1, 1};
            const long* shape = values.shape();
        
            for (ssize_t i = 0; i < values.ndim(); i += 1) {
                nn[i] = (size_t)*shape;
                ++shape;
            }
        
            // Assuming nn contains the dimensions in Fortran (column-major) order
            size_t nx = nn[3], ny = nn[2], nz = nn[1], nnt = nn[0];
            size_t size = nnt * nz * ny * nx;
            auto dataC = std::vector<int>(size); // C-style array
        
            auto r = values.unchecked<4>(); // Assuming values is a 4D array; use unchecked for read-only access
        
            for (size_t ll = 0; ll < nnt; ++ll) {
                for (size_t kk = 0; kk < nz; ++kk) {
                    for (size_t jj = 0; jj < ny; ++jj) {
                        for (size_t ii = 0; ii < nx; ++ii) {
                            // Convert Fortran index to C index on the fly
                            size_t indF = ll * (nx * ny * nz) + kk * (nx * ny) + jj * nx + ii;
                            size_t indC = ii * (ny * nz * nnt) + jj * (nz * nnt) + kk * nnt + ll;
        
                            // No temporary array; direct assignment
                            dataC[indC] = r(ll, kk, jj, ii); // Use the indices according to Fortran order in r()
                        }
                    }
                }
            }
        
            // Pass the reshaped data to the original addIndexLayer function
            // Need to convert dataC back to a format that addIndexLayer expects (e.g., py::array)
            py::array_t<int> dataC_py = py::array(size, dataC.data());
            return self.addIndexLayer(type, name, x0, y0, t0, width, height, timespan, nx, ny, nz, nnt, dataC_py);

		})
		.def("getDoubleArray", [](PLibForeFire& self, char* name) {
			return self.getDoubleArray(name);
		})
		.def("__setitem__", [](PLibForeFire &self, const std::string &key, py::object value) {
            if (py::isinstance<py::int_>(value)) {
                // Integer value
                self.setInt(const_cast<char*>(key.c_str()), value.cast<int>());
            } else if (py::isinstance<py::float_>(value)) {
                // Double value
                self.setDouble(const_cast<char*>(key.c_str()), value.cast<double>());
            } else if (py::isinstance<py::str>(value)) {
                // String value
                std::string val = value.cast<std::string>();
                self.setString(const_cast<char*>(key.c_str()), const_cast<char*>(val.c_str()));
            } else {
                throw std::runtime_error("Unsupported value type");
            }
        })
        .def("__getitem__", [&](PLibForeFire &self, const std::string &key) -> py::object {
            // This example assumes that you can somehow determine the type of the parameter
            // beforehand, which might require additional logic in your C++ code that's not
            // shown here.

            // Placeholder for type checking. You need to implement actual type determination logic.
            // For demonstration, this code assumes everything is treated as a string for retrieval.

            // Attempt to get a string, and if it fails, fall back to double, then int.
            try {
                return py::cast(self.getString(const_cast<char*>(key.c_str())));
            } catch (...) {
                try {
                    return py::cast(self.getDouble(const_cast<char*>(key.c_str())));
                } catch (...) {
                    return py::cast(self.getInt(const_cast<char*>(key.c_str())));
                }
            }
        });
}
