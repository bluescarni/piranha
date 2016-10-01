#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

wget https://github.com/msgpack/msgpack-c/releases/download/cpp-2.0.0/msgpack-2.0.0.tar.gz
tar xzf msgpack-2.0.0.tar.gz
cd msgpack-2.0.0

mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DMSGPACK_BUILD_EXAMPLES=FALSE -DMSGPACK_CXX11=TRUE -DMSGPACK_ENABLE_CXX=TRUE -DMSGPACK_ENABLE_SHARED=FALSE -DCMAKE_INSTALL_PREFIX=/home/travis/.local
make
make install
