PROJECT(piranha)

ENABLE_TESTING()

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.7)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules" "${CMAKE_SOURCE_DIR}/cmake_modules/yacma")

MESSAGE(STATUS "System name: ${CMAKE_SYSTEM_NAME}")

# Set default build type to "Release".
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

# Build Option: when active the file main.cpp is built.
option(BUILD_MAIN "Build 'main.cpp'." OFF)

# Build option: enable test set.
option(BUILD_TESTS "Build test set." OFF)

# Build option: build the C++ tutorial.
option(BUILD_TUTORIAL "Build the C++ tutorial." OFF)

# Build option: build the Python bindings.
option(BUILD_PYRANHA "Build Pyranha, the Python bindings for Piranha." OFF)

# Use TCMalloc as allocator.
option(USE_TCMALLOC "Enable use of TCMalloc in release builds." OFF)

# Enable msgpack serialization format.
option(PIRANHA_WITH_MSGPACK "Enable support for the msgpack serialization format." OFF)

# Enable zlib/gzip compression.
option(PIRANHA_WITH_ZLIB "Enable support for zlib/gzip compression." OFF)

# A general-purpose option to signal that we intend to run Piranha under Valgrind.
# At the moment it just disables tests involving long double that give problems in Valgrind,
# in the future it might become a more general-purpose flag.
option(RUN_ON_VALGRIND "Configure Piranha to be run on Valgrind." OFF)
# Make it an advanced option, not really interesting for non-developers.
mark_as_advanced(RUN_ON_VALGRIND)

# Initialise (empty) list of libraries to link against.
SET(MANDATORY_LIBRARIES "")

# Initial setup of the compilation flags.
include(PiranhaCompilerLinkerSettings)

# Additional platform-specific setup.
include(PiranhaPlatformSettings)

# Threading setup.
include(YACMAThreadingSetup)
set(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${YACMA_THREADING_CXX_FLAGS}")
if(YACMA_HAS_PTHREAD_AFFINITY)
	set(PIRANHA_PTHREAD_AFFINITY "#define PIRANHA_HAVE_PTHREAD_AFFINITY")
endif()
if(YACMA_HAS_THREAD_LOCAL)
	set(PIRANHA_THREAD_LOCAL "#define PIRANHA_HAVE_THREAD_LOCAL")
endif()

# Report the CXX flags.
message(STATUS "Current CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Current CXX debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")

# NOTE: ideally we would want this inside the pyranha CMakeLists.txt, however
# it seems like there's a strange interaction between the code for finding Boost.Python
# and the CMake FindPythonLibs macro, and it does not work that way.
if(BUILD_PYRANHA)
	include(YACMAPythonSetup)
endif()

# Boost libraries setup.
# NOTE: apparently, according to the FindBoost.cmake of cmake 3.5, iostreams
# depends on regex.
SET(REQUIRED_BOOST_LIBS serialization iostreams regex)
IF(BUILD_TESTS)
	# These libraries are needed only if building tests.
	MESSAGE(STATUS "Linking unit tests to Boost.Test, Boost.Filesystem and Boost System")
	SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} unit_test_framework filesystem system)
	IF(CMAKE_BUILD_TYPE STREQUAL "Release")
		MESSAGE(STATUS "Linking performance tests to Boost Timer and Boost.Chrono.")
		SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} timer chrono)
	ENDIF()
ENDIF()
IF(BUILD_PYRANHA)
	SET(REQUIRED_BOOST_LIBS ${REQUIRED_BOOST_LIBS} python)
ENDIF()
MESSAGE(STATUS "Required Boost libraries: ${REQUIRED_BOOST_LIBS}")
# Timers are available since 1.48.0.
FIND_PACKAGE(Boost 1.48.0 REQUIRED COMPONENTS "${REQUIRED_BOOST_LIBS}")
MESSAGE(STATUS "Detected Boost version: ${Boost_VERSION}")
# Include system Boost headers.
MESSAGE(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost libraries: ${Boost_LIBRARIES}")
# NOTE: mark as system headers to avoid excessive warnings in debug mode.
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})
# Set the mandatory Boost libraries.
# NOTE: here we do not include the libraries pulled in by the tests, only those which are needed by Piranha's core.
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${Boost_SERIALIZATION_LIBRARY} ${Boost_IOSTREAMS_LIBRARY} ${Boost_REGEX_LIBRARY})

# GMP setup.
FIND_PACKAGE(GMP REQUIRED)
MESSAGE(STATUS "GMP library found.")
MESSAGE(STATUS "GMP include dir is: ${GMP_INCLUDE_DIR}")
MESSAGE(STATUS "GMP library is: ${GMP_LIBRARIES}")
# Same as Boost, mark as a system header.
INCLUDE_DIRECTORIES(SYSTEM ${GMP_INCLUDE_DIR})

# MPFR setup.
FIND_PACKAGE(MPFR REQUIRED)
MESSAGE(STATUS "MPFR library found.")
MESSAGE(STATUS "MPFR include dir is: ${MPFR_INCLUDE_DIR}")
MESSAGE(STATUS "MPFR library is: ${MPFR_LIBRARIES}")
MESSAGE(STATUS "Checking MPFR version.")
TRY_COMPILE(MPFR_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/mpfr_check_version.cpp"
	CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${MPFR_INCLUDE_DIR};${GMP_INCLUDE_DIR}")
IF(NOT MPFR_VERSION_CHECK)
	MESSAGE(FATAL_ERROR "Unsupported MPFR version, please upgrade.")
ENDIF()
MESSAGE(STATUS "MPFR version is ok.")
INCLUDE_DIRECTORIES(SYSTEM ${MPFR_INCLUDE_DIR})

# NOTE: MPFR should be linked in before GMP, that's why we link GMP here.
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${MPFR_LIBRARIES} ${GMP_LIBRARIES})

if(PIRANHA_WITH_MSGPACK)
	find_package(MSGPACK-C REQUIRED)
	message(STATUS "msgpack-c library found.")
	message(STATUS "msgpack-c include dir is: ${MSGPACK-C_INCLUDE_DIR}")
	try_compile(MSGPACK-C_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/msgpack_check_version.cpp"
		CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${MSGPACK-C_INCLUDE_DIR}")
	if(NOT MSGPACK-C_VERSION_CHECK)
		message(FATAL_ERROR "Unsupported msgpack-c version, please upgrade.")
	endif()
	message(STATUS "msgpack-c version is ok.")
	# msgpack load file requires memory mapping for uncompressed format.
	try_compile(BOOST_MAPPED_FILE_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/test_mmapped_file.cpp"
		CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${Boost_INCLUDE_DIRS}" LINK_LIBRARIES "${Boost_IOSTREAMS_LIBRARY}")
	if(NOT BOOST_MAPPED_FILE_CHECK)
		message(FATAL_ERROR "msgpack-c requires an implementation of Boost's memory mapped file.")
	endif()
	message(STATUS "Boost's memory mapped file is supported.")
	# We make explicit use of std::uint32_t.
	set(CMAKE_EXTRA_INCLUDE_FILES "cstdint")
	check_type_size(std::uint32_t PIRANHA_HAVE_UINT32_T LANGUAGE CXX)
	if(NOT PIRANHA_HAVE_UINT32_T)
		message(FATAL_ERROR "msgpack support requires the availability of the std::uint32_t type.")
	endif()
	unset(CMAKE_EXTRA_INCLUDE_FILES)
	unset(PIRANHA_HAVE_UINT32_T)
	include_directories(SYSTEM ${MSGPACK-C_INCLUDE_DIR})
	set(PIRANHA_ENABLE_MSGPACK "#define PIRANHA_WITH_MSGPACK")
endif()

if(PIRANHA_WITH_ZLIB)
	find_package(ZLIB REQUIRED)
	message(STATUS "zlib library found.")
	message(STATUS "zlib include dir is: ${ZLIB_INCLUDE_DIR}")
	message(STATUS "zlib library is: ${ZLIB_LIBRARIES}")
	include_directories(SYSTEM ${ZLIB_INCLUDE_DIR})
	set(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${ZLIB_LIBRARIES})
	set(PIRANHA_ENABLE_ZLIB "#define PIRANHA_WITH_ZLIB")
endif()

# bzip2 setup.
find_package(BZip2 REQUIRED)
message(STATUS "bzip2 library found.")
message(STATUS "bzip2 include dir is: ${BZIP2_INCLUDE_DIR}")
message(STATUS "bzip2 library is: ${BZIP2_LIBRARIES}")
include_directories(SYSTEM ${BZIP2_INCLUDE_DIR})
SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${BZIP2_LIBRARIES})

# TCMalloc setup.
IF(USE_TCMALLOC AND CMAKE_BUILD_TYPE STREQUAL "Release")
	FIND_LIBRARY(TCMALLOC_LIBRARY NAMES tcmalloc tcmalloc_minimal)
	IF(NOT TCMALLOC_LIBRARY)
		MESSAGE(FATAL_ERROR "TCMalloc use was requested but the library could not be located.")
	ENDIF()
	SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${TCMALLOC_LIBRARY})
ENDIF()

FIND_PROGRAM(PIRANHA_GIT_EXECUTABLE "git" DOC "Path to the git binary.")
IF(PIRANHA_GIT_EXECUTABLE)
	MESSAGE(STATUS "Git executable: ${PIRANHA_GIT_EXECUTABLE}")
	# Version number setup.
	SET(PIRANHA_GIT_ARGS "log" "--no-color" "-n1" "--date=short" "--pretty=format:%ad")
	EXECUTE_PROCESS(COMMAND ${PIRANHA_GIT_EXECUTABLE} ${PIRANHA_GIT_ARGS} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE PIRANHA_GIT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
	STRING(REGEX REPLACE "-" "." PIRANHA_VERSION ${PIRANHA_GIT_VERSION})
	MESSAGE(STATUS "Piranha version: ${PIRANHA_VERSION}")
	SET(PIRANHA_VERSION "#define PIRANHA_VERSION \"${PIRANHA_VERSION}\"")
ELSE(PIRANHA_GIT_EXECUTABLE)
	MESSAGE(STATUS "Git executable: not found")
	SET(PIRANHA_VERSION "#define PIRANHA_VERSION \"Undetermined\"")
ENDIF()

# Configure config.hpp.
CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/src/config.hpp.in" "${CMAKE_SOURCE_DIR}/src/config.hpp")

# Add the directory for the piranha library.
ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/src")

# Link main to piranha library.
IF(BUILD_MAIN)
	ADD_EXECUTABLE(main main.cpp)
	TARGET_LINK_LIBRARIES(main ${MANDATORY_LIBRARIES})
ENDIF()

IF(BUILD_TESTS)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/tests")
ENDIF()

if(BUILD_TUTORIAL)
	add_subdirectory("${CMAKE_SOURCE_DIR}/tutorial")
endif()

IF(BUILD_PYRANHA)
	ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/pyranha")
ENDIF()
