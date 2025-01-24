cmake_minimum_required(VERSION 3.10)
project(forefire VERSION 1.0)

# ----------------------------------
# Set C++ Standard
# ----------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


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
# Check for MESONH and XYZ Environment Variables
# ----------------------------------
# These variables override the standard NETCDF_HOME if set
if(DEFINED ENV{MESONH} AND DEFINED ENV{XYZ})
    message(STATUS "MESONH and XYZ environment variables are set. Configuring static NetCDF.")
    
    # Set MESONH and XYZ from environment variables
    set(MESONH $ENV{MESONH})
    set(XYZ $ENV{XYZ})
    
    # Automatically detect NETCDF_HOME by finding the NETCDF-* directory
    file(GLOB NETCDF_DIR "${MESONH}/src/dir_obj${XYZ}/MASTER/NETCDF-*")
    if(NETCDF_DIR)
        list(GET NETCDF_DIR 0 NETCDF_HOME)
        message(STATUS "Detected NETCDF_HOME: ${NETCDF_HOME}")
    else()
        message(FATAL_ERROR "No NETCDF-* directory found under ${MESONH}/src/dir_obj${XYZ}/MASTER/")
    endif()
    
    # Include directories
    include_directories(${NETCDF_HOME}/include)
    
    # Find ZLIB
    find_package(ZLIB REQUIRED)
    if(ZLIB_FOUND)
        message(STATUS "ZLIB found: ${ZLIB_LIBRARIES}")
    else()
        message(FATAL_ERROR "ZLIB not found")
    endif()
    
    # Specify static libraries
    set(NETCDF_LIB "${NETCDF_HOME}/lib64/libnetcdf.a")
    set(NETCDF_CXX_LIB "${NETCDF_HOME}/lib64/libnetcdf_c++4.a")
    set(HDF5_LIB "${NETCDF_HOME}/lib64/libhdf5.a")
    set(HDF5_HL_LIB "${NETCDF_HOME}/lib64/libhdf5_hl.a")
    set(SZIP_LIB "${NETCDF_HOME}/lib64/libsz.a")
    set(AEC_LIB "${NETCDF_HOME}/lib64/libaec.a")  # Added libaec.a
    
    # Check if all required static libraries exist
    foreach(lib IN LISTS NETCDF_LIB NETCDF_CXX_LIB HDF5_LIB HDF5_HL_LIB SZIP_LIB AEC_LIB)
        if(NOT EXISTS ${lib})
            message(FATAL_ERROR "Required static library not found: ${lib}")
        endif()
    endforeach()
    
    # Create a variable with all libraries needed for static linking
    set(NETCDF_STATIC_LIBS
        ${NETCDF_LIB}
        ${NETCDF_CXX_LIB}
        ${HDF5_LIB}
        ${HDF5_HL_LIB}
        ${SZIP_LIB}
        ${AEC_LIB}               # Included libaec.a
        ${ZLIB_LIBRARIES}       # from find_package(ZLIB)
    )
    
    # Ensure static linking by specifying the full path to the static libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    
    # Optionally, prevent CMake from searching for shared libraries
    set(BUILD_SHARED_LIBS OFF)
    
else()
    # ----------------------------------
    # NetCDF Paths (via environment variables)
    # ----------------------------------
    if(DEFINED ENV{NETCDF_HOME})
        set(NETCDF_HOME $ENV{NETCDF_HOME})
        include_directories(${NETCDF_HOME}/include)
        link_directories(${NETCDF_HOME}/lib)
        message(STATUS "NETCDF_HOME set to ${NETCDF_HOME}")
    else()
        message(STATUS "NETCDF_HOME not set. Attempting default path or system library paths.")
        # Fallback logic could go here if needed
    endif()
       

    if(MPI_FOUND)
        link_libraries(netcdf netcdf_c++4 ${MPI_LIBRARIES})
    else()
        link_libraries(netcdf netcdf_c++4)
    endif()
    
endif()  # End of MESONH and XYZ check
    
# ----------------------------------
# Compiler Flags
# Adjust or add as needed
# ----------------------------------
# Example release flags for MPI
if(MPI_FOUND)
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing -DMPI_COUPLING"
    )
else()
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O3 -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing"
    )
endif()

# ----------------------------------
# Output Directories
# ----------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# ----------------------------------
# Main Project Executable
# ----------------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)
# Build only if MPI is found or you do not strictly require MPI
add_executable(forefire ${SRC_FILES})

if(DEFINED NETCDF_STATIC_LIBS)
    if(MPI_FOUND)
        target_link_libraries(forefire PRIVATE ${NETCDF_STATIC_LIBS} ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefire PRIVATE ${NETCDF_STATIC_LIBS})
    endif()
else()
    # Non-static linking case
    if(MPI_FOUND)
        target_link_libraries(forefire PRIVATE ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefire PRIVATE netcdf netcdf_c++4)
    endif()
endif()

# ----------------------------------
# Shared Library
# ----------------------------------
add_library(forefireL SHARED ${SRC_FILES})
if(DEFINED NETCDF_STATIC_LIBS)
    if(MPI_FOUND)
        target_link_libraries(forefireL PRIVATE ${NETCDF_STATIC_LIBS} ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefireL PRIVATE ${NETCDF_STATIC_LIBS})
    endif()
else()
    # Non-static linking case
    if(MPI_FOUND)
        target_link_libraries(forefireL PRIVATE ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefireL PRIVATE netcdf netcdf_c++4)
    endif()
endif()

# ----------------------------------
# Test Executable
# ----------------------------------
set(TEST_MAIN tools/runANN/ANNTest.cpp)
add_executable(ANN_test ${TEST_MAIN})
if(DEFINED NETCDF_STATIC_LIBS)
    if(MPI_FOUND)
        target_link_libraries(ANN_test PRIVATE ${NETCDF_STATIC_LIBS} ${MPI_LIBRARIES})
    else()
        target_link_libraries(ANN_test PRIVATE ${NETCDF_STATIC_LIBS})
    endif()
else()
    # Non-static linking case
    if(MPI_FOUND)
        target_link_libraries(ANN_test PRIVATE ${MPI_LIBRARIES})
    else()
        target_link_libraries(ANN_test PRIVATE netcdf netcdf_c++4)
    endif()
endif()
