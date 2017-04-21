cmake_minimum_required(VERSION 3.2.0)

project(piranha VERSION 0.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/yacma")

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")

# Set default build type to "Release".
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING
		"Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
	FORCE)
endif()

# Build option: enable test set.
option(PIRANHA_BUILD_TESTS "Build test set." OFF)

# Build option: enable benchmarks.
option(PIRANHA_BUILD_BENCHMARKS "Build benchmarks." OFF)

# Build option: build the C++ tutorials.
option(PIRANHA_BUILD_TUTORIALS "Build the C++ tutorials." OFF)

# Build option: build the Python bindings.
option(PIRANHA_BUILD_PYRANHA "Build Pyranha, the Python bindings for Piranha." OFF)

# Build option: enable the msgpack serialization format.
option(PIRANHA_WITH_MSGPACK "Enable support for the msgpack serialization format." OFF)

# Build option: enable zlib/gzip compression.
option(PIRANHA_WITH_ZLIB "Enable support for zlib/gzip compression." OFF)

# Build option: enable bzip2 compression.
option(PIRANHA_WITH_BZIP2 "Enable support for bzip2 compression." OFF)

# Build option: enable the installation of the library headers.
option(PIRANHA_INSTALL_HEADERS "Enable the installation of the library headers." ON)
mark_as_advanced(PIRANHA_INSTALL_HEADERS)

# A general-purpose option to signal that we intend to run Piranha under Valgrind.
# At the moment it just disables tests involving long double that give problems in Valgrind,
# in the future it might become a more general-purpose flag.
option(PIRANHA_RUN_ON_VALGRIND "Configure Piranha to be run on Valgrind." OFF)
# Make it an advanced option, not really interesting for non-developers.
mark_as_advanced(PIRANHA_RUN_ON_VALGRIND)

# Setup of the compilation flags.
include(YACMACompilerLinkerSettings)

# Threading setup.
include(YACMAThreadingSetup)
if(YACMA_HAVE_PTHREAD_AFFINITY)
	set(PIRANHA_PTHREAD_AFFINITY "#define PIRANHA_HAVE_PTHREAD_AFFINITY")
endif()
if(YACMA_HAVE_THREAD_LOCAL)
	set(PIRANHA_THREAD_LOCAL "#define PIRANHA_HAVE_THREAD_LOCAL")
endif()

# Start to set up the list of mandatory targets for piranha to link to.
set(PIRANHA_MANDATORY_LINK_TARGETS Threads::Threads)

# Additional platform-specific setup.
include(PiranhaPlatformSettings)

# Assemble the flags.
set(PIRANHA_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG} ${YACMA_THREADING_CXX_FLAGS})
set(PIRANHA_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS} ${YACMA_THREADING_CXX_FLAGS})
# NOTE: the idea here is to enable globally certain compiler options that always need to be activated
# on a certain platform. Further compiler flags will be set for specific targets.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND YACMA_COMPILER_IS_CLANGXX)
	message(STATUS "Clang compiler on OSX detected, setting the standard library to 'libc++'.")
	list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-stdlib=libc++")
	list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-stdlib=libc++")
endif()
if(MINGW)
	# Flag needed to deal with big binaries in MinGW.
	message(STATUS "Enabling the '-Wa,-mbig-obj' flag in MinGW builds.")
	list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-Wa,-mbig-obj")
	list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-Wa,-mbig-obj")
endif()

# NOTE: ideally we would want this inside the pyranha CMakeLists.txt, however
# it seems like there's a strange interaction between the code for finding Boost.Python
# and the CMake FindPythonLibs macro, and it does not work that way.
if(PIRANHA_BUILD_PYRANHA)
	include(YACMAPythonSetup)
endif()

# Boost libraries setup.
include(PiranhaFindBoost)
list(APPEND PIRANHA_MANDATORY_LINK_TARGETS Boost::boost Boost::disable_autolinking Boost::serialization Boost::iostreams)

# GMP/MPFR setup.
find_package(GMP REQUIRED)
message(STATUS "GMP library found.")
message(STATUS "GMP include dir is: ${GMP_INCLUDE_DIR}")
message(STATUS "GMP library is: ${GMP_LIBRARY}")
find_package(MPFR REQUIRED)
message(STATUS "MPFR library found.")
message(STATUS "MPFR include dir is: ${MPFR_INCLUDE_DIR}")
message(STATUS "MPFR library is: ${MPFR_LIBRARY}")
# NOTE: link last GMP, link order might matter depending on the situation.
list(APPEND PIRANHA_MANDATORY_LINK_TARGETS MPFR::MPFR GMP::GMP)

if(PIRANHA_WITH_MSGPACK)
	find_package(MSGPACK-C REQUIRED)
	message(STATUS "msgpack-c library found.")
	message(STATUS "msgpack-c include dir is: ${MSGPACK-C_INCLUDE_DIR}")
	# msgpack load file requires memory mapping for uncompressed format.
	if(NOT PIRANHA_BOOST_MAPPED_FILE_CHECK)
		message(STATUS "Testing whether Boost's memory mapped file is supported.")
		try_compile(_PIRANHA_BOOST_MAPPED_FILE_CHECK "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/test_mmapped_file.cpp"
			LINK_LIBRARIES Boost::boost Boost::disable_autolinking Boost::iostreams)
		if(NOT _PIRANHA_BOOST_MAPPED_FILE_CHECK)
			message(FATAL_ERROR "msgpack support in Piranha requires an implementation of Boost's memory mapped file, which seems to be unsupported on this platform.")
		endif()
		# Put it into the cache, so we avoid running the check again in future CMake invocations.
		set(PIRANHA_BOOST_MAPPED_FILE_CHECK YES CACHE INTERNAL "")
	endif()
	message(STATUS "Boost's memory mapped file is supported.")
	set(PIRANHA_ENABLE_MSGPACK "#define PIRANHA_WITH_MSGPACK")
	list(APPEND PIRANHA_MANDATORY_LINK_TARGETS MSGPACK-C::MSGPACK-C)
endif()

if(PIRANHA_WITH_ZLIB)
	find_package(ZLIB REQUIRED)
	message(STATUS "zlib library found.")
	message(STATUS "zlib include dir is: ${ZLIB_INCLUDE_DIR}")
	message(STATUS "zlib library is: ${ZLIB_LIBRARIES}")
	set(PIRANHA_ENABLE_ZLIB "#define PIRANHA_WITH_ZLIB")
	list(APPEND PIRANHA_MANDATORY_LINK_TARGETS ZLIB::ZLIB)
endif()

if(PIRANHA_WITH_BZIP2)
	include(PiranhaFindBZip2)
	message(STATUS "bzip2 library found.")
	message(STATUS "bzip2 include dir is: ${BZIP2_INCLUDE_DIR}")
	message(STATUS "bzip2 library is: ${BZIP2_LIBRARIES}")
	set(PIRANHA_ENABLE_BZIP2 "#define PIRANHA_WITH_BZIP2")
	list(APPEND PIRANHA_MANDATORY_LINK_TARGETS BZip2::BZip2)
endif()

# Try to determine the git revision.
find_package(Git)
if(Git_FOUND)
	message(STATUS "Git executable: ${GIT_EXECUTABLE}")
	execute_process(COMMAND ${GIT_EXECUTABLE} "log" "--no-color" "-n1" "--date=short" "--pretty=format:%H" WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} OUTPUT_VARIABLE PIRANHA_GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
endif()
if(NOT PIRANHA_GIT_REVISION)
	set(PIRANHA_GIT_REVISION "unknown")
endif()
message(STATUS "Git revision: ${PIRANHA_GIT_REVISION}")
message(STATUS "Piranha version: ${piranha_VERSION}")

# Configure config.hpp.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" @ONLY)

# Configure Doxyfile.
set(PIRANHA_DOXYGEN_STRIP_FROM_INC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in" "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile" @ONLY)

# Configure Doxyfile.
set(PIRANHA_DOXYGEN_STRIP_FROM_INC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in" "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile" @ONLY)

# Setup the piranha interface library.
add_library(piranha INTERFACE)
target_include_directories(piranha INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_link_libraries(piranha INTERFACE ${PIRANHA_MANDATORY_LINK_TARGETS})

# Installation.
if(PIRANHA_INSTALL_HEADERS)
	# Setup of the export.
	install(TARGETS piranha EXPORT piranha_export)
	set(_PIRANHA_CONFIG_OPTIONAL_DEPS)
	if(PIRANHA_WITH_ZLIB)
		set(_PIRANHA_CONFIG_OPTIONAL_DEPS "${_PIRANHA_CONFIG_OPTIONAL_DEPS}find_package(ZLIB REQUIRED)\n")
	endif()
	if(PIRANHA_WITH_BZIP2)
		set(_PIRANHA_CONFIG_OPTIONAL_DEPS "${_PIRANHA_CONFIG_OPTIONAL_DEPS}include(PiranhaFindBZip2)\n")
	endif()
	if(PIRANHA_WITH_MSGPACK)
		set(_PIRANHA_CONFIG_OPTIONAL_DEPS "${_PIRANHA_CONFIG_OPTIONAL_DEPS}find_package(MSGPACK-C REQUIRED)\n")
		install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/FindMSGPACK-C.cmake" DESTINATION "lib/cmake/piranha")
	endif()
	configure_file("${CMAKE_CURRENT_SOURCE_DIR}/piranha-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/piranha-config.cmake" @ONLY)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/piranha-config.cmake" DESTINATION "lib/cmake/piranha")
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/FindGMP.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/FindMPFR.cmake"
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/PiranhaFindBoost.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/PiranhaFindBZip2.cmake"
		DESTINATION "lib/cmake/piranha")
	install(EXPORT piranha_export NAMESPACE Piranha:: DESTINATION lib/cmake/piranha)
	# Take care of versioning.
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/piranha-config-version.cmake" VERSION ${piranha_VERSION}
		COMPATIBILITY ExactVersion)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/piranha-config-version.cmake" DESTINATION "lib/cmake/piranha")

	# Installation of the header files.
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/piranha" DESTINATION include)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp" DESTINATION include/piranha)
endif()

# Tests and benchmarks setup.
# This is just a simple counter variable, internal use only.
set(_PIRANHA_TEST_NUM "0")
# Check splitting options. These need to be set from the command line.
# - PIRANHA_TEST_NSPLIT: number of chunks into which the unit tests will be divided (must be > 1).
# - PIRANHA_TEST_SPLIT_NUM: 0-based index of the chunk to run.
if(PIRANHA_TEST_NSPLIT AND "${PIRANHA_TEST_SPLIT_NUM}" STREQUAL "")
	message(FATAL_ERROR "Test splitting was requested, but the PIRANHA_TEST_SPLIT_NUM variable was not set.")
elseif(NOT PIRANHA_TEST_NSPLIT AND NOT "${PIRANHA_TEST_SPLIT_NUM}" STREQUAL "")
	message(FATAL_ERROR "The PIRANHA_TEST_SPLIT_NUM variable was set, but test splitting was not requested.")
endif()
if(PIRANHA_TEST_NSPLIT)
	message(STATUS "Tests will be split into ${PIRANHA_TEST_NSPLIT} chunks. The chunk with index ${PIRANHA_TEST_SPLIT_NUM} will be processed.")
endif()

if(PIRANHA_BUILD_TESTS)
	enable_testing()
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

if(PIRANHA_BUILD_BENCHMARKS)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/benchmarks")
endif()

if(PIRANHA_BUILD_TUTORIALS)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tutorials")
endif()

IF(PIRANHA_BUILD_PYRANHA)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/pyranha")
	if(MINGW)
		message(STATUS "Creating the files for the generation of a binary wheel for MinGW.")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/mingw_wheel_setup.py" "${CMAKE_CURRENT_BINARY_DIR}/wheel/setup.py" @ONLY)
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/mingw_wheel_libs_python${PYTHON_VERSION_MAJOR}.txt" "${CMAKE_CURRENT_BINARY_DIR}/wheel/mingw_wheel_libs_python${PYTHON_VERSION_MAJOR}.txt" @ONLY)
	endif()
endif()
