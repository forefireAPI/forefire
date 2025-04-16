FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    libnetcdf-c++4-dev \
    cmake

WORKDIR /forefire
ENV FOREFIREHOME=/forefire

# we could only copy src, cmakelists.txt and cmake-build.sh
COPY . .

# install forefire
RUN sh cmake-build.sh

# add executable to PATH
RUN cp /forefire/bin/forefire /bin
