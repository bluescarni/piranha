include(YACMACompilerLinkerSettings)

IF(UNIX)
	# Install path for libraries.
	SET(LIB_INSTALL_PATH "lib")
	TRY_COMPILE(PIRANHA_POSIX_MEMALIGN_TEST ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/posix_memalign_test.cpp")
	IF(PIRANHA_POSIX_MEMALIGN_TEST)
		MESSAGE(STATUS "POSIX memalign detected.")
		SET(PIRANHA_POSIX_MEMALIGN "#define PIRANHA_HAVE_POSIX_MEMALIGN")
	ELSE()
		MESSAGE(STATUS "POSIX memalign not detected.")
	ENDIF()
ENDIF()

IF(MINGW)
	# NOTE: workaround for CMake being unable to locate Boost libraries in certain
	# configurations. See:
	# http://www.ogre3d.org/tikiwiki/Setting%20Up%20An%20Application%20-%20Mac%20OSX
	# http://www.gccxml.org/Bug/view.php?id=9865
	SET(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "")
ENDIF()

# Setup for the machinery to detect cache line size in Windows. It's not supported everywhere, so we
# check for the existence of the SYSTEM_LOGICAL_PROCESSOR_INFORMATION type.
# http://msdn.microsoft.com/en-us/library/ms686694(v=vs.85).aspx
IF(WIN32)
	INCLUDE(CheckTypeSize)
	SET(CMAKE_EXTRA_INCLUDE_FILES Windows.h)
	CHECK_TYPE_SIZE("SYSTEM_LOGICAL_PROCESSOR_INFORMATION" SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
	# Clear the variable.
	UNSET(CMAKE_EXTRA_INCLUDE_FILES)
	IF(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
		MESSAGE(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type found.")
		SET(PIRANHA_SYSTEM_LOGICAL_PROCESSOR_INFORMATION "#define PIRANHA_HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION")
	ELSE()
		MESSAGE(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type not found.")
	ENDIF()
	UNSET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
ENDIF()

IF(UNIX OR MINGW)
	# NOTE: here it seems like the CFLAGS for the TRY_COMPILE are inherited from the ones already defined,
	# so no need to define them explicitly.
	TRY_COMPILE(PIRANHA_IGNORE_LIBM ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/test_libm.cpp")
	IF(NOT PIRANHA_IGNORE_LIBM)
		MESSAGE(STATUS "Explicit linking to the math library is needed.")
		FIND_LIBRARY(SYSTEM_M_LIBRARY NAMES m)
		IF(SYSTEM_M_LIBRARY)
			SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${SYSTEM_M_LIBRARY})
			MESSAGE(STATUS "Math library found: ${SYSTEM_M_LIBRARY}")
		ELSE()
			MESSAGE(FATAL_ERROR "Math library not found, aborting.")
		ENDIF()
	ELSE()
		MESSAGE(STATUS "Explicit linking to the math library is not needed.")
	ENDIF()
ENDIF()

# OS X setup.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	IF(YACMA_COMPILER_IS_CLANGXX)
		# On OS X with clang we need to use libc++.
		MESSAGE(STATUS "Clang compiler on OS X detected, using libc++ as standard library.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	ENDIF()
ENDIF()
