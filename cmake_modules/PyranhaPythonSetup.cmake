# Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)
#
# This file is part of the Piranha library.
#
# The Piranha library is free software; you can redistribute it and/or modify
# it under the terms of either:
#
#  * the GNU Lesser General Public License as published by the Free
#    Software Foundation; either version 3 of the License, or (at your
#    option) any later version.
#
# or
#
#  * the GNU General Public License as published by the Free Software
#    Foundation; either version 3 of the License, or (at your option) any
#    later version.
#
# or both in parallel, as here.
#
# The Piranha library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received copies of the GNU General Public License and the
# GNU Lesser General Public License along with the Piranha library.  If not,
# see https://www.gnu.org/licenses/.

# NOTE: if and when we decide to support OSX, recover some of the quirks in the PaGMO
# version of this file. Let's keep it basic for the moment.

INCLUDE(FindPythonLibs)
# We need the Python interpreter to figure out Python's version in certain cases.
INCLUDE(FindPythonInterp)

# Find Python libraries
FIND_PACKAGE(PythonLibs REQUIRED)
MESSAGE(STATUS "Python libraries: " "${PYTHON_LIBRARIES}")
MESSAGE(STATUS "Python library: " "${PYTHON_LIBRARY}")
MESSAGE(STATUS "Python include dirs: " "${PYTHON_INCLUDE_DIRS}")

# These flags are used to signal the need to override the default extension of the Python modules
# depending on the architecture. Under Windows, for instance, CMake produces shared objects as
# .dll files, but Python from 2.5 onwards requires .pyd files (hence the need to override).
SET(PYDEXTENSION FALSE)

IF(UNIX)
	# We need the Python interpreter in order to detect the appropriate directory of modules installation.
	IF(NOT PYTHONINTERP_FOUND)
		MESSAGE(FATAL_ERROR "Unable to locate the Python interpreter.")
	ENDIF(NOT PYTHONINTERP_FOUND)
	MESSAGE(STATUS "Python interpreter is: ${PYTHON_EXECUTABLE}")
	# Now we must establish if the installation dir for Python modules is named 'site-packages' (as usual)
	# or 'dist-packages' (apparently Ubuntu 9.04 or maybe Python 2.6, it's not clear).
	EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/cmake_modules/python_packages_dir.py
		OUTPUT_VARIABLE PY_PACKAGES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	MESSAGE(STATUS "Python packages dir is: ${PY_PACKAGES_DIR}")
	# In Unix systems we can fetch the Python version number directly from the library.
	STRING(REGEX MATCH libpython[0-9]*\\.[0-9]* PYTHON_LIBRARY_VERSION_DOT ${PYTHON_LIBRARY})
	# Remove the dot from the Python version.
	STRING(REGEX REPLACE libpython "" PYTHON_LIBRARY_VERSION_DOT ${PYTHON_LIBRARY_VERSION_DOT})
	STRING(REGEX REPLACE \\. "" PYTHON_LIBRARY_VERSION ${PYTHON_LIBRARY_VERSION_DOT})
	# Let's use CMAKE_INSTALL_PREFIX, so that if we specify a different install path it will be respected.
	SET(PYTHON_MODULES_PATH lib/python${PYTHON_LIBRARY_VERSION_DOT}/${PY_PACKAGES_DIR})
ELSE(UNIX)
	STRING(REGEX MATCH python[0-9]* PYTHON_LIBRARY_VERSION ${PYTHON_LIBRARY})
	STRING(REGEX REPLACE python "" PYTHON_LIBRARY_VERSION ${PYTHON_LIBRARY_VERSION})
	SET(PYTHON_MODULES_PATH .)
	IF(WIN32)
		# .pyd extension is default on Windows with supported Python versions.
		SET(PYDEXTENSION TRUE)
		MESSAGE(STATUS "Windows platform detected. Output extension for compiled modules will be '.pyd'.")
	ENDIF(WIN32)
ENDIF(UNIX)

MESSAGE(STATUS "Python library version: " ${PYTHON_LIBRARY_VERSION})
MESSAGE(STATUS "Python modules install path: " "${PYTHON_MODULES_PATH}")

IF(${PYTHON_LIBRARY_VERSION} LESS 26)
	MESSAGE(FATAL_ERROR "Minimum supported Python version is 2.6.")
ENDIF(${PYTHON_LIBRARY_VERSION} LESS 26)
