#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

if [[ "${TRAVIS_OS_NAME}" == "osx" ]]; then
    wget https://repo.continuum.io/miniconda/Miniconda2-latest-MacOSX-x86_64.sh -O miniconda.sh;
else
    wget https://repo.continuum.io/miniconda/Miniconda2-latest-Linux-x86_64.sh -O miniconda.sh;
fi
export deps_dir=$HOME/.local
bash miniconda.sh -b -p $deps_dir
export PATH="$deps_dir/bin:$PATH"
conda config --add channels conda-forge --force

conda_pkgs="gmp mpfr boost>=1.55 cmake>=3.0 bzip2 zlib"

if [[ "${BUILD_TYPE}" == "Python2" ]]; then
    conda_pkgs="$conda_pkgs python=2.7 numpy mpmath sphinx sphinx-bootstrap-theme"
elif [[ "${BUILD_TYPE}" == "Python3" ]]; then
    conda_pkgs="$conda_pkgs python=3.5 numpy mpmath sphinx sphinx-bootstrap-theme"
fi

conda install -y $conda_pkgs

wget https://github.com/msgpack/msgpack-c/releases/download/cpp-2.0.0/msgpack-2.0.0.tar.gz
tar xzf msgpack-2.0.0.tar.gz
cd msgpack-2.0.0

mkdir build
cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DMSGPACK_BUILD_EXAMPLES=FALSE -DMSGPACK_CXX11=TRUE -DMSGPACK_ENABLE_CXX=TRUE -DMSGPACK_ENABLE_SHARED=FALSE -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
cd ..
cd ..

if [[ "${BUILD_TYPE}" == "Doxygen" ]]; then
    # Install a recent version of Doxygen locally.
    wget "http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.12.src.tar.gz";
    tar xzf doxygen-1.8.12.src.tar.gz;
    cd doxygen-1.8.12;
    mkdir build;
    cd build;
    cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -Duse_libclang=YES ../;
    make -j2;
    make install;
fi

set +e
set +x
