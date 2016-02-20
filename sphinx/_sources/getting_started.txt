.. _getting_started:

Getting started
===============

Supported platforms
-------------------

Piranha requires a recent compiler with robust support for the C++11 standard. The following
compilers are routinely used to develop and test the library:

* `GCC <http://gcc.gnu.org/>`__ 4.8 and later,
* `Clang <http://clang.llvm.org/>`__ 3.5 (earlier versions up to 3.1 should work as well),
* `Intel C++ compiler <https://software.intel.com/en-us/c-compilers>`__ 15 (beta support).

The main platform used for the development of Piranha is 64-bit GNU/Linux.
OSX, Windows (using `MinGW <http://mingw-w64.sourceforge.net/>`__) and BSD flavours are also supported,
although the library is compiled and tested on these platforms somewhat less frequently.

Piranha aims to be standard-compliant and any portability issue in the source code
should be reported as a bug.

Dependencies
------------

Piranha has a few mandatory dependencies:

* the `Boost <http://www.boost.org/>`__ C++ libraries,
* the `GMP <http://www.gmplib.org>`__ multiprecision library (the `MPIR <http://mpir.org/>`__ fork should work as well),
* the `GNU MPFR <http://www.mpfr.org>`__ multiprecision floating-point library,
* the `bzip2 <http://www.bzip.org/>`__ compression library.

The optional Python bindings, Pyranha, need the `Python <http://www.python.org/>`__ programming language (Python
2.6 and later versions, including Python 3.x, are supported). `CMake <http://www.cmake.org/>`__ is the build system
used by Piranha and must also be installed.

These dependencies should be installed on your system before attempting to compile Piranha. On GNU/Linux
and BSD flavours, the best way to proceed is to install them via the distribution's package manager
(remembering to install also the ``-devel`` counterparts of the packages). If the dependencies are installed
in non-standard locations (as it can often happen on OSX and Windows), a certain degree of manual tweaking
of the :ref:`configuration process <getting_started_configuration>` will probably be required.

Download
--------

At the present time Piranha is being actively developed and no stable release exists yet. Development
happens in the `GitHub repository <https://github.com/bluescarni/piranha>`__.
The ``master`` branch is considered to be the stable version of Piranha,
whereas the separate ``development`` branch is where active development takes place.

A snapshot of the ``master`` branch can be downloaded
`here <https://github.com/bluescarni/piranha/archive/master.zip>`__, or checked out using
the `Git <http://git-scm.com/>`__ version control system. The following Git command will download
the latest version of the ``master`` branch in a directory called ``piranha``:

.. code-block:: console

   $ git clone git@github.com:bluescarni/piranha.git piranha

You can keep the Piranha source code up-to-date by running periodically the command

.. code-block:: console

   $ git pull

from the ``piranha`` source directory.

Installation from source
------------------------

The installation of Piranha from source code is split into three main phases:

* the *configuration* phase, where the build system probes the host environment, configures the source
  code and sets the appropriate compiler flags,
* the *building* phase, and
* the *installation* phase, where the library is installed on the system.

All three phases are handled, in Piranha, by the `CMake <http://www.cmake.org/>`__ build system.
In this section we will detail the installation process in a Unix-like environment using
a command-line interface. The process will be conceptually similar in case you are using
one of the available CMake GUIs (such as ``cmake-gui`` or ``ccmake``).

.. _getting_started_configuration:

Configuration
^^^^^^^^^^^^^

The first step is to create a separate ``build`` directory within the ``piranha`` source tree.
This directory will contain all the temporary files created during the compilation of the source code.
If you want to restart the compilation process from scratch, it will be enough to erase the ``build`` directory,
and repeat the process described here. This is technically called an *out-of-source* build, as the files
generated during the build process are separated from the original source files.

From within the ``build`` directory, run the command

.. code-block:: console

   $ cmake ../

CMake will look for Piranha's dependencies in standard paths, and it will produce an error message if it cannot
detect them. It is possible to tell explicitly CMake where to find a specific library by passing the information
on the command line. For instance,

.. code-block:: console

   $ cmake ../ -DGMP_LIBRARIES=/custom/location/for/libgmp.so

will instruct CMake to use the GMP library at the location ``/custom/location/for/libgmp.so``. CMake's GUIs are handy
to discover, set and, if necessary, override the internal variables set by CMake during the configuration phase.
This is particularily useful on platforms such as OSX and Windows.

Piranha can be built either in ``Debug`` or in ``Release`` mode. In ``Debug`` mode, the code will perform
extensive self-checking and performance will be greatly reduced with respect to the ``Release`` mode. The build
mode can be set with the ``CMAKE_BUILD_TYPE`` CMake variable, e.g.,

.. code-block:: console

   $ cmake ../ -DCMAKE_BUILD_TYPE=Debug

In order to compile Pyranha, the ``BUILD_PYRANHA`` option must be enabled:

.. code-block:: console

   $ cmake ../ -DBUILD_PYRANHA=ON

Another useful CMake option is ``BUILD_TESTS``: if selected, a suite of tests will be built. In ``Debug`` mode,
unit tests will be built, in ``Release`` mode performance tests will be built.

The compiler selected by CMake is chosen according to platform-specific heuristics. If you have only one compiler
installed on your system, there will be no ambiguity. If, however, you have multiple toolchains installed and want
to force CMake to pick a specific one, you can set the ``CXX`` environment variable *before* running CMake. In
``bash``, a possible way of doing this is

.. code-block:: console

   $ CXX=/path/to/other/compiler/icpc cmake ../

This will force CMake to use the Intel C++ compiler ``icpc`` at the location ``/path/to/other/compiler/``.

Building
^^^^^^^^

After the configuration step, Piranha is ready to be built. Piranha is a header-only C++ library,
so, technically, you do not need to actually compile anything to use the library from C++ (but the configuration
step above is still necessary to setup platform-specific functionality in the headers). The building stage
is however needed when building the Python bindings Pyranha and/or when testing is enabled.

In Unix-like environments, you can build the tests and/or the Python bindings by running the standard
``make`` tool from the ``build`` directory:

.. code-block:: console

   $ make

On a multicore machine, it is possible to launch make in parallel to speed up the compilation. An example with 8 parallel
jobs:

.. code-block:: console

   $ make -j8

.. warning:: Be aware that the compilation of Piranha's unit tests and Python bindings consumes a
   large amount of memory. At least 8GB of RAM are suggested for the compilation of the Python bindings.

After a successful build in ``Debug`` mode, it is good practice to run the test suite:

.. code-block:: console

   $ make test
   Running tests...
   Test project /home/yardbird/repos/piranha/build
         Start  1: array_key
    1/45 Test  #1: array_key ...............................   Passed    0.11 sec
         Start  2: base_term
    2/45 Test  #2: base_term ...............................   Passed    0.03 sec
         Start  3: cache_aligning_allocator
    3/45 Test  #3: cache_aligning_allocator ................   Passed    0.01 sec
         Start  4: convert_to
    4/45 Test  #4: convert_to ..............................   Passed    0.01 sec
         Start  5: dynamic_aligning_allocator
    5/45 Test  #5: dynamic_aligning_allocator ..............   Passed    0.01 sec
         Start  6: echelon_size
    6/45 Test  #6: echelon_size ............................   Passed    0.01 sec
         Start  7: environment
    7/45 Test  #7: environment .............................   Passed    0.01 sec
         Start  8: exceptions
    8/45 Test  #8: exceptions ..............................   Passed    0.01 sec
         Start  9: hash_set
    9/45 Test  #9: hash_set ................................   Passed    8.35 sec
   [...]
   42/45 Test #42: tracing .................................   Passed    0.00 sec
         Start 43: trigonometric_series
   43/45 Test #43: trigonometric_series ....................   Passed    0.03 sec
         Start 44: tuning
   44/45 Test #44: tuning ..................................   Passed    0.00 sec
         Start 45: type_traits
   45/45 Test #45: type_traits .............................   Passed    0.00 sec

   100% tests passed, 0 tests failed out of 45

   Total Test time (real) = 675.26 sec

A full run of the test suite should take a few minutes on a modern desktop machine. Any failure in the unit tests should be reported as a bug.

.. note:: Some of the performance tests will create extremely large series. It is advisable, at least initially, to run each performance test separately
   while monitoring the memory usage in order to avoid heavy thrashing.

Installation
^^^^^^^^^^^^

The final step is the installation of Piranha on the system. In Unix-like environments, the default installation path (also known as the
``PREFIX``) is ``/usr/local``. The standard

.. code-block:: console

   $ make install

command will copy the Piranha C++ headers into ``PREFIX/include/piranha``, and the Pyranha module (if built) in an auto-detected subdirectory
of ``PREFIX`` where Python modules can be found by the Python interpreter (e.g., something like ``PREFIX/lib/python2.7/site-packages`` in a
typical Python 2.7 installation on GNU/Linux).

If you do not have write permissions in ``/usr/local``, it is possible to change the ``PREFIX`` in the configuration phase. It is
advisable to set the ``PREFIX`` to a subdirectory in the user's home directory (e.g., ``/home/username/.local``).
The ``PREFIX`` can be set via the ``CMAKE_INSTALL_PREFIX`` CMake variable during the
:ref:`configuration process <getting_started_configuration>`.

On the Python side, in order to check that the installation of the Pyranha module was successful it will be enough to
attempt importing it from a Python session:

>>> import pyranha

If this command produces no error messages, then the installation of Pyranha was successful. You can run the Pyranha
test suite with the following commands:

.. code-block:: python

   >>> import pyranha.test
   >>> pyranha.test.run_test_suite()
   runTest (pyranha.test.basic_test_case) ... ok
   runTest (pyranha.test.mpmath_test_case) ... ok
   runTest (pyranha.test.math_test_case) ... ok
   runTest (pyranha.test.polynomial_test_case) ... ok
   runTest (pyranha.test.poisson_series_test_case) ... ok
   runTest (pyranha.test.converters_test_case) ... ok
   runTest (pyranha.test.serialization_test_case) ... ok

   ----------------------------------------------------------------------
   Ran 7 tests in 2.905s

   OK

Note that if you specified a non-standard ``PREFIX`` during the configuration phase, you might need to set the ``PYTHONPATH``
environment variable in order for the Python interpreter to locate Pyranha. More information is available
`here <https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH>`__ .

