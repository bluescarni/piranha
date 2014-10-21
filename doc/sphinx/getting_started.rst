.. _getting_started:

Getting started
===============

Supported platforms
-------------------

Piranha requires a recent compiler with robust support for the C++11 standard. The following
compilers are routinely used to develop and test the library:

* `GCC <http://gcc.gnu.org/>`_ 4.8 and later,
* `Clang <http://clang.llvm.org/>`_ 3.4 (version 3.3 should work as well),
* `Intel C++ compiler <https://software.intel.com/en-us/c-compilers>`_ 15 (limited support).

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
of the configuration process will probably be required.

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

   git clone git@github.com:bluescarni/piranha.git piranha


Installation from source
------------------------
