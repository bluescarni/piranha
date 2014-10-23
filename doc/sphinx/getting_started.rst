.. _getting_started:

Getting started
===============

Supported platforms
-------------------

Piranha requires a recent compiler with robust support for the C++11 standard. The following
compilers are routinely used to develop and test the library:

* `GCC <http://gcc.gnu.org/>`_ 4.8 and later,
* `Clang <http://clang.llvm.org/>`_ 3.4 (version 3.3 should work as well),
* `Intel C++ compiler <https://software.intel.com/en-us/c-compilers>`_ 15 (beta support).

The main platform used for the development of Piranha is 64-bit GNU/Linux. Some level of support
for OSX, Windows (using MinGW) and BSD flavours exists, but the library is compiled and tested on these
platforms only occasionally.

The code is designed to be standard-compliant and any portability issue in the source code
should be reported as a bug.

Dependencies
------------

Piranha has a few mandatory dependencies:

* the `Boost <http://www.boost.org/>`_ C++ libraries,
* the `GNU GMP <http://www.gmplib.org>`_ multiprecision library (the `MPIR <http://mpir.org/>`_ fork should work as well),
* the `GNU MPFR <http://www.mpfr.org>`_ multiprecision floating-point library.

The optional Python bindings, Pyranha, need the `Python <http://www.python.org/>`_ programming language (both Python
2.x and 3.x are supported). `CMake <http://www.cmake.org/>`_ is the build system used by Piranha and
must also be installed.

These dependencies should be installed on your system before attempting to compile Piranha. On GNU/Linux
and BSD flavours, the best way to proceed is to install them via the distribution's package manager
(remembering to install also the ``-devel`` counterparts of the packages). If the dependencies are installed
in non-standard locations (as it can often happen on OSX and Windows), a certain degree of manual tweaking
of the :ref:`configuration process <getting_started_configuration>` will probably be required.

Download
--------

At the present time Piranha is being actively developed and no stable release exists yet. Development
happens in the `GitHub repository <https://github.com/bluescarni/piranha>`_.
The ``master`` branch is considered to be the stable version of Piranha,
whereas the separate ``development`` branch is where active development takes place.

A snapshot of the ``master`` branch can be downloaded
`here <https://github.com/bluescarni/piranha/archive/master.zip>`_, or checked out using
the `Git <http://git-scm.com/>`_ version control system. The following Git command will download
the latest version of the ``master`` branch in a directory called ``piranha``:

.. code-block:: bash

   $ git clone git@github.com:bluescarni/piranha.git piranha

You can keep the Piranha source code up-to-date by running periodically the command

.. code-block:: bash

   $ git pull

from the ``piranha`` source directory.

Installation from source
------------------------

The installation of Piranha from source code is split into three main phases:

* the *configuration* phase, where the build system probes the host environment, configures the source
  code and sets the appropriate compiler flags,
* the *building* phase, and
* the *installation* phase, where the library is installed on the system.

All three phases are handled, in Piranha, by the `CMake <http://www.cmake.org/>`_ build system.
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

.. code-block:: bash

   $ cmake ../

CMake will look for Piranha's dependencies in standard paths, and it will produce an error message if it cannot
detect them. It is possible to tell explicitly CMake where to find a specific library by passing the information
on the command line. For instance,

.. code-block:: bash

   $ cmake ../ -DGMP_LIBRARIES=/custom/location/for/libgmp.so

will instruct CMake to use the GMP library at the location ``/custom/location/for/libgmp.so``. CMake's GUIs are handy
to discover, set and, if necessary, override the internal variables set by CMake during the configuration phase.
This is particularily useful on platforms such as OSX and Windows.

Piranha can be built either in ``Debug`` or in ``Release`` mode. In ``Debug`` mode, the code will perform
extensive self-checking and performance will be greatly reduced with respect to the ``Release`` mode. The build
mode can be set with the ``CMAKE_BUILD_TYPE`` CMake variable, e.g.,

.. code-block:: bash

   $ cmake ../ -DCMAKE_BUILD_TYPE=Release

In order to compile Pyranha, the ``BUILD_PYRANHA`` option must be enabled:

.. code-block:: bash

   $ cmake ../ -DBUILD_PYRANHA=ON

Another useful CMake option is ``BUILD_TESTS``: if selected, a suite of tests will be built. In ``Debug`` mode,
unit tests will be built, in ``Release`` mode performance tests will be built.

The compiler selected by CMake is chosen according to platform-specific heuristics. If you have only one compiler
installed on your system, there will be no ambiguity. If, however, you have multiple toolchains installed and want
to force CMake to pick a specific one, you can set the ``CXX`` environment variable *before* running CMake. In
``bash``, a possible way of doing this is

.. code-block:: bash

   $ CXX=/path/to/other/compiler/icpc cmake ../

This will force CMake to use the Intel C++ compiler ``icpc`` at the location ``/path/to/other/compiler/``.

Building
^^^^^^^^

After the configuration step, Piranha is ready to be built. Piranha is a header-only C++ library,
so, technically, you do not need to actually compile anything to use the library from C++ (but the configuration
step above is still necessary to setup platform-specific functionality in the headers). The building stage
is however needed when building the Python bindings Pyranha and/or when testing is enabled.

In Unix-like environments, you can build the tests and the Python bindings by running the standard
``make`` tool from the ``build`` directory:

.. code-block:: bash

   $ make

On a multicore machine, it is possible to launch make in parallel to speed up the compilation. An example with 8 parallel
jobs:

.. code-block:: bash

   $ make -j8

.. warning:: Be aware that, at the present time, the compilation of Piranha's unit tests and Python bindings consumes a
   large amount of memory. Do not run too many jobs in parallel if the amount of memory available on your machine is limited.

After a successful build in ``Debug`` mode, it is good practice to run the test suite:

.. code-block:: bash

   $ make test

A full run of the test suite should take a few minutes on a modern desktop machine. Any failure in the unit tests should be reported as a bug.

Installation
^^^^^^^^^^^^

The final step is the installation of Piranha on the system. In Unix-like environments, the default installation path (also known as the
``PREFIX``) is ``/usr/local``. The standard

.. code-block:: bash

   $ make install

command will copy the Piranha C++ headers into ``PREFIX/include/piranha``, and the Pyranha module (if built) in an auto-detected subdirectory
of ``PREFIX`` where Python modules can be found by the Python interpreter (e.g., something like ``PREFIX/lib/python2.7/site-packages`` in a
typical Python 2.7 installation).

If you do not have write permissions in ``/usr/local``, it is possible to change the ``PREFIX`` in the configuration phase. It is
advisable to set the ``PREFIX`` to a subdirectory in the user's home directory (e.g., ``/home/username/.local``).
The ``PREFIX`` can be set via the ``CMAKE_INSTALL_PREFIX`` CMake variable.

Hello Piranha!
--------------

It should now be possible to compile and run your first C++ Piranha program.

.. code-block:: c++

   #include <iostream>

   // Include the global Piranha header.
   #include "piranha/piranha.hpp"

   // Import the Piranha namespace.
   using namespace piranha;

   int main()
   {
     std::cout << rational{4,3} << '\n';
   }

In Unix-like environments, you can compile this simple program with GCC (or Clang, or the Intel compiler) via the command:

.. code-block:: bash

   $ g++ -std=c++11 hello_piranha.cpp -lmpfr -lgmp

A couple of things to note:

* we pass the ``-std=c++11`` to specify that we want to use the C++11 version of the C++ language for the compilation;
* we specify via the ``-lmpfr -lgmp`` flags that the executable needs to be linked to the GMP and MPFR libraries (if
  you do not do this, the program will still compile but the final linking will fail due to undefined references);
* at the present time, all the Boost libraries used within Piranha are header-only and thus no linking to any Boost
  library is necessary.

Note that if you installed Piranha in a custom ``PREFIX``, you will need to specify on the command line where
the Piranha headers are located via the ``-I`` switch. E.g.,

.. code-block:: bash

   $ g++ -I/home/username/.local/include -std=c++11 hello_piranha.cpp -lmpfr -lgmp

If the GMP and/or MPFR libraries are not installed in a standard path, you can use the ``-L`` switch to tell GCC
where to look for them:

.. code-block:: bash

   $ g++ -std=c++11 hello_piranha.cpp -L/custom/library/path -lmpfr -lgmp
