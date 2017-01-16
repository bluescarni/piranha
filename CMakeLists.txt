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

# Build option: build the C++ tutorial.
option(PIRANHA_BUILD_TUTORIAL "Build the C++ tutorial." OFF)

# Build option: build the Python bindings.
option(PIRANHA_BUILD_PYRANHA "Build Pyranha, the Python bindings for Piranha." OFF)

# Build option: enable the msgpack serialization format.
option(PIRANHA_WITH_MSGPACK "Enable support for the msgpack serialization format." OFF)

# Build option: enable zlib/gzip compression.
option(PIRANHA_WITH_ZLIB "Enable support for zlib/gzip compression." OFF)

# Build option: enable bzip2 compression.
option(PIRANHA_WITH_BZIP2 "Enable support for bzip2 compression." OFF)

# Build option: enable the installation of the headers.
option(PIRANHA_INSTALL_HEADERS "Enable the installation of the headers." ON)
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

# Start to set up the list of mandatory targets to link to.
set(PIRANHA_MANDATORY_LINK_TARGETS Threads::Threads)

# Additional platform-specific setup.
include(PiranhaPlatformSettings)

# Assemble the flags.
set(PIRANHA_CXX_FLAGS_DEBUG ${YACMA_CXX_FLAGS} ${YACMA_CXX_FLAGS_DEBUG} ${YACMA_THREADING_CXX_FLAGS})
set(PIRANHA_CXX_FLAGS_RELEASE ${YACMA_CXX_FLAGS} ${YACMA_THREADING_CXX_FLAGS})
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND YACMA_COMPILER_IS_CLANGXX)
	message(STATUS "Clang compiler on OSX detected, setting the standard library to 'libc++'.")
	list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-stdlib=libc++")
	list(APPEND PIRANHA_CXX_FLAGS_RELEASE "-stdlib=libc++")
endif()
if(MINGW)
	# Curb binary size in MinGW debug builds.
	message(STATUS "Appending the '-g0 -Os' flags in MinGW debug builds.")
	list(APPEND PIRANHA_CXX_FLAGS_DEBUG "-g0" "-Os")
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
set(_PIRANHA_REQUIRED_BOOST_LIBS serialization iostreams)
if(PIRANHA_BUILD_TESTS)
	# These libraries are needed only if building tests.
	list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS filesystem system)
endif()
if(PIRANHA_BUILD_PYRANHA)
	list(APPEND _PIRANHA_REQUIRED_BOOST_LIBS python)
endif()
message(STATUS "Required Boost libraries: ${_PIRANHA_REQUIRED_BOOST_LIBS}")
find_package(Boost 1.55.0 REQUIRED COMPONENTS "${_PIRANHA_REQUIRED_BOOST_LIBS}")
message(STATUS "Detected Boost version: ${Boost_VERSION}")
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
list(APPEND PIRANHA_MANDATORY_LINK_TARGETS Boost::boost Boost::disable_autolinking Boost::serialization Boost::iostreams)

# GMP/MPFR setup.
find_package(GMP REQUIRED)
message(STATUS "GMP library found.")
message(STATUS "GMP include dir is: ${GMP_INCLUDE_DIR}")
message(STATUS "GMP library is: ${GMP_LIBRARIES}")
find_package(MPFR REQUIRED)
message(STATUS "MPFR library found.")
message(STATUS "MPFR include dir is: ${MPFR_INCLUDE_DIR}")
message(STATUS "MPFR library is: ${MPFR_LIBRARIES}")
list(APPEND PIRANHA_MANDATORY_LINK_TARGETS MPFR::MPFR GMP::GMP)

if(PIRANHA_WITH_MSGPACK)
	include(CheckTypeSize)
	find_package(MSGPACK-C REQUIRED)
	message(STATUS "msgpack-c library found.")
	message(STATUS "msgpack-c include dir is: ${MSGPACK-C_INCLUDE_DIR}")
	# msgpack load file requires memory mapping for uncompressed format.
	if(NOT PIRANHA_BOOST_MAPPED_FILE_CHECK)
		message(STATUS "Testing whether Boost's memory mapped file is supported.")
		try_compile(_PIRANHA_BOOST_MAPPED_FILE_CHECK "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/test_mmapped_file.cpp"
			LINK_LIBRARIES Boost::iostreams Boost::boost)
		if(NOT _PIRANHA_BOOST_MAPPED_FILE_CHECK)
			message(FATAL_ERROR "msgpack support in Piranha requires an implementation of Boost's memory mapped file.")
		endif()
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
	find_package(BZip2 REQUIRED)
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
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/include/piranha/config.hpp")

# Setup the piranha interface library.
add_library(piranha INTERFACE)
target_link_libraries(piranha INTERFACE ${PIRANHA_MANDATORY_LINK_TARGETS})
target_include_directories(piranha INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Installation.
if(PIRANHA_INSTALL_HEADERS)
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include" DESTINATION include)
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include" DESTINATION include)
endif()

if(PIRANHA_BUILD_TESTS)
	enable_testing()
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tests")
endif()

if(PIRANHA_BUILD_TUTORIAL)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/tutorial")
endif()

IF(PIRANHA_BUILD_PYRANHA)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/pyranha")
	if(MINGW)
		message(STATUS "Creating the files for the generation of a binary wheel for MinGW.")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/mingw_wheel_setup.py" "${CMAKE_CURRENT_BINARY_DIR}/wheel/setup.py")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/mingw_wheel_libs_python${PYTHON_VERSION_MAJOR}.txt" "${CMAKE_CURRENT_BINARY_DIR}/wheel/mingw_wheel_libs_python${PYTHON_VERSION_MAJOR}.txt")
	endif()
endif()
