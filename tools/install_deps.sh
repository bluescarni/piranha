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
export deps_dir=$HOME/local
bash miniconda.sh -b -p $HOME/miniconda
export PATH="$HOME/miniconda/bin:$PATH"
conda config --add channels conda-forge --force

conda_pkgs="gmp mpfr boost>=1.58 cmake>=3.2 bzip2 zlib msgpack-c doxygen"

if [[ "${BUILD_TYPE}" == "Python2" ]]; then
    conda_pkgs="$conda_pkgs python=2.7 numpy mpmath sphinx sphinx-bootstrap-theme"
elif [[ "${BUILD_TYPE}" == "Python3" ]]; then
    conda_pkgs="$conda_pkgs python=3.5 numpy mpmath sphinx sphinx-bootstrap-theme"
fi

conda create -q -p $deps_dir -y $conda_pkgs
source activate $deps_dir

set +e
set +x
