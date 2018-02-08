#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

export PATH="$HOME/miniconda/bin:$PATH"

if [[ "${PIRANHA_BUILD}" == "Documentation" ]]; then
    cd ../doc/sphinx;
    pip install sphinx requests[security] guzzle_sphinx_theme;
    export SPHINX_OUTPUT=`make html linkcheck 2>&1 >/dev/null`;
    if [[ "${SPHINX_OUTPUT}" != "" ]]; then
        echo "Sphinx encountered some problem:";
        echo "${SPHINX_OUTPUT}";
        exit 1;
    fi
elif [[ "${PIRANHA_BUILD}" == "ReleaseGCC48" ]]; then
    CXX=g++-4.8 CC=gcc-4.8 cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes ../;
    make install VERBOSE=1;

    # Check that all headers are really installed.
    for f in `find $deps_dir/include/piranha -iname '*.hpp'`; do basename $f; done|grep -v config.hpp|sort  > inst_list.txt
    for f in `find ../include/piranha -iname '*.hpp'`; do basename $f; done|sort > src_list.txt
    export INSTALL_DIFF=`diff -Nru inst_list.txt src_list.txt`
    if [[ "${INSTALL_DIFF}" != "" ]]; then
        echo "Not all headers were installed. The diff is:";
        echo "--------";
        echo "${INSTALL_DIFF}";
        echo "--------";
        echo "Aborting.";
        exit 1;
    fi
    if [[ `find $deps_dir/include/piranha -iname config.hpp` == "" ]]; then
        echo "The config.hpp file was not installed, aborting.";
        exit 1;
    fi

    # Test the CMake export installation.
    cd ../tools/sample_project;
    mkdir build;
    cd build;
    cmake ../ -DCMAKE_PREFIX_PATH=$deps_dir;
    make;
    ./main;
elif [[ "${PIRANHA_BUILD}" == "DebugGCC48" ]]; then
    CXX=g++-4.8 CC=gcc-4.8 cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DCMAKE_CXX_FLAGS_DEBUG="-fsanitize=address -g0 -Os" -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
    make VERBOSE=1;
    ctest -E "thread|memory" -V;
elif [[ "${PIRANHA_BUILD}" == "DebugGCC7" ]]; then
    CXX=g++-7 CC=gcc-7 cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DCMAKE_CXX_FLAGS_DEBUG="-Og --coverage -fconcepts" -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
    bash <(curl -s https://codecov.io/bash) -x gcov-7
elif [[ "${PIRANHA_BUILD}" == "DebugClang39" ]]; then
    CXX=clang++-3.9 CC=clang-3.9 cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} -DQuadmath_INCLUDE_DIR=/usr/lib/gcc/x86_64-linux-gnu/7/include -DQuadmath_LIBRARY=/usr/lib/gcc/x86_64-linux-gnu/7/libquadmath.so ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
elif [[ "${PIRANHA_BUILD}" == "OSXDebug" ]]; then
    CXX=clang++ CC=clang cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} -DCMAKE_CXX_FLAGS_DEBUG="-D_LIBCPP_DEBUG=1" ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
fi

set +e
set +x
