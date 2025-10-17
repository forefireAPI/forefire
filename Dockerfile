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

# Copy build configuration first (rarely changes)
COPY CMakeLists.txt cmake-build.sh LICENSE ./

# Copy source code and applications
COPY src/ ./src/
COPY app/ ./app/
COPY tools/runANN/ ./tools/runANN/

# Build and install the ForeFire C++ library
RUN sh cmake-build.sh

# Add the main forefire executable to the PATH
RUN cp /forefire/bin/forefire /usr/local/bin/

# Use pip to install the Python bindings
COPY bindings/ ./bindings/
RUN pip3 install ./bindings/python

# Copy everything else (changes most frequently - docs, tests, etc.)
COPY . .

# WE COULD USE A CUSTOM ENTRYPOINT SCRIPT TO START THE HTTP SERVER AND RUN THE DEMO SIMULATION
# COPY tools/devops/entrypoint.sh /usr/local/bin/entrypoint.sh
# RUN chmod +x /usr/local/bin/entrypoint.sh
# ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# Set the entrypoint to bash for interactive sessions
CMD ["bash"]