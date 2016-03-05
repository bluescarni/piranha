#!/usr/bin/env bash

# Exit on error
set -e
# Echo each command
set -x

if [[ "${BUILD_TYPE}" == "Debug" ]]; then
    if [[ "${PIRANHA_COMPILER}" == "gcc" ]]; then
        cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address -Os" -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DBUILD_TUTORIAL=yes -DPIRANHA_TEST_SPLIT=yes -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
        make;
        ctest -E "thread|memory" -V;
    elif [[ "${PIRANHA_COMPILER}" == "clang" ]]; then
        cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TUTORIAL=yes -DPIRANHA_TEST_SPLIT=yes -DPIRANHA_TEST_SPLIT_NUM=${SPLIT_TEST_NUM} ../;
        make;
        ctest -E "thread" -V;
    fi
elif [[ "${BUILD_TYPE}" == "Release" ]]; then
    cmake -DCMAKE_BUILD_TYPE=Release ../;
    make;
    ctest -E "gastineau|perminov" -V;
elif [[ "${BUILD_TYPE}" == "Python2" ]]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_PYRANHA=yes -DBUILD_TESTS=no -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DCMAKE_CXX_FLAGS=-Os -DCMAKE_INSTALL_PREFIX=/home/travis/.local -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_python-py27.so -DBoost_PYTHON_LIBRARY_DEBUG=/usr/lib/x86_64-linux-gnu/libboost_python-py27.so -DPYTHON_EXECUTABLE=/usr/bin/python2 ../;
    make install;
    # Install mpmath via pip.
    pip install --user mpmath;
    python -c "import pyranha.test; pyranha.test.run_test_suite()";
    # Install sphinx and the rtd theme.
    pip install --user sphinx
    pip install --user sphinx_rtd_theme
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
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_PYRANHA=yes -DBUILD_TESTS=no -DCMAKE_CXX_FLAGS_DEBUG=-g0 -DCMAKE_CXX_FLAGS=-Os -DCMAKE_INSTALL_PREFIX=/home/travis/.local -DBoost_PYTHON_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_python-py32.so -DBoost_PYTHON_LIBRARY_DEBUG=/usr/lib/x86_64-linux-gnu/libboost_python-py32.so -DPYTHON_EXECUTABLE=/usr/bin/python3 ../;
    make install;
    # Install mpmath manually.
    wget "http://mpmath.org/files/mpmath-0.19.tar.gz";
    tar xzf mpmath-0.19.tar.gz;
    cd mpmath-0.19;
    python3 setup.py install --user;
    cd ..;
    python3 -c "import pyranha.test; pyranha.test.run_test_suite()";
elif [[ "${BUILD_TYPE}" == "Tutorial" ]]; then
    cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TUTORIAL=yes -DBUILD_TESTS=no ../;
    make;
    ctest -V;
elif [[ "${BUILD_TYPE}" == "Doxygen" ]]; then
    # Configure.
    cmake ../;
    # Install a recent version of Doxygen locally.
    wget "http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.11.src.tar.gz";
    tar xzf doxygen-1.8.11.src.tar.gz;
    cd doxygen-1.8.11;
    mkdir build;
    cd build;
    cmake -DCMAKE_INSTALL_PREFIX=/home/travis/.local ../;
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
