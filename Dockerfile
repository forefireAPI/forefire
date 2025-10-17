FROM ubuntu:22.04

# Install build tools, ForeFire dependencies, AND Python environment
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    libnetcdf-c++4-dev \
    cmake \
    curl \
    python3 \
    python3-pip \
    python3-dev \
    git && \
    rm -rf /var/lib/apt/lists/*

# Install Python dependencies for testing
RUN pip3 install --no-cache-dir lxml xarray netCDF4

WORKDIR /forefire
ENV FOREFIREHOME=/forefire

# we could only copy src, cmakelists.txt and cmake-build.sh
COPY . .

# Build and install the ForeFire C++ library
RUN sh cmake-build.sh

# Add the main forefire executable to the PATH
RUN cp /forefire/bin/forefire /usr/local/bin/

# Use pip to install the Python bindings
RUN pip3 install ./bindings/python

# Set the entrypoint to bash for interactive sessions
CMD ["bash"]