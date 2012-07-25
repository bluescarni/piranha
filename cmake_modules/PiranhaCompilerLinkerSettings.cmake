INCLUDE(CheckCXXCompilerFlag)

# Clang detection:
# http://stackoverflow.com/questions/10046114/in-cmake-how-can-i-test-if-the-compiler-is-clang
IF("${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}" MATCHES ".*clang")
	MESSAGE(STATUS "Clang compiler detected.")
	SET(CMAKE_COMPILER_IS_CLANGXX 1)
ENDIF("${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1}" MATCHES ".*clang")

# Configuration for GCC.
IF(CMAKE_COMPILER_IS_GNUCXX)
	INCLUDE(CheckTypeSize)
	MESSAGE(STATUS "GNU compiler detected.")
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
	CHECK_CXX_COMPILER_FLAG(-std=c++11 GNUCXX_STD_CPP11)
	IF(GNUCXX_STD_CPP11)
		MESSAGE(STATUS "c++11 support detected.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	ELSE(GNUCXX_STD_CPP11)
		CHECK_CXX_COMPILER_FLAG(-std=c++0x GNUCXX_STD_CPP0X)
		IF(GNUCXX_STD_CPP0X)
			MESSAGE(STATUS "c++11 support detected.")
			SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		ELSE(GNUCXX_STD_CPP0X)
			MESSAGE(FATAL_ERROR "c++11 support could not be detected.")
		ENDIF(GNUCXX_STD_CPP0X)
	ENDIF(GNUCXX_STD_CPP11)
	CHECK_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden GNUCXX_VISIBILITY_INLINES_HIDDEN)
	CHECK_CXX_COMPILER_FLAG(-fvisibility=hidden GNUCXX_VISIBILITY_HIDDEN)
	IF(GNUCXX_VISIBILITY_INLINES_HIDDEN AND GNUCXX_VISIBILITY_HIDDEN AND NOT MINGW)
		MESSAGE(STATUS "Enabling GCC visibility support.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
	ENDIF(GNUCXX_VISIBILITY_INLINES_HIDDEN AND GNUCXX_VISIBILITY_HIDDEN AND NOT MINGW)
	# Add to the base flags extra warnings. Also, additional flags to turn off some GCC warnings that in practice clutter the compilation output.
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors -Wdisabled-optimization")
	# Suggested for multithreaded code.
	ADD_DEFINITIONS(-D_REENTRANT)
	# When compiling with GCC, enable the GNU extensions - used for instance in thread affinity settings.
	ADD_DEFINITIONS(-D_GNU_SOURCE)
	# Try to see if we have the 128-bit integer type available. Can be either
	# __int128_t or __int128 (in later versions).
	CHECK_TYPE_SIZE("__int128_t" PIRANHA_GCC___INT128_T)
	IF(PIRANHA_GCC___INT128_T)
		SET(PIRANHA_HAVE_GCC_INT128 "#define PIRANHA_GCC_INT128_T __int128_t")
		SET(PIRANHA_HAVE_GCC_UINT128 "#define PIRANHA_GCC_UINT128_T __uint128_t")
	ELSE(PIRANHA_GCC___INT128_T)
		CHECK_TYPE_SIZE("__int128" PIRANHA_GCC___INT128)
		IF(PIRANHA_GCC___INT128)
			SET(PIRANHA_HAVE_GCC_INT128 "#define PIRANHA_GCC_INT128_T __int128")
			SET(PIRANHA_HAVE_GCC_UINT128 "#define PIRANHA_GCC_UINT128_T unsigned __int128")
		ENDIF(PIRANHA_GCC___INT128)
	ENDIF(PIRANHA_GCC___INT128_T)
	IF(PIRANHA_HAVE_GCC_INT128)
		MESSAGE(STATUS "GCC 128-bit integer type detected.")
	ENDIF(PIRANHA_HAVE_GCC_INT128)
	# Enable libstdc++ pedantic debug mode in debug builds.
	# NOTE: this is disabled by default, as it requires the c++ library to be compiled with this
	# flag enabled in order to be reliable (and this is not the case usually):
	# http://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode.html
# 	IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
# 		ADD_DEFINITIONS(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
# 	ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# Disable the -g flag and enable the -Os flag on MinGW in debug mode in order to reduce the size
	# of the executables and make them compilable.
	IF(MINGW)
		SET(CMAKE_CXX_FLAGS_DEBUG "-Os")
	ENDIF(MINGW)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(CMAKE_COMPILER_IS_CLANGXX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -Wall -Wextra")
ENDIF(CMAKE_COMPILER_IS_CLANGXX)
