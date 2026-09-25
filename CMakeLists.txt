cmake_minimum_required(VERSION 3.15)

# ----------------------------------
# Version (single source of truth: src/include/Version.h)
# ----------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/include/Version.h" _ff_version_header)
string(REGEX MATCH "ff_version[ \t]*=[ \t]*\"v?([0-9]+\\.[0-9]+\\.[0-9]+)\"" _ff_version_match "${_ff_version_header}")
if(NOT _ff_version_match)
    message(FATAL_ERROR "Could not parse ff_version from src/include/Version.h")
endif()
set(FOREFIRE_VERSION "${CMAKE_MATCH_1}")

project(forefire VERSION ${FOREFIRE_VERSION})
message(STATUS "ForeFire version: ${FOREFIRE_VERSION}")

# ----------------------------------
# Build Options
# ----------------------------------
# `SKBUILD` is defined when CMake is driven by scikit-build-core, i.e. when a
# Python wheel is being built. Wheels have to run on machines other than the
# builder, so they default to no `-march=native` (which would otherwise crash
# with SIGILL on older CPUs) and no MPI, since auto-detected MPI support is a
# build-environment accident rather than a user choice (see issue #102).
# Every default below can still be overridden explicitly, through
# `tool.scikit-build.cmake.define` or -D on the command line.
if(DEFINED SKBUILD)
    set(_ff_wheel_build ON)
    set(_ff_default_mpi OFF)
    set(_ff_default_native OFF)
    set(_ff_default_python ON)
    set(_ff_default_static ON)
    set(_ff_default_tools OFF)
    set(_ff_default_lfs OFF)
    set(_ff_default_tests OFF)
else()
    set(_ff_wheel_build OFF)
    set(_ff_default_mpi ON)
    set(_ff_default_native ON)
    set(_ff_default_python OFF)
    set(_ff_default_static OFF)
    set(_ff_default_tools ON)
    set(_ff_default_lfs ON)
    set(_ff_default_tests ON)
endif()

option(FOREFIRE_ENABLE_MPI "Enable MPI coupling when MPI is available" ${_ff_default_mpi})
option(FOREFIRE_NATIVE_ARCH "Optimise for the build machine's CPU (-march=native)" ${_ff_default_native})
option(FOREFIRE_BUILD_PYTHON "Build the pyforefire Python extension module" ${_ff_default_python})
option(FOREFIRE_STATIC_CORE "Build the ForeFire core as a static library" ${_ff_default_static})
option(FOREFIRE_BUILD_TOOLS "Build the ANN_test helper executable" ${_ff_default_tools})
option(FOREFIRE_CHECK_LFS "Run the Git LFS data integrity check at configure time" ${_ff_default_lfs})
option(FOREFIRE_BUILD_TESTS "Build the C++ unit tests, registered with CTest" ${_ff_default_tests})
option(FOREFIRE_ENABLE_WARNINGS "Compile ForeFire's own sources with -Wall -Wextra" ON)
option(FOREFIRE_WARNINGS_AS_ERRORS "Fail the build on a compiler warning" OFF)
set(FOREFIRE_SANITIZE "" CACHE STRING
    "Sanitizers to build with, passed to -fsanitize= (e.g. address, undefined, address,undefined)")

# ----------------------------------
# Set C++ Standard
# ----------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------
# MPI Detection
# ----------------------------------
if(NOT FOREFIRE_ENABLE_MPI)
    message(STATUS "MPI support disabled (FOREFIRE_ENABLE_MPI=OFF)")
    set(MPI_FOUND FALSE)
elseif(MPI_C_COMPILER AND MPI_CXX_COMPILER)
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
# NetCDF Dependency
# ----------------------------------
# Everything that needs NetCDF links against this interface target, so the
# detection logic below lives in exactly one place.
add_library(forefire_netcdf INTERFACE)

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
    find_package(ZLIB REQUIRED)
    message(STATUS "ZLIB found: ${ZLIB_LIBRARIES}")
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
    target_include_directories(forefire_netcdf SYSTEM INTERFACE "${NETCDF_HOME}/include")
    target_link_libraries(forefire_netcdf INTERFACE
        ${NETCDF_LIB}
        ${NETCDF_CXX_LIB}
        ${HDF5_HL_LIB}
        ${HDF5_LIB}
        ${SZIP_LIB}
        ${AEC_LIB}
        ${ZLIB_LIBRARIES}
    )
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
else()
    # NETCDF_HOME / NETCDF_CXX_HOME may come from the environment or from
    # -DNETCDF_HOME=... on the command line; both are only hints, system paths
    # are searched as well.
    if(NOT NETCDF_HOME AND DEFINED ENV{NETCDF_HOME})
        set(NETCDF_HOME $ENV{NETCDF_HOME})
    endif()
    if(NOT NETCDF_CXX_HOME AND DEFINED ENV{NETCDF_CXX_HOME})
        set(NETCDF_CXX_HOME $ENV{NETCDF_CXX_HOME})
    endif()
    set(_netcdf_hints ${NETCDF_HOME} ${NETCDF_CXX_HOME})

    find_path(NETCDF_INCLUDE_DIR NAMES netcdf.h HINTS ${_netcdf_hints} PATH_SUFFIXES include)
    find_library(NETCDF_LIBRARY NAMES netcdf HINTS ${_netcdf_hints} PATH_SUFFIXES lib lib64)
    # The legacy C++4 API ships the umbrella header `netcdf` (no extension)
    # next to ncFile.h, and is named netcdf_c++4 everywhere except Homebrew,
    # which calls it netcdf-cxx4 (older bottles: netcdf-cxx).
    find_path(NETCDF_CXX_INCLUDE_DIR NAMES ncFile.h HINTS ${_netcdf_hints} PATH_SUFFIXES include)
    find_library(NETCDF_CXX_LIBRARY NAMES netcdf_c++4 netcdf-cxx4 netcdf-cxx HINTS ${_netcdf_hints} PATH_SUFFIXES lib lib64)

    foreach(_var NETCDF_INCLUDE_DIR NETCDF_LIBRARY NETCDF_CXX_INCLUDE_DIR NETCDF_CXX_LIBRARY)
        if(NOT ${_var})
            message(FATAL_ERROR
                "NetCDF not found (${_var} is missing).\n"
                "ForeFire needs both the NetCDF C library and the legacy C++4 API:\n"
                "  Debian/Ubuntu: apt install libnetcdf-dev libnetcdf-c++4-dev\n"
                "  Fedora/RHEL:   dnf install netcdf-devel netcdf-cxx4-devel\n"
                "  macOS:         brew install netcdf netcdf-cxx\n"
                "Set -DNETCDF_HOME=... (and -DNETCDF_CXX_HOME=... if the C++ API "
                "lives elsewhere) to point at a custom installation.")
        endif()
    endforeach()

    message(STATUS "NetCDF C:   ${NETCDF_LIBRARY} (headers: ${NETCDF_INCLUDE_DIR})")
    message(STATUS "NetCDF C++: ${NETCDF_CXX_LIBRARY} (headers: ${NETCDF_CXX_INCLUDE_DIR})")
    # SYSTEM: the NetCDF C++4 headers trip -Wunused-variable, and that is not a
    # warning anyone here can act on.
    target_include_directories(forefire_netcdf SYSTEM INTERFACE ${NETCDF_CXX_INCLUDE_DIR} ${NETCDF_INCLUDE_DIR})
    target_link_libraries(forefire_netcdf INTERFACE ${NETCDF_CXX_LIBRARY} ${NETCDF_LIBRARY})
endif()

if(MPI_FOUND)
    target_link_libraries(forefire_netcdf INTERFACE ${MPI_LIBRARIES})
endif()

# ----------------------------------
# Compiler Flags
# ----------------------------------
if(MPI_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DMPI_COUPLING")
elseif(FOREFIRE_SANITIZE)
    # A sanitizer build wants frame pointers and debug info; the release set
    # below strips both, and -flto makes the reports harder to read besides.
    # -O1 keeps the suites quick enough to run in CI.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1 -fno-omit-frame-pointer")
else()
    # uncomment if want to debug
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -flto -fomit-frame-pointer -finline-functions -funroll-loops -ftree-vectorize -fno-strict-aliasing")
    if(FOREFIRE_NATIVE_ARCH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
endif()

# Sanitizers have to reach the link line as well as the compile line, and the
# shared library too: the core is where the allocations happen.
if(FOREFIRE_SANITIZE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${FOREFIRE_SANITIZE}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${FOREFIRE_SANITIZE}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=${FOREFIRE_SANITIZE}")
    message(STATUS "Sanitizers enabled: -fsanitize=${FOREFIRE_SANITIZE}")
endif()

# Warnings are a target property rather than a global flag on purpose: they
# should apply to code this project writes, not to pybind11's headers or to
# whatever a consumer compiles against the installed library.
set(_ff_warning_flags "")
if(FOREFIRE_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        list(APPEND _ff_warning_flags -Wall -Wextra)
    elseif(MSVC)
        list(APPEND _ff_warning_flags /W4)
    endif()
endif()
if(FOREFIRE_WARNINGS_AS_ERRORS AND _ff_warning_flags)
    if(MSVC)
        list(APPEND _ff_warning_flags /WX)
    else()
        list(APPEND _ff_warning_flags -Werror)
    endif()
endif()

# ----------------------------------
# Output Directories
# ----------------------------------
# Wheel builds must not write into the source tree; CMake's defaults (relative
# to the build directory) are what scikit-build-core expects.
if(NOT _ff_wheel_build)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
    message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif()
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# ----------------------------------
# Source Files
# ----------------------------------
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# ----------------------------------
# Core Library Target
# ----------------------------------
if(FOREFIRE_STATIC_CORE)
    # OBJECT, not STATIC: every propagation and flux model registers itself
    # through a static initialiser that nothing references by name (see
    # FireDomain::registerPropagationModelInstantiator). Out of a static
    # archive the linker discards those objects and the models silently
    # disappear at runtime; object libraries are always linked whole.
    add_library(forefireL OBJECT ${SRC_FILES})
    set_target_properties(forefireL PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
    add_library(forefireL SHARED ${SRC_FILES})
endif()
target_include_directories(forefireL PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/include")
# SYSTEM and PRIVATE: vendored headers are ours to use but not ours to fix, so
# the warning flags below must not apply to them, and nothing outside this
# library includes them.
target_include_directories(forefireL SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
# Threads is explicit because a static core does not drag libpthread in for
# its consumers the way the shared library does.
find_package(Threads REQUIRED)
target_link_libraries(forefireL PUBLIC forefire_netcdf Threads::Threads)
if(_ff_warning_flags)
    target_compile_options(forefireL PRIVATE ${_ff_warning_flags})
endif()

# ----------------------------------
# Test Executable
# ----------------------------------
if(FOREFIRE_BUILD_TOOLS)
    add_executable(ANN_test tools/runANN/ANNTest.cpp)
    target_link_libraries(ANN_test PRIVATE forefire_netcdf)
endif()

# ----------------------------------
# Main Project Executable
# ----------------------------------
add_executable(forefire app/forefire/ForeFire.cpp app/forefire/AdvancedLineEditor.cpp)
target_link_libraries(forefire PRIVATE forefireL)
if(_ff_warning_flags)
    target_compile_options(forefire PRIVATE ${_ff_warning_flags})
endif()

# ----------------------------------
# Unit Tests
# ----------------------------------
if(FOREFIRE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests/unit)
endif()

# ----------------------------------
# Python Extension Module
# ----------------------------------
if(FOREFIRE_BUILD_PYTHON)
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
    find_package(pybind11 CONFIG REQUIRED)

    pybind11_add_module(_pyforefire bindings/python/src/pyforefire/_pyforefire.cpp)
    target_link_libraries(_pyforefire PRIVATE forefireL)
    install(TARGETS _pyforefire DESTINATION pyforefire)

    # Ship the command line interpreter so that `pip install forefire` also
    # provides a working `forefire` command.
    #
    # It goes *inside* the package, next to _pyforefire, rather than into
    # SKBUILD_SCRIPTS_DIR. auditwheel and delocate point a relocated binary at
    # the vendored libraries using a path relative to the binary itself, and
    # only a binary inside the package keeps that path valid: pip installs
    # `.data/scripts/` into `<env>/bin` but the package into
    # `<env>/lib/pythonX.Y/site-packages/`, so a reference from one to the
    # other cannot survive installation. delocate produced exactly that broken
    # path on macOS. `pyforefire._cli` is the entry point that execs it.
    if(_ff_wheel_build)
        install(TARGETS forefire RUNTIME DESTINATION pyforefire)
    endif()
endif()

# ----------------------------------
# CPack Configuration for DMG (macOS)
# ----------------------------------
if(NOT _ff_wheel_build)
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
    install(TARGETS forefire RUNTIME DESTINATION bin)
    if(NOT FOREFIRE_STATIC_CORE)
        # With a static core there is no libforefireL to install: it is linked
        # straight into the executables.
        install(TARGETS forefireL
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib)
    endif()
    if(FOREFIRE_BUILD_TOOLS)
        install(TARGETS ANN_test RUNTIME DESTINATION bin)
    endif()

    # For macOS DMG packaging using the DragNDrop generator:
    if(APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "forefire_${PROJECT_VERSION}")
        # Optionally, customize DMG settings (background, icon, etc.)
    endif()

    include(CPack)
endif()

# ----------------------------------
# LFS Data Integrity Check
# ----------------------------------
if(FOREFIRE_CHECK_LFS)
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
endif()
