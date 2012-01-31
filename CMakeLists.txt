PROJECT(piranha)

ENABLE_TESTING()

CMAKE_MINIMUM_REQUIRED(VERSION 2.6.0)

SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)
INCLUDE(FindBoost)
INCLUDE(FindGMP)
INCLUDE(FindThreads)

MESSAGE(STATUS "System name: ${CMAKE_SYSTEM_NAME}")

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

# Build option: build the Python bindings.
OPTION(BUILD_PYRANHA "Build Pyranha, the Python bindings for Piranha." OFF)

# Use TCMalloc as allocator.
OPTION(USE_TCMALLOC "Enable use of TCMalloc in release builds." OFF)

# Initialise (empty) list of libraries to link against.
SET(MANDATORY_LIBRARIES "")

# Initial thread setup.
# NOTE: we do it only on Unix for now, as on MinGW it might find and try to use the internal libpthread
# implementation, which is not what we want (we want the native Win32 threads).
IF(NOT WIN32)
	FIND_PACKAGE(Threads REQUIRED)
	MESSAGE(STATUS "Thread library: ${CMAKE_THREAD_LIBS_INIT}")
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF(NOT WIN32)

# Include these macros here as they reference variables defined above.
INCLUDE(PiranhaCompilerLinkerSettings)
INCLUDE(PiranhaPlatformSettings)
IF(BUILD_PYRANHA)
	INCLUDE(PyranhaPythonSetup)
ENDIF(BUILD_PYRANHA)

# Boost libraries setup.
SET(REQUIRED_BOOST_LIBS serialization unit_test_framework)
IF(PIRANHA_ENABLE_BOOST_THREAD)
	MESSAGE(STATUS "Enabling Boost.thread.")
	SET(PIRANHA_USE_BOOST_THREAD "#define PIRANHA_USE_BOOST_THREAD")
	SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} thread)
ENDIF(PIRANHA_ENABLE_BOOST_THREAD)
IF(BUILD_PYRANHA)
	SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} python)
ENDIF(BUILD_PYRANHA)
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

# TCMalloc setup.
IF(USE_TCMALLOC AND CMAKE_BUILD_TYPE STREQUAL "Release")
	FIND_LIBRARY(TCMALLOC_LIBRARY NAMES tcmalloc tcmalloc_minimal)
	IF(NOT TCMALLOC_LIBRARY)
		MESSAGE(FATAL_ERROR "TCMalloc use was requested but the library could not be located.")
	ENDIF(NOT TCMALLOC_LIBRARY)
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${TCMALLOC_LIBRARY})
	SET(PIRANHA_USE_TCMALLOC "#define PIRANHA_USE_TCMALLOC")
ENDIF(USE_TCMALLOC AND CMAKE_BUILD_TYPE STREQUAL "Release")

FIND_PROGRAM(PIRANHA_GIT_EXECUTABLE "git" DOC "Path to the git binary.")
IF(PIRANHA_GIT_EXECUTABLE)
	MESSAGE(STATUS "Git executable: ${PIRANHA_GIT_EXECUTABLE}")
	# Version number setup.
	SET(PIRANHA_GIT_ARGS "log" "--no-color" "-n1" "--date=short" "--pretty=format:%ad")
	EXECUTE_PROCESS(COMMAND ${PIRANHA_GIT_EXECUTABLE} ${PIRANHA_GIT_ARGS} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE PIRANHA_GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REGEX REPLACE "-" "." PIRANHA_VERSION ${PIRANHA_GIT_VERSION})
	MESSAGE(STATUS "Piranha version: ${PIRANHA_VERSION}")
	SET(PIRANHA_VERSION "#define PIRANHA_VERSION \"${PIRANHA_VERSION}\"")
ELSE(PIRANHA_GIT_EXECUTABLE)
	MESSAGE(STATUS "Git executable: not found")
	SET(PIRANHA_VERSION "#define PIRANHA_VERSION \"Undetermined\"")
ENDIF(PIRANHA_GIT_EXECUTABLE)

# Configure extended integer types.
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/extended_integer_types.hpp.in" "${CMAKE_SOURCE_DIR}/src/extended_integer_types.hpp")

# Configure config.hpp.
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.hpp.in" "${CMAKE_SOURCE_DIR}/src/config.hpp")

# Determine integer types to use in arbitrary-precision integer class.
TRY_RUN(PIRANHA_DETERMINE_INTS_RUN_STATUS PIRANHA_DETERMINE_INTS_COMPILE_STATUS
	"${CMAKE_BINARY_DIR}/cmake_configure_executables" "${CMAKE_SOURCE_DIR}/cmake_modules/determine_int_types.cpp"
	CMAKE_FLAGS
	"-DLINK_LIBRARIES:STRING=${GMP_LIBRARIES}"
	"-DINCLUDE_DIRECTORIES:STRING=${Boost_INCLUDE_DIRS};${GMP_INCLUDE_DIR}"
	COMPILE_OUTPUT_VARIABLE PIRANHA_DETERMINE_INTS_COMPILE_OUT
	RUN_OUTPUT_VARIABLE PIRANHA_DETERMINE_INTS_RUN_OUT
)

IF(NOT PIRANHA_DETERMINE_INTS_COMPILE_STATUS)
	MESSAGE(FATAL_ERROR "Could not compile the test to analyse integer types. Compiler output is:\n${PIRANHA_DETERMINE_INTS_COMPILE_OUT}")
ENDIF(NOT PIRANHA_DETERMINE_INTS_COMPILE_STATUS)

IF(PIRANHA_DETERMINE_INTS_RUN_STATUS)
	MESSAGE(FATAL_ERROR "Error while trying to analyse integer types. Command output is:\n${PIRANHA_DETERMINE_INTS_RUN_OUT}")
ENDIF(PIRANHA_DETERMINE_INTS_RUN_STATUS)

STRING(STRIP "${PIRANHA_DETERMINE_INTS_RUN_OUT}" PIRANHA_DETERMINE_INTS_RUN_OUT_STRIP)
STRING(REGEX REPLACE ",.*" "" PIRANHA_IMMEDIATE_INTEGER_TYPE "${PIRANHA_DETERMINE_INTS_RUN_OUT_STRIP}")
MESSAGE(STATUS "Immediate integer type: ${PIRANHA_IMMEDIATE_INTEGER_TYPE}")
STRING(REGEX REPLACE "[^,]*," "" PIRANHA_WIDE_INTEGER_TYPE "${PIRANHA_DETERMINE_INTS_RUN_OUT_STRIP}")
MESSAGE(STATUS "Wide integer type: ${PIRANHA_WIDE_INTEGER_TYPE}")

# Configure the file for arbitrary-precision integer,
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/integer.hpp.in" "${CMAKE_SOURCE_DIR}/src/integer.hpp")

# Add the directory for the piranha library.
ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/src")

# Link main to piranha_static library.
IF(BUILD_MAIN)
	ADD_EXECUTABLE(main main.cpp)
	TARGET_LINK_LIBRARIES(main piranha_static)
ENDIF(BUILD_MAIN)

IF(BUILD_TESTS)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/tests")
ENDIF(BUILD_TESTS)

IF(BUILD_PYRANHA)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/pyranha")
ENDIF(BUILD_PYRANHA)
