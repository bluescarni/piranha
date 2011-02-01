PROJECT(piranha)

ENABLE_TESTING()

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
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
ENDIF(${POINTER_SIZE} EQUAL 8)

# Set default build type to "Debug".
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

# Initial thread setup.
FIND_PACKAGE(Threads REQUIRED)
IF(NOT CMAKE_THREAD_LIBS_INIT)
	MESSAGE(FATAL_ERROR "Could not locate thread library.")
ENDIF(NOT CMAKE_THREAD_LIBS_INIT)
MESSAGE(STATUS "Thread library: ${CMAKE_THREAD_LIBS_INIT}")
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# Include these macros here as they reference variables defined above.
INCLUDE(PiranhaCompilerLinkerSettings)
INCLUDE(PiranhaPlatformSettings)

# Boost libraries setup.
SET(REQUIRED_BOOST_LIBS serialization unit_test_framework)
IF(PIRANHA_ENABLE_BOOST_THREAD)
	MESSAGE(STATUS "Enabling Boost.thread.")
	SET(PIRANHA_USE_BOOST_THREAD "#define PIRANHA_USE_BOOST_THREAD")
	SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} thread)
ENDIF(PIRANHA_ENABLE_BOOST_THREAD)
MESSAGE(STATUS "Required Boost libraries: ${REQUIRED_BOOST_LIBS}")
IF(PIRANHA_ENABLE_BOOST_THREAD)
	# NOTE: it seems like proper c++0x support in Boost.thread is there from around 1.45.0. Possibly earlier,
	# but no earlier than 1.43.0.
	FIND_PACKAGE(Boost 1.45.0 REQUIRED COMPONENTS "${REQUIRED_BOOST_LIBS}")
ELSE(PIRANHA_ENABLE_BOOST_THREAD)
	FIND_PACKAGE(Boost 1.42.0 REQUIRED COMPONENTS "${REQUIRED_BOOST_LIBS}")
ENDIF(PIRANHA_ENABLE_BOOST_THREAD)
MESSAGE(STATUS "Detected Boost version: ${Boost_VERSION}")
# Include system Boost headers.
MESSAGE(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost libraries: ${Boost_LIBRARIES}")
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
# Set the mandatory Boost libraries. NOTE: here we do not include the unit testing, since it will only be useful
# when compiling tests.
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY})
IF(PIRANHA_ENABLE_BOOST_THREAD)
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${Boost_THREAD_LIBRARY})
ENDIF(PIRANHA_ENABLE_BOOST_THREAD)

# GMP setup.
FIND_PACKAGE(GMP REQUIRED)
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${GMP_LIBRARIES})
INCLUDE_DIRECTORIES(${GMP_INCLUDE_DIR})

# Configure config.hpp.
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.hpp.in" "${CMAKE_SOURCE_DIR}/src/config.hpp")

# Add the directory for the piranha library.
ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/src")

# Link main to piranha_static library.
IF(BUILD_MAIN)
	ADD_EXECUTABLE(main main.cpp)
	TARGET_LINK_LIBRARIES(main  piranha_static)
ENDIF(BUILD_MAIN)

IF(BUILD_TESTS)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/tests")
ENDIF(BUILD_TESTS)
