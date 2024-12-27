
# NetCDF library paths to set here if required
set(netcdfHome /Users/filippi_j/soft/)
link_directories(${netcdfHome}/lib)
include_directories(${netcdfHome}/include)


cmake_minimum_required(VERSION 3.10)
project(forefire VERSION 1.0)

# Set C++17 as the standard for the project
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
# Compiler flags
# DEBUG
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g  -DMPI_COUPLING")
# Optimized for MPI, 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -flto -fomit-frame-pointer -finline-functions  -march=native -funroll-loops  -ftree-vectorize -fno-strict-aliasing  -DMPI_COUPLING")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")
# Aggressive warning, aggresive wont work for MPI due to memory missalignement - so no MPI
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall  -Wsign-compare -O3 -march=native -funroll-loops -fstrict-aliasing -ftree-vectorize")

# Output directories for binaries and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# Link against NetCDF libraries globally (if all targets need it)
link_libraries("-lnetcdf_c++4" ${MPI_LIBRARIES})

# Main project executable
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(forefire ${SRC_FILES})
# Shared library
add_library(forefireL SHARED ${SRC_FILES})
# Specific main for the test if different
set(TEST_MAIN tools/runANN/ANNTest.cpp)
add_executable(ANN_test ${TEST_MAIN})






