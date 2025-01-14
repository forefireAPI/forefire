cmake_minimum_required(VERSION 3.10)
project(forefire VERSION 1.0)

# ----------------------------------
# Set C++ Standard
# ----------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------
# NetCDF Paths (via environment variables)
# ----------------------------------
# You can also switch to find_package(NetCDF) if preferred
if(NOT DEFINED ENV{NETCDF_HOME})
    message(STATUS "NETCDF_HOME not set. Attempting default path or system library paths.")
    # Fallback logic could go here if needed
else()
    set(NETCDF_HOME $ENV{NETCDF_HOME})
    include_directories(${NETCDF_HOME}/include)
    link_directories(${NETCDF_HOME}/lib)
endif()

# ----------------------------------
# ECCodes detection
# ----------------------------------
# Use an option so users can toggle ECCodes build
option(ENABLE_ECCODES "Enable ECCodes support" ON)

set(ECCODES_FOUND FALSE)
if(ENABLE_ECCODES)
    if(NOT DEFINED ENV{ECCODES_HOME})
        message(STATUS "ECCODES_HOME not set. Will look in system library paths.")
    else()
        set(ECCODES_HOME $ENV{ECCODES_HOME})
        include_directories(${ECCODES_HOME}/include)
        link_directories(${ECCODES_HOME}/lib)
        link_directories(${ECCODES_HOME}/lib64)  # Some systems use lib64
    endif()

    # Simple check for the library:
    find_library(ECCODES_LIB
        NAMES eccodes
        HINTS ${ECCODES_HOME}/lib ${ECCODES_HOME}/lib64
    )
    if(ECCODES_LIB)
        set(ECCODES_FOUND TRUE)
        message(STATUS "ECCodes found: ${ECCODES_LIB}")
    else()
        message(WARNING "ECCodes not found. grib2ffnc will not be built.")
    endif()
endif()

# ----------------------------------
# MPI detection
# ----------------------------------
# If user specifically passes MPI compilers, try to use them
if(MPI_C_COMPILER AND MPI_CXX_COMPILER)
    message(STATUS "Using manually specified MPI compilers:")
    message(STATUS "  MPI_C_COMPILER: ${MPI_C_COMPILER}")
    message(STATUS "  MPI_CXX_COMPILER: ${MPI_CXX_COMPILER}")

    # Use them as actual compilers
    set(CMAKE_C_COMPILER "${MPI_C_COMPILER}")
    set(CMAKE_CXX_COMPILER "${MPI_CXX_COMPILER}")

    # If user also provides these paths/libraries:
    if(MPI_C_LIBRARIES)
        link_directories(${MPI_C_LIBRARIES})
    endif()
    if(MPI_CXX_LIBRARIES)
        link_directories(${MPI_CXX_LIBRARIES})
    endif()

    # We assume MPI is found in this scenario
    set(MPI_FOUND TRUE)
else()
    # Otherwise, try find_package
    find_package(MPI)
    if(MPI_FOUND)
        message(STATUS "MPI found via find_package")
        include_directories(${MPI_INCLUDE_PATH})
    else()
        message(WARNING "MPI not found. MPI targets will not be built.")
    endif()
endif()

# ----------------------------------
# Compiler Flags
# Adjust or add as needed
# ----------------------------------
# Example release flags for MPI
if(MPI_FOUND)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wno-unused-parameter -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing -DMPI_COUPLING"
    )
else()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wno-unused-parameter -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing"
    )
endif()

# ----------------------------------
# Output Directories
# ----------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../lib)

# ----------------------------------
# Global Linking (NetCDF, MPI)
# ----------------------------------
# NetCDF typically needs -lnetcdf and possibly -lnetcdf_c++4
# You can adapt this if your NetCDF library uses different names
if(MPI_FOUND)
    # If find_package(MPI) was used, you typically link to MPI::MPI_CXX or ${MPI_LIBRARIES}.
    # For simplicity here, assume ${MPI_LIBRARIES}.
    link_libraries(netcdf netcdf_c++4 ${MPI_LIBRARIES})
else()
    link_libraries(netcdf netcdf_c++4)
endif()

# ----------------------------------
# Main Project Executable
# ----------------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)
# Build only if MPI is found or you do not strictly require MPI
add_executable(forefire ${SRC_FILES})
if(MPI_FOUND)
    target_link_libraries(forefire ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
endif()

# ----------------------------------
# Shared library
# ----------------------------------
add_library(forefireL SHARED ${SRC_FILES})
if(MPI_FOUND)
    target_link_libraries(forefireL ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
endif()

# ----------------------------------
# Test executable
# ----------------------------------
set(TEST_MAIN tools/runANN/ANNTest.cpp)
add_executable(ANN_test ${TEST_MAIN})
if(MPI_FOUND)
    target_link_libraries(ANN_test ${MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
endif()

# ----------------------------------
# grib2ffnc (build only if ECCodes found)
# ----------------------------------
set(DATA_COMPILE tools/grib2weatherNC/grib2ffnc.cpp)
if(ECCODES_FOUND)
    add_executable(grib2ffnc ${DATA_COMPILE})
    # For ECCodes we link with -leccodes
    if(MPI_FOUND)
        target_link_libraries(grib2ffnc ${ECCODES_LIB} ${MPI_LIBRARIES})
    else()
        target_link_libraries(grib2ffnc ${ECCODES_LIB})
    endif()
else()
    message(WARNING "ECCodes not found; skipping grib2ffnc target.")
endif()
