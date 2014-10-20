.. _getting_started:

Getting started
===============

Supported platforms
-------------------

Piranha requires a recent compiler with robust support for the C++11 standard. The following
compilers are routinely used to develop and test the library:

* GCC 4.8 and later,
* Clang 3.4 (version 3.3 should work as well),
* Intel C++ 15 (limited support).

The main platform used for the development of Piranha is 64-bit GNU/Linux. Some level of support
for OSX, Windows (using MinGW) and BSD flavours exists, but the library is compiled and tested on these
platforms only occasionally.

The code is designed to be standard-compliant and any portability issue in the source code
should be reported as a bug.

Dependencies
------------

Piranha has a few mandatory dependencies:

* the Boost C++ libraries,
* the GNU multiprecision library, GMP (the MPIR fork should work as well),
* the MPFR multiprecision floating-point library.

The optional Python bindings, Pyranha, need the Python programming language (both Python
2.x and 3.x are supported).

These dependencies should be installed on your system before attempting to compile Piranha. On GNU/Linux
and BSD flavours, the best way to proceed is to install them via the distribution's package manager
(remembering to install also the ``-devel`` counterparts of the packages). If the dependencies are installed
in non-standard locations (as it can often happen on OSX and Windows), a certain degree of manual tweaking
of the configuration process will probably be required.

Download
--------

Installation from source
------------------------
