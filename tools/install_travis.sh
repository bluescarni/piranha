#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

export PATH="$HOME/miniconda/bin:$PATH"

if [[ "${PIRANHA_BUILD}" == "DebugGCC48" ]]; then
    CXX=g++-4.8 CC=gcc-4.8 cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DCMAKE_CXX_FLAGS_DEBUG="-fsanitize=address -g0 -Os" -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
    make VERBOSE=1;
    ctest -E "thread|memory" -V;
elif [[ "${PIRANHA_BUILD}" == "DebugGCC7" ]]; then
    CXX=g++-7 CC=gcc-7 cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DCMAKE_CXX_FLAGS_DEBUG="-g0 -Os -fconcepts" -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
elif [[ "${PIRANHA_BUILD}" == "DebugClang39" ]]; then
    CXX=clang++-3.9 CC=clang-3.9 cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} -DQuadmath_INCLUDE_DIR=/usr/lib/gcc/x86_64-linux-gnu/7/include -DQuadmath_LIBRARY=/usr/lib/gcc/x86_64-linux-gnu/7/libquadmath.so ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
elif [[ "${PIRANHA_BUILD}" == "OSXDebug" ]]; then
    CXX=clang++ CC=clang cmake -DCMAKE_INSTALL_PREFIX=$deps_dir -DCMAKE_PREFIX_PATH=$deps_dir -DCMAKE_BUILD_TYPE=Debug -DPIRANHA_BUILD_TESTS=yes -DPIRANHA_WITH_BOOST_S11N=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_ZLIB=yes -DPIRANHA_TEST_NSPLIT=${TEST_NSPLIT} -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
    make VERBOSE=1;
    ctest -E "thread" -V;
fi

set +e
set +x
