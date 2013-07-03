INCLUDE(CheckCXXCompilerFlag)

# Clang detection:
# http://stackoverflow.com/questions/10046114/in-cmake-how-can-i-test-if-the-compiler-is-clang
# http://www.cmake.org/cmake/help/v2.8.10/cmake.html#variable:CMAKE_LANG_COMPILER_ID
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET(CMAKE_COMPILER_IS_CLANGXX 1)
ENDIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

# Configuration for GCC.
IF(CMAKE_COMPILER_IS_GNUCXX)
	INCLUDE(CheckTypeSize)
	MESSAGE(STATUS "GNU compiler detected, checking version.")
	TRY_COMPILE(GCC_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/gcc_check_version.cpp")
	IF(NOT GCC_VERSION_CHECK)
		MESSAGE(FATAL_ERROR "Unsupported GCC version, please upgrade your compiler.")
	ENDIF(NOT GCC_VERSION_CHECK)
	MESSAGE(STATUS "GCC version is ok.")
	CHECK_CXX_COMPILER_FLAG(-Wnon-virtual-dtor GNUCXX_VIRTUAL_DTOR)
	IF(GNUCXX_VIRTUAL_DTOR)
		MESSAGE(STATUS "Enabling '-Wnon-virtual-dtor' compiler flag.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
	ENDIF(GNUCXX_VIRTUAL_DTOR)
	CHECK_CXX_COMPILER_FLAG(-Wzero-as-null-pointer-constant GNUCXX_WZERO_AS_NULL_POINTER)
	IF(GNUCXX_WZERO_AS_NULL_POINTER)
		# NOTE: disable this for now, as it results in a lot of clutter from Boost.
		#MESSAGE(STATUS "Enabling '-Wzero-as-null-pointer-constant' compiler flag.")
		#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wzero-as-null-pointer-constant")
	ENDIF(GNUCXX_WZERO_AS_NULL_POINTER)
	MESSAGE(STATUS "Enabling c++11 flag.")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	CHECK_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden GNUCXX_VISIBILITY_INLINES_HIDDEN)
	CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden GNUCXX_VISIBILITY_HIDDEN)
	IF(GNUCXX_VISIBILITY_INLINES_HIDDEN AND GNUCXX_VISIBILITY_HIDDEN AND NOT MINGW)
		MESSAGE(STATUS "Enabling GCC visibility support.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
	ENDIF(GNUCXX_VISIBILITY_INLINES_HIDDEN AND GNUCXX_VISIBILITY_HIDDEN AND NOT MINGW)
	# Add to the base flags extra warnings.
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors -Wdisabled-optimization")
	# Try to see if we have the 128-bit integer type available.
	CHECK_TYPE_SIZE("__int128" PIRANHA_GCC___INT128)
	IF(PIRANHA_GCC___INT128)
                MESSAGE(STATUS "GCC 128-bit integer type detected.")
		SET(PIRANHA_HAVE_GCC_INT128 "#define PIRANHA_GCC_INT128_T __int128")
		SET(PIRANHA_HAVE_GCC_UINT128 "#define PIRANHA_GCC_UINT128_T unsigned __int128")
	ENDIF()
	# Enable libstdc++ pedantic debug mode in debug builds.
	# NOTE: this is disabled by default, as it requires the c++ library to be compiled with this
	# flag enabled in order to be reliable (and this is not the case usually):
	# http://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode.html
# 	IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
# 		ADD_DEFINITIONS(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
# 	ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# Some flags to curb the size of binaries in MinGW: do debug mode with -Os and without actual
	# debug information, and enable globally the inlining flag for functions.
	IF(MINGW)
		SET(CMAKE_CXX_FLAGS_DEBUG "-g0 -Os")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finline-functions")
	ENDIF(MINGW)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(CMAKE_COMPILER_IS_CLANGXX)
	MESSAGE(STATUS "Clang compiler detected, checking version.")
	TRY_COMPILE(CLANG_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/clang_check_version.cpp")
	IF(NOT CLANG_VERSION_CHECK)
		MESSAGE(FATAL_ERROR "Unsupported Clang version, please upgrade your compiler.")
	ENDIF(NOT CLANG_VERSION_CHECK)
	MESSAGE(STATUS "Clang version is ok.")
	MESSAGE(STATUS "Enabling c++11 flag.")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -Wall -Wextra")
ENDIF(CMAKE_COMPILER_IS_CLANGXX)
