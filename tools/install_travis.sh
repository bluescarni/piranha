#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
    if [[ "${PIRANHA_COMPILER}" == "gcc" ]]; then
        cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=yes -DCMAKE_CXX_FLAGS="-fsanitize=address -Os" -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DPIRANHA_TEST_SPLIT=yes -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
        make VERBOSE=1;
        ctest -E "thread|memory" -V;
    elif [[ "${PIRANHA_COMPILER}" == "clang" ]]; then
        cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=yes -DPIRANHA_TEST_SPLIT=yes -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
        make VERBOSE=1;
        ctest -E "thread" -V;
    fi
elif [[ "${BUILD_TYPE}" == "Coverage" ]]; then
        cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=yes -DCMAKE_CXX_FLAGS="-Og --coverage" -DPIRANHA_TEST_SPLIT=yes -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
        make VERBOSE=1;
        ctest -E "thread" -V;
        wget https://codecov.io/bash;
        # Run gcov manually, then remove all coverage information pertaining
        # include files in /usr/include.
        find ./ -type f -name '*.gcno' -not -path CMakeFiles -exec "${GCOV_EXECUTABLE}" -pb {} +
        find ./ -iname '*usr*include*.gcov' | xargs rm;
        bash bash -p ./tests -X gcov -g CMakeFiles || echo "Codecov did not collect coverage reports";
elif [[ "${BUILD_TYPE}" == "Release" ]]; then
    cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=yes ../;
    make VERBOSE=1;
    ctest -E "gastineau|pearce2_unpacked" -V;
    # Do the release here.
    export PIRANHA_RELEASE_VERSION=`echo "${TRAVIS_TAG}"|grep -E 'v[0-9]+\.[0-9]+.*'|cut -c 2-`
    if [[ "${PIRANHA_RELEASE_VERSION}" != "" ]]; then
      echo "Creating new piranha release: ${PIRANHA_RELEASE_VERSION}"
      set +x
      curl -s --data '{"tag_name": "'"${TRAVIS_TAG}"'","name": "piranha-'"${PIRANHA_RELEASE_VERSION}"'","body": "Release of version '"${PIRANHA_RELEASE_VERSION}"'.","prerelease": true}' "https://api.github.com/repos/bluescarni/piranha/releases?access_token=${GH_RELEASE_TOKEN}" 2>&1 > /dev/null
      set -x
    fi
elif [[ "${BUILD_TYPE}" == "Python2" ]]; then
    cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Debug -DBUILD_PYRANHA=yes -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DCMAKE_CXX_FLAGS=-Os -DCMAKE_INSTALL_PREFIX=/home/travis/.local -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_python-py27.so -DBoost_PYTHON_LIBRARY_DEBUG=/usr/lib/x86_64-linux-gnu/libboost_python-py27.so -DPYTHON_EXECUTABLE=/usr/bin/python2 ../;
    make VERBOSE=1;
    make install;
    # Install mpmath via pip.
    pip install --user mpmath;
    python -c "import pyranha.test; pyranha.test.run_test_suite()";
    # Install sphinx and the rtd theme.
    pip install --user sphinx
    # Workaround? This should be amongst the deps of sphinx but travis complains.
    pip install --user utils
    pip install --user sphinx_bootstrap_theme
    export PATH=$PATH:/home/travis/.local/bin
    cd ../doc/sphinx;
    export SPHINX_OUTPUT=`make html 2>&1 >/dev/null`;
    if [[ "${SPHINX_OUTPUT}" != "" ]]; then
        echo "Sphinx encountered some problem:";
        echo "${SPHINX_OUTPUT}";
        exit 1;
    fi
    echo "Sphinx ran successfully";
    if [[ "${TRAVIS_PULL_REQUEST}" != "false" ]]; then
        echo "Testing a pull request, the generated documentation will not be uploaded.";
        exit 0;
    fi
    if [[ "${TRAVIS_BRANCH}" != "master" ]]; then
        echo "Branch is not master, the generated documentation will not be uploaded.";
        exit 0;
    fi
    # Move out the resulting documentation.
    mv _build/html /home/travis/sphinx;
    # Checkout a new copy of the repo, for pushing to gh-pages.
    cd ../../../;
    git config --global push.default simple
    git config --global user.name "Travis CI"
    git config --global user.email "bluescarni@gmail.com"
    set +x
    git clone "https://${GH_TOKEN}@github.com/bluescarni/piranha.git" piranha_gh_pages -q
    set -x
    cd piranha_gh_pages
    git checkout -b gh-pages --track origin/gh-pages;
    git rm -fr sphinx;
    mv /home/travis/sphinx .;
    git add sphinx;
    # We assume here that a failure in commit means that there's nothing
    # to commit.
    git commit -m "Update Sphinx documentation, commit ${TRAVIS_COMMIT} [skip ci]." || exit 0
    PUSH_COUNTER=0
    until git push -q
    do
        git pull -q
        PUSH_COUNTER=$((PUSH_COUNTER + 1))
        if [ "$PUSH_COUNTER" -gt 3 ]; then
            echo "Push failed, aborting."
            exit 1
        fi
    done
elif [[ "${BUILD_TYPE}" == "Python3" ]]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_PYRANHA=yes -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DCMAKE_CXX_FLAGS=-Os -DCMAKE_INSTALL_PREFIX=/home/travis/.local -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_python-py32.so -DBoost_PYTHON_LIBRARY_DEBUG=/usr/lib/x86_64-linux-gnu/libboost_python-py32.so -DPYTHON_EXECUTABLE=/usr/bin/python3 ../;
    make VERBOSE=1;
    make install;
    # Install mpmath manually.
    wget "http://mpmath.org/files/mpmath-0.19.tar.gz";
    tar xzf mpmath-0.19.tar.gz;
    cd mpmath-0.19;
    python3 setup.py install --user;
    cd ..;
    python3 -c "import pyranha.test; pyranha.test.run_test_suite()";
elif [[ "${BUILD_TYPE}" == "Tutorial" ]]; then
    cmake -DPIRANHA_WITH_MSGPACK=yes -DPIRANHA_WITH_BZIP2=yes -DPIRANHA_WITH_ZLIB=yes -DMSGPACK-C_INCLUDE_DIR=/home/travis/.local/include -DCMAKE_BUILD_TYPE=Debug -DBUILD_TUTORIAL=yes ../;
    make VERBOSE=1;
    ctest -V;
elif [[ "${BUILD_TYPE}" == "Doxygen" ]]; then
    # Configure.
    cmake ../;
    # Install a recent version of Doxygen locally.
    wget "http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.12.src.tar.gz";
    tar xzf doxygen-1.8.12.src.tar.gz;
    cd doxygen-1.8.12;
    mkdir build;
    cd build;
    cmake -DCMAKE_INSTALL_PREFIX=/home/travis/.local -Duse_libclang=YES ../;
    make -j2;
    make install;
    # Now run it.
    cd ../../../doc/doxygen;
    export DOXYGEN_OUTPUT=`/home/travis/.local/bin/doxygen 2>&1 >/dev/null`;
    if [[ "${DOXYGEN_OUTPUT}" != "" ]]; then
        echo "Doxygen encountered some problem:";
        echo "${DOXYGEN_OUTPUT}";
        exit 1;
    fi
    echo "Doxygen ran successfully";
    if [[ "${TRAVIS_PULL_REQUEST}" != "false" ]]; then
        echo "Testing a pull request, the generated documentation will not be uploaded.";
        exit 0;
    fi
    if [[ "${TRAVIS_BRANCH}" != "master" ]]; then
        echo "Branch is not master, the generated documentation will not be uploaded.";
        exit 0;
    fi
    # Move out the resulting documentation.
    mv html /home/travis/doxygen;
    # Checkout a new copy of the repo, for pushing to gh-pages.
    cd ../../../;
    git config --global push.default simple
    git config --global user.name "Travis CI"
    git config --global user.email "bluescarni@gmail.com"
    set +x
    git clone "https://${GH_TOKEN}@github.com/bluescarni/piranha.git" piranha_gh_pages -q
    set -x
    cd piranha_gh_pages
    git checkout -b gh-pages --track origin/gh-pages;
    git rm -fr doxygen;
    mv /home/travis/doxygen .;
    git add doxygen;
    # We assume here that a failure in commit means that there's nothing
    # to commit.
    git commit -m "Update Doxygen documentation, commit ${TRAVIS_COMMIT} [skip ci]." || exit 0
    PUSH_COUNTER=0
    until git push -q
    do
        git pull -q
        PUSH_COUNTER=$((PUSH_COUNTER + 1))
        if [ "$PUSH_COUNTER" -gt 3 ]; then
            echo "Push failed, aborting."
            exit 1
        fi
    done
fi
