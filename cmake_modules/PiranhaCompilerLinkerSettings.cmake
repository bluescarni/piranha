INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CheckTypeSize)

message(STATUS "The C++ compiler ID is: ${CMAKE_CXX_COMPILER_ID}")

# Clang detection:
# http://stackoverflow.com/questions/10046114/in-cmake-how-can-i-test-if-the-compiler-is-clang
# http://www.cmake.org/cmake/help/v2.8.10/cmake.html#variable:CMAKE_LANG_COMPILER_ID
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET(CMAKE_COMPILER_IS_CLANGXX 1)
ENDIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	set(CMAKE_COMPILER_IS_INTELXX 1)
endif()

macro(PIRANHA_CHECK_ENABLE_CXX_FLAG flag)
	set(PIRANHA_CHECK_CXX_FLAG)
	check_cxx_compiler_flag("${flag}" PIRANHA_CHECK_CXX_FLAG)
	if(PIRANHA_CHECK_CXX_FLAG)
		message(STATUS "Enabling the '${flag}' compiler flag.")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
	else()
		message(STATUS "Disabling the '${flag}' compiler flag.")
	endif()
	unset(PIRANHA_CHECK_CXX_FLAG CACHE)
endmacro()

macro(PIRANHA_CHECK_ENABLE_DEBUG_CXX_FLAG flag)
	set(PIRANHA_CHECK_DEBUG_CXX_FLAG)
	check_cxx_compiler_flag("${flag}" PIRANHA_CHECK_DEBUG_CXX_FLAG)
	if(PIRANHA_CHECK_DEBUG_CXX_FLAG)
		message(STATUS "Enabling the '${flag}' debug compiler flag.")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${flag}")
	else()
		message(STATUS "Disabling the '${flag}' debug compiler flag.")
	endif()
	unset(PIRANHA_CHECK_DEBUG_CXX_FLAG CACHE)
endmacro()

# Macro to detect the 128-bit unsigned integer type available on some compilers.
macro(PIRANHA_CHECK_UINT128_T)
	MESSAGE(STATUS "Looking for a 128-bit unsigned integer type.")
	# NOTE: for now we support only the GCC integer.
	# NOTE: use this instead of the unsigned __int128. See:
	# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50454
	CHECK_TYPE_SIZE("__uint128_t" PIRANHA_UINT128_T)
	IF(PIRANHA_UINT128_T)
		MESSAGE(STATUS "128-bit unsigned integer type detected.")
		SET(PIRANHA_HAVE_UINT128_T "#define PIRANHA_UINT128_T __uint128_t")
	ELSE()
		MESSAGE(STATUS "No 128-bit unsigned integer type detected.")
	ENDIF()
endmacro(PIRANHA_CHECK_UINT128_T)

# Configuration for GCC.
IF(CMAKE_COMPILER_IS_GNUCXX)
	MESSAGE(STATUS "GNU compiler detected, checking version.")
	TRY_COMPILE(GCC_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/gcc_check_version.cpp")
	IF(NOT GCC_VERSION_CHECK)
		MESSAGE(FATAL_ERROR "Unsupported GCC version, please upgrade your compiler.")
	ENDIF(NOT GCC_VERSION_CHECK)
	MESSAGE(STATUS "GCC version is ok.")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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
	PIRANHA_CHECK_UINT128_T()
	# Color diagnostic available since GCC 4.9.
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-fdiagnostics-color=auto)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(CMAKE_COMPILER_IS_CLANGXX)
	MESSAGE(STATUS "Clang compiler detected, checking version.")
	TRY_COMPILE(CLANG_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/clang_check_version.cpp")
	IF(NOT CLANG_VERSION_CHECK)
		MESSAGE(FATAL_ERROR "Unsupported Clang version, please upgrade your compiler.")
	ENDIF(NOT CLANG_VERSION_CHECK)
	MESSAGE(STATUS "Clang version is ok.")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	# This used to be necessary with earlier versions of Clang which
	# were not completely compatible with GCC's stdlib. Nowadays it seems
	# unnecessary on most platforms.
	# PIRANHA_CHECK_ENABLE_CXX_FLAG(-stdlib=libc++)
	PIRANHA_CHECK_UINT128_T()
ENDIF(CMAKE_COMPILER_IS_CLANGXX)

if(CMAKE_COMPILER_IS_INTELXX)
	message(STATUS "Intel compiler detected, checking version.")
	try_compile(INTEL_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/intel_check_version.cpp")
	if(NOT INTEL_VERSION_CHECK)
		message(FATAL_ERROR "Unsupported Intel compiler version, please upgrade your compiler.")
	endif()
	message(STATUS "Intel compiler version is ok.")
	# The diagnostic from the Intel compiler can be wrong and a pain in the ass.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -diag-disable 2304,2305,1682,2259,3373")
	PIRANHA_CHECK_UINT128_T()
endif()

# Common configuration for GCC, Clang and Intel.
if(CMAKE_COMPILER_IS_CLANGXX OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_INTELXX)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wall)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wextra)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wnon-virtual-dtor)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wnoexcept)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wlogical-op)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wconversion)
	# NOTE: this can be useful, but at the moment it triggers lots of warnings in type traits.
	# Keep it in mind for the next time we touch type traits.
	# PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wold-style-cast)
	# NOTE: disable this for now, as it results in a lot of clutter from Boost.
	# PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wzero-as-null-pointer-constant)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-pedantic-errors)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-Wdisabled-optimization)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-fvisibility-inlines-hidden)
	PIRANHA_CHECK_ENABLE_CXX_FLAG(-fvisibility=hidden)
	# This is useful when the compiler decides the template backtrace is too verbose.
	PIRANHA_CHECK_ENABLE_DEBUG_CXX_FLAG(-ftemplate-backtrace-limit=0)
	PIRANHA_CHECK_ENABLE_DEBUG_CXX_FLAG(-fstack-protector-all)
endif()
