#!/usr/bin/env bash
#
# Install the NetCDF stack inside a manylinux_2_28 container, for cibuildwheel.
#
# HDF5 comes from EPEL. The NetCDF C library and the C++4 API are both built
# from source into /usr/local, where auditwheel finds them and vendors them
# into the wheel.
#
# NetCDF C is built rather than installed from EPEL so that DAP and HDF4 can
# be switched off. EPEL enables both, and auditwheel then has to vendor their
# entire dependency closure: DAP pulls libcurl, which drags in OpenSSL,
# Kerberos, LDAP, libssh and a dozen more, about 10 MB across 22 libraries
# that ForeFire never calls. macOS avoids this only because the system
# provides libcurl. HDF4 adds another 1.4 MB, and Homebrew's NetCDF does not
# enable it, so turning it off here also makes the two platforms agree.
set -euo pipefail

NETCDF_C_VERSION="${NETCDF_C_VERSION:-4.9.3}"
NETCDF_C_SHA256="a474149844e6144566673facf097fea253dc843c37bc0a7d3de047dc8adda5dd"
NETCDF_CXX4_VERSION="${NETCDF_CXX4_VERSION:-4.3.1}"
NETCDF_CXX4_SHA256="6a1189a181eed043b5859e15d5c080c30d0e107406fbb212c8fb9814e90f3445"
PREFIX="${PREFIX:-/usr/local}"

if [[ -f "${PREFIX}/include/netcdf" ]]; then
    echo "NetCDF C++4 already installed in ${PREFIX}, nothing to do."
    exit 0
fi

echo "::group::Installing HDF5 from EPEL"
dnf install -y epel-release
dnf install -y hdf5-devel
echo "::endgroup::"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT
cd "${workdir}"

fetch() {
    local url="$1" tarball="$2" sha="$3"
    curl -fsSL -o "${tarball}" "${url}"
    echo "${sha}  ${tarball}" | sha256sum --check --strict
    tar xzf "${tarball}"
}

echo "::group::Building netcdf-c ${NETCDF_C_VERSION}"
fetch "https://downloads.unidata.ucar.edu/netcdf-c/${NETCDF_C_VERSION}/netcdf-c-${NETCDF_C_VERSION}.tar.gz" \
      "netcdf-c-${NETCDF_C_VERSION}.tar.gz" "${NETCDF_C_SHA256}"
cd "netcdf-c-${NETCDF_C_VERSION}"

# --disable-dap and --disable-byterange both have to go: byterange is a second
# remote-access path that links libcurl on its own. --disable-hdf4 drops
# libmfhdf, libdf, libjpeg and libtirpc. ForeFire reads local .nc and PGD
# files, so none of these are reachable from it.
./configure \
    --prefix="${PREFIX}" \
    --enable-shared \
    --disable-static \
    --disable-dap \
    --disable-byterange \
    --disable-hdf4 \
    --disable-libxml2 \
    --disable-examples \
    --disable-testsets
make -j "$(nproc)"
make install
ldconfig
cd "${workdir}"
echo "::endgroup::"

echo "::group::Building netcdf-cxx4 ${NETCDF_CXX4_VERSION}"
fetch "https://downloads.unidata.ucar.edu/netcdf-cxx/${NETCDF_CXX4_VERSION}/netcdf-cxx4-${NETCDF_CXX4_VERSION}.tar.gz" \
      "netcdf-cxx4-${NETCDF_CXX4_VERSION}.tar.gz" "${NETCDF_CXX4_SHA256}"
cd "netcdf-cxx4-${NETCDF_CXX4_VERSION}"

# nc-config from the netcdf-c just installed has to win over anything else on
# PATH, so that the C++ API links the DAP-free build.
export PATH="${PREFIX}/bin:${PATH}"
export PKG_CONFIG_PATH="${PREFIX}/lib:${PREFIX}/lib64:${PKG_CONFIG_PATH:-}"

# Autotools rather than the bundled CMake build: netcdf-cxx4 4.3.1 declares
# `cmake_minimum_required(VERSION 2.8.12)`, and CMake 4 refuses anything below
# 3.5. configure picks up the NetCDF built above through nc-config.
./configure \
    --prefix="${PREFIX}" \
    --enable-shared \
    --disable-static

# SUBDIRS is overridden to skip examples/. They call nc_set_log_level, which
# netcdf-c only exports when built --enable-logging, so they fail to link
# against our build. 4.3.1 has no --disable-examples. The library, all the
# headers including the <netcdf> umbrella, and the pkg-config file are still
# installed.
make -j "$(nproc)" SUBDIRS=cxx4
make install SUBDIRS=cxx4
ldconfig
echo "::endgroup::"

echo "NetCDF C++4 installed:"
ls -l "${PREFIX}"/lib*/libnetcdf_c++4.so* "${PREFIX}/include/netcdf"

# Fail loudly here rather than shipping a wheel that quietly regained the
# dependency this script exists to avoid.
if ldd "${PREFIX}"/lib*/libnetcdf.so | grep -q libcurl; then
    echo "ERROR: libnetcdf still links libcurl; DAP or byterange is enabled." >&2
    exit 1
fi
echo "Confirmed: libnetcdf does not link libcurl."
