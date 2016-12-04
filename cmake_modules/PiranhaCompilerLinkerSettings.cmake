include(YACMACompilerLinkerSettings)
include(CheckTypeSize)

# Macro to detect the 128-bit unsigned integer type available on some compilers.
macro(PIRANHA_CHECK_UINT128_T)
	message(STATUS "Looking for a 128-bit unsigned integer type.")
	# NOTE: for now we support only the GCC integer.
	# NOTE: use this instead of the unsigned __int128. See:
	# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=50454
	CHECK_TYPE_SIZE("__uint128_t" PIRANHA_UINT128_T)
	if(PIRANHA_UINT128_T)
		message(STATUS "128-bit unsigned integer type detected.")
		set(PIRANHA_HAVE_UINT128_T "#define PIRANHA_UINT128_T __uint128_t")
	else()
		message(STATUS "No 128-bit unsigned integer type detected.")
	endif()
endmacro()

# Setup the C++ standard flag. We try C++14 first, if not available we go with C++11.
macro(PIRANHA_SET_CPP_STD_FLAG)
	# This is valid for GCC, and clang and Intel on unix. I think that MSVC has the std version hardcoded.
	# In Windows we might need a different flag format?
	if(YACMA_COMPILER_IS_GNUCXX OR (UNIX AND (YACMA_COMPILER_IS_CLANGXX OR YACMA_COMPILER_IS_INTELXX)))
		set(PIRANHA_CHECK_CXX_FLAG)
		check_cxx_compiler_flag("-std=c++14" PIRANHA_CHECK_CXX_FLAG)
		if(PIRANHA_CHECK_CXX_FLAG)
			message(STATUS "C++14 supported by the compiler, enabling.")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
		else()
			message(STATUS "C++14 is not supported by the compiler, C++11 will be enabled.")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
		endif()
	endif()
endmacro()

# Configuration for GCC.
if(YACMA_COMPILER_IS_GNUCXX)
	message(STATUS "GNU compiler detected, checking version.")
	try_compile(PIRANHA_GCC_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/gcc_check_version.cpp")
	if(NOT PIRANHA_GCC_VERSION_CHECK)
		message(FATAL_ERROR "Unsupported GCC version, please upgrade your compiler.")
	endif()
	message(STATUS "GCC version is ok.")
	# The trouble here is that -g (which implies -g2) results in ICE in some tests and in
	# some pyranha exposition cases. We just append -g1 here, which overrides the default -g.
	# For MinGW, we disable debug info altogether.
	if (MINGW)
		message(STATUS "Forcing the debug flag to -g0 for GCC.")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g0")
	else()
		message(STATUS "Forcing the debug flag to -g1 for GCC.")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g1")
	endif()
	# Set the standard flag.
	PIRANHA_SET_CPP_STD_FLAG()
	# Enable libstdc++ pedantic debug mode in debug builds.
	# NOTE: this is disabled by default, as it requires the c++ library to be compiled with this
	# flag enabled in order to be reliable (and this is not the case usually):
	# http://gcc.gnu.org/onlinedocs/libstdc++/manual/debug_mode.html
# 	IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
# 		ADD_DEFINITIONS(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
# 	ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# A problem with MinGW is that we run into a "too many sections" quite often. Recently, MinGW
	# added a -mbig-obj for the assembler that solves this, but this is available only in recent versions.
	# Other tentative workarounds include disabling debug information, enabling global inlining, use -Os and others.
	if(MINGW)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
		# The debug build can run into a "file is too large" error. Work around by
		# compiling for small size.
		set(CMAKE_CXX_FLAGS_DEBUG "-Os")
		#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finline-functions")
	endif()
	PIRANHA_CHECK_UINT128_T()
endif()

if(YACMA_COMPILER_IS_CLANGXX)
	message(STATUS "Clang compiler detected, checking version.")
	try_compile(PIRANHA_CLANG_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/clang_check_version.cpp")
	if(NOT PIRANHA_CLANG_VERSION_CHECK)
		message(FATAL_ERROR "Unsupported Clang version, please upgrade your compiler.")
	endif()
	message(STATUS "Clang version is ok.")
	# Set the standard flag.
	PIRANHA_SET_CPP_STD_FLAG()
	PIRANHA_CHECK_UINT128_T()
endif()

if(YACMA_COMPILER_IS_INTELXX)
	message(STATUS "Intel compiler detected, checking version.")
	try_compile(PIRANHA_INTEL_VERSION_CHECK ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/intel_check_version.cpp")
	if(NOT PIRANHA_INTEL_VERSION_CHECK)
		message(FATAL_ERROR "Unsupported Intel compiler version, please upgrade your compiler.")
	endif()
	message(STATUS "Intel compiler version is ok.")
	# Set the standard flag.
	PIRANHA_SET_CPP_STD_FLAG()
	# The diagnostic from the Intel compiler can be wrong and a pain in the ass.
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -diag-disable 2304,2305,1682,2259,3373")
	PIRANHA_CHECK_UINT128_T()
endif()

# Setup the CXX flags from yacma.
YACMA_SETUP_CXX_FLAGS()
