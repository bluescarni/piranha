PROJECT(piranha)

ENABLE_TESTING()

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckTypeSize)
INCLUDE(FindBoost)
INCLUDE(FindGMP)
INCLUDE(FindThreads)

MESSAGE(STATUS "System name: ${CMAKE_SYSTEM_NAME}")

# Check pointer size.
CHECK_TYPE_SIZE("void *" POINTER_SIZE)
MESSAGE(STATUS "Pointer size is " ${POINTER_SIZE})
IF(${POINTER_SIZE} EQUAL 8)
	MESSAGE(STATUS "Enabling 64bit support.")
	SET(PIRANHA_64BIT_MODE "#define PIRANHA_64BIT_MODE")
ELSE(${POINTER_SIZE} EQUAL 8)
	SET(PIRANHA_64BIT_MODE "")
ENDIF(${POINTER_SIZE} EQUAL 8)

# Configuration for GCC.
IF(CMAKE_COMPILER_IS_GNUCXX)
	MESSAGE(STATUS "GNU compiler detected.")
	CHECK_CXX_COMPILER_FLAG(-Wnon-virtual-dtor GNUCXX_VIRTUAL_DTOR)
	IF(GNUCXX_VIRTUAL_DTOR)
		MESSAGE(STATUS "Enabling '-Wnon-virtual-dtor' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
	ENDIF(GNUCXX_VIRTUAL_DTOR)
	CHECK_CXX_COMPILER_FLAG(-std=c++0x GNUCXX_STD_CPP0X)
	IF(GNUCXX_STD_CPP0X)
		MESSAGE(STATUS "c++0x support detected.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	ELSE(GNUCXX_STD_CPP0X)
		MESSAGE(FATAL_ERROR "c++0x support could not be detected.")
	ENDIF(GNUCXX_STD_CPP0X)
	# Add to the base flags extra warnings. Also, additional flags to turn off some GCC warnings that in practice clutter the compilation output.
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wdisabled-optimization")
	# Suggested for multithreaded code.
	ADD_DEFINITIONS(-D_REENTRANT)
	# When compiling with GCC, enable the GNU extensions - used for instance in thread affinity settings.
	ADD_DEFINITIONS(-D_GNU_SOURCE)
ELSE(CMAKE_COMPILER_IS_GNUCXX)
	MESSAGE(FATAL_ERROR "This compiler is not supported at the present time.")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

# Set default build type to "Release", change it in the GUI if you need to build with debug.
IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE Debug CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Build Option: when active the file main.cpp is built and linked to the piranha static library.
OPTION(BUILD_MAIN "Build 'main.cpp'." OFF)

# Build option: enable test set.
OPTION(BUILD_TESTS "Build test set." ON)

# Initialise (empty) list of libraries to link against.
SET(MANDATORY_LIBRARIES "")

# Look for the math library and if found set it as mandatory
FIND_LIBRARY(SYSTEM_M_LIBRARY NAMES m)
IF(SYSTEM_M_LIBRARY)
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${SYSTEM_M_LIBRARY})
	MESSAGE(STATUS "Math library found: ${SYSTEM_M_LIBRARY}")
ENDIF(SYSTEM_M_LIBRARY)

# Boost libraries setup.
SET(REQUIRED_BOOST_LIBS serialization unit_test_framework)
MESSAGE(STATUS "Required Boost libraries: ${REQUIRED_BOOST_LIBS}")
FIND_PACKAGE(Boost 1.40.0 REQUIRED COMPONENTS "${REQUIRED_BOOST_LIBS}")
MESSAGE(STATUS "Detected Boost version: ${Boost_VERSION}")
# Include system Boost headers.
MESSAGE(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost libraries: ${Boost_LIBRARIES}")
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
# Set the mandatory Boost libraries. NOTE: here we do not include the unit testing, since it will only be useful
# when compiling tests.
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY})

# GMP setup.
FIND_PACKAGE(GMP REQUIRED)
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${GMP_LIBRARIES})
INCLUDE_DIRECTORIES(${GMP_INCLUDE_DIR})

IF(UNIX)
	# Install path for libraries.
	SET(LIB_INSTALL_PATH "lib")
	FIND_PACKAGE(Threads REQUIRED)
	IF(NOT CMAKE_THREAD_LIBS_INIT)
		MESSAGE(FATAL_ERROR "Could not locate thread library.")
	ENDIF(NOT CMAKE_THREAD_LIBS_INIT)
	MESSAGE(STATUS "Thread library: ${CMAKE_THREAD_LIBS_INIT}")
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	# Enable the pthread flag in Unix only if the compiler is GNU.
	# NOTE: here the situation is very hairy: different systems require different GCC flags:
	# http://gcc.gnu.org/onlinedocs/libstdc++/manual/using_concurrency.html
	# This will work at least on Linux/x86 and FreeBSD.
	IF(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_USE_PTHREADS_INIT)
		MESSAGE(STATUS "GCC with POSIX threads detected.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	ENDIF(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_USE_PTHREADS_INIT)
ENDIF(UNIX)

IF(CMAKE_USE_PTHREADS_INIT)
	# Set definitions in config.hpp.
	SET(PIRANHA_THREAD_MODEL "#define PIRANHA_THREAD_MODEL_PTHREADS")
ENDIF(CMAKE_USE_PTHREADS_INIT)

# Configure config.hpp.
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.hpp.in" "${CMAKE_SOURCE_DIR}/src/config.hpp")

# Add the directory for the piranha library.
ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/src")

# From now on all targets will use the static piranha library. Add the relevant flags.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STATIC_LIB_PIRANHA_USE_FLAGS}")

# Link main to piranha_static library.
IF(BUILD_MAIN)
	ADD_EXECUTABLE(main main.cpp)
	TARGET_LINK_LIBRARIES(main  piranha_static)
ENDIF(BUILD_MAIN)

IF(BUILD_TESTS)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/tests")
ENDIF(BUILD_TESTS)
