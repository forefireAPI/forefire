cmake_minimum_required(VERSION 3.10)
project(forefire VERSION 1.0)

# ----------------------------------
# Set C++ Standard
# ----------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------
# MPI Detection
# ----------------------------------
if(MPI_C_COMPILER AND MPI_CXX_COMPILER)
    message(STATUS "Using manually specified MPI compilers:")
    message(STATUS "  MPI_C_COMPILER: ${MPI_C_COMPILER}")
    message(STATUS "  MPI_CXX_COMPILER: ${MPI_CXX_COMPILER}")
    set(CMAKE_C_COMPILER "${MPI_C_COMPILER}")
    set(CMAKE_CXX_COMPILER "${MPI_CXX_COMPILER}")

    if(MPI_C_LIBRARIES)
        link_directories(${MPI_C_LIBRARIES})
    endif()

    if(MPI_CXX_LIBRARIES)
        link_directories(${MPI_CXX_LIBRARIES})
    endif()

    set(MPI_FOUND TRUE)
else()
    find_package(MPI)
    if(MPI_FOUND)
        message(STATUS "MPI found via find_package")
        include_directories(${MPI_INCLUDE_PATH})
    else()
        # Try to detect MPI installation from environment variables like MPI_HOME, MPI_ROOT, or any *MPI*ROOT*
        set(_mpi_env_vars)
        foreach(_var_name IN LISTS ENV)
            string(TOUPPER "${_var_name}" _upper_name)
            if(_upper_name MATCHES "MPI_HOME|MPI_ROOT|.*MPI.*ROOT.*")
                list(APPEND _mpi_env_vars "${_var_name}")
            endif()
        endforeach()

        set(_mpi_found_env_var "")
        foreach(_mpi_env_var ${_mpi_env_vars})
            if(DEFINED ENV{${_mpi_env_var}})
                set(_mpi_found_env_var "${_mpi_env_var}")
                break()
            endif()
        endforeach()

        if(_mpi_found_env_var)
            set(HEREMPI $ENV{${_mpi_found_env_var}})
            message(STATUS "Detected MPI environment variable: ${_mpi_found_env_var} = ${HEREMPI}")
            set(MPI_C_COMPILER "${HEREMPI}/bin/mpicc")
            set(MPI_CXX_COMPILER "${HEREMPI}/bin/mpicxx")
            set(MPI_C_LIBRARIES "${HEREMPI}/lib/release/libmpi.so")
            set(MPI_CXX_LIBRARIES "${HEREMPI}/lib/libmpicxx.so")
            set(MPI_INCLUDE_PATH "${HEREMPI}/include")
            set(CMAKE_C_COMPILER "${MPI_C_COMPILER}")
            set(CMAKE_CXX_COMPILER "${MPI_CXX_COMPILER}")
            link_directories("${HEREMPI}/lib" "${HEREMPI}/lib/release")
            include_directories("${MPI_INCLUDE_PATH}")
            message(STATUS "Set MPI_C_COMPILER: ${MPI_C_COMPILER}")
            message(STATUS "Set MPI_CXX_COMPILER: ${MPI_CXX_COMPILER}")
            message(STATUS "Set MPI_C_LIBRARIES: ${MPI_C_LIBRARIES}")
            message(STATUS "Set MPI_CXX_LIBRARIES: ${MPI_CXX_LIBRARIES}")
            message(STATUS "Set MPI_INCLUDE_PATH: ${MPI_INCLUDE_PATH}")
            set(MPI_FOUND TRUE)
        else()
            message(WARNING "No suitable MPI environment variable (e.g., MPI_HOME, MPI_ROOT, *MPI*ROOT*) found. Please specify MPI paths manually with -DMPI_C_COMPILER, -DMPI_CXX_COMPILER, -DMPI_C_LIBRARIES, -DMPI_CXX_LIBRARIES, -DMPI_INCLUDE_PATH.")
        endif()
    endif()
endif()

# ----------------------------------
# Check for MESONH and XYZ Environment Variables
# ----------------------------------
if(DEFINED ENV{SRC_MESONH} AND DEFINED ENV{XYZ} AND DEFINED ENV{FF_STATIC})
    message(STATUS "MESONH and XYZ environment variables are set. FF_STATIC too, static NetCDF.")
    set(MESONH $ENV{SRC_MESONH})
    set(XYZ $ENV{XYZ})
    file(GLOB NETCDF_DIR "${MESONH}/src/dir_obj${XYZ}/MASTER/NETCDF-*")
    if(NETCDF_DIR)
        list(GET NETCDF_DIR 0 NETCDF_HOME)
        message(STATUS "Detected NETCDF_HOME: ${NETCDF_HOME}")
    else()
        message(FATAL_ERROR "No NETCDF-* directory found under ${MESONH}/src/dir_obj${XYZ}/MASTER/")
    endif()
    include_directories(${NETCDF_HOME}/include)
    find_package(ZLIB REQUIRED)
    if(ZLIB_FOUND)
        message(STATUS "ZLIB found: ${ZLIB_LIBRARIES}")
    else()
        message(FATAL_ERROR "ZLIB not found")
    endif()
    set(NETCDF_LIB "${NETCDF_HOME}/lib64/libnetcdf.a")
    set(NETCDF_CXX_LIB "${NETCDF_HOME}/lib64/libnetcdf_c++4.a")
    set(HDF5_LIB "${NETCDF_HOME}/lib64/libhdf5.a")
    set(HDF5_HL_LIB "${NETCDF_HOME}/lib64/libhdf5_hl.a")
    set(SZIP_LIB "${NETCDF_HOME}/lib64/libsz.a")
    set(AEC_LIB "${NETCDF_HOME}/lib64/libaec.a")
    foreach(lib IN LISTS NETCDF_LIB NETCDF_CXX_LIB HDF5_LIB HDF5_HL_LIB SZIP_LIB AEC_LIB)
        if(NOT EXISTS ${lib})
            message(FATAL_ERROR "Required static library not found: ${lib}")
        endif()
    endforeach()
    set(NETCDF_STATIC_LIBS
        ${NETCDF_LIB}
        ${NETCDF_CXX_LIB}
        ${HDF5_HL_LIB}
        ${HDF5_LIB}
        ${SZIP_LIB}
        ${AEC_LIB}
        ${ZLIB_LIBRARIES}
    )
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
else()
    if(DEFINED ENV{NETCDF_HOME})
        set(NETCDF_HOME $ENV{NETCDF_HOME})
        include_directories(${NETCDF_HOME}/include)
        link_directories(${NETCDF_HOME}/lib)
        message(STATUS "NETCDF_HOME set to ${NETCDF_HOME}")
    else()
        message(STATUS "NETCDF_HOME not set. Attempting default path or system library paths.")
    endif()
    if(MPI_FOUND)
        link_libraries(netcdf netcdf_c++4 ${MPI_LIBRARIES})
    else()
        link_libraries(netcdf netcdf_c++4)
    endif()
endif()

# ----------------------------------
# Compiler Flags
# ----------------------------------
if(MPI_FOUND)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DMPI_COUPLING")
else()
    # uncoment if want to debug
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -fomit-frame-pointer -finline-functions -march=native -funroll-loops -ftree-vectorize -fno-strict-aliasing")
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
# Source Files
# ----------------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# ----------------------------------
# Shared Library Target
# ----------------------------------
add_library(forefireL SHARED ${SRC_FILES})
if(DEFINED NETCDF_STATIC_LIBS)
    if(MPI_FOUND)
        target_link_libraries(forefireL PRIVATE ${NETCDF_STATIC_LIBS} ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefireL PRIVATE ${NETCDF_STATIC_LIBS})
    endif()
else()
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
    if(MPI_FOUND)
        target_link_libraries(ANN_test PRIVATE ${MPI_LIBRARIES})
    else()
        target_link_libraries(ANN_test PRIVATE netcdf netcdf_c++4)
    endif()
endif()

# ----------------------------------
# Main Project Executable
# ----------------------------------
set(INTERPRETER app/forefire/ForeFire.cpp app/forefire/AdvancedLineEditor.cpp)
add_executable(forefire ${INTERPRETER})
if(DEFINED NETCDF_STATIC_LIBS)
    if(MPI_FOUND)
        target_link_libraries(forefire PRIVATE forefireL ${NETCDF_STATIC_LIBS} ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefire PRIVATE forefireL ${NETCDF_STATIC_LIBS})
    endif()
else()
    if(MPI_FOUND)
        target_link_libraries(forefire PRIVATE forefireL ${MPI_LIBRARIES})
    else()
        target_link_libraries(forefire PRIVATE forefireL netcdf netcdf_c++4)
    endif()
endif()

# ----------------------------------
# CPack Configuration for DMG (macOS)
# ----------------------------------
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "forefire")
set(CPACK_PACKAGE_VENDOR "CNRS - University of Corsica")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Forefire - a wildfire solver")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")  # Adjust as necessary

# Specify installation directories
install(TARGETS forefire forefireL ANN_test
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# For macOS DMG packaging using the DragNDrop generator:
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "forefire_${PROJECT_VERSION}")
    # Optionally, customize DMG settings (background, icon, etc.)
endif()

include(CPack)

# ----------------------------------
# LFS Data Integrity Check
# ----------------------------------
message(STATUS "Running Git LFS data integrity check...")

execute_process(
    COMMAND bash tools/check_all_lfs.bash
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE LFS_CHECK_RESULT
    OUTPUT_VARIABLE LFS_CHECK_OUTPUT
    ERROR_VARIABLE  LFS_CHECK_ERROR
)

if(LFS_CHECK_RESULT EQUAL 0)
    message(STATUS "✅ LFS data check passed successfully.")
else()
    message(WARNING "
************************************************************
⚠️  WARNING: Git LFS data integrity check failed!
You will NOT be able to run built-in examples until LFS data
files are correctly downloaded or restored.
------------------------------------------------------------
Command output:
${LFS_CHECK_OUTPUT}
${LFS_CHECK_ERROR}
************************************************************
")
endif()