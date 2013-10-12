# POSIX thread setup. Intended both for UNIX and Windows (the latter when using some sort of
# pthread emulation/wrapper like pthreads-win32).
IF(CMAKE_USE_PTHREADS_INIT)
	MESSAGE(STATUS "POSIX threads detected.")
	# For POSIX threads, we try to see if the compiler accepts the -pthread flag. It is a bit of a kludge,
	# but I do not have any better idea at the moment. The situation is hairy, e.g.,
	# different systems require different GCC flags:
	# http://gcc.gnu.org/onlinedocs/libstdc++/manual/using_concurrency.html
	CHECK_CXX_COMPILER_FLAG(-pthread PIRANHA_PTHREAD_COMPILER_FLAG)
	# NOTE: we do not enable the -pthread flag on OS X as it is apparently ignored.
	IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND PIRANHA_PTHREAD_COMPILER_FLAG)
		MESSAGE(STATUS "Enabling the -pthread compiler flag.")
		# NOTE: according to GCC docs, this sets the flag for both compiler and linker. This should
		# work similarly for clang as well.
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	ENDIF()
	TRY_COMPILE(PIRANHA_PTHREAD_AFFINITY_TESTS ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/pthread_affinity_tests.cpp")
	IF(PIRANHA_PTHREAD_AFFINITY_TESTS)
		MESSAGE(STATUS "POSIX threads affinity extensions detected.")
		SET(PIRANHA_PTHREAD_AFFINITY "#define PIRANHA_HAVE_PTHREAD_AFFINITY")
	ELSE()
		MESSAGE(STATUS "POSIX threads affinity extensions not detected.")
	ENDIF()
ENDIF()

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
ENDIF(UNIX)

IF(MINGW)
	IF(NOT CMAKE_USE_PTHREADS_INIT)
		# NOTE: the idea here is that the -mthreads flag is useful only when using the native Windows
		# threads, apparently if we are using some pthread variant it is not needed:
		# http://mingw-users.1079350.n2.nabble.com/pthread-vs-mthreads-td7114500.html
		MESSAGE(STATUS "Native Windows threads detected on MinGW, enabling the -mthreads flag.")
		# The -mthreads flag is needed both in compiling and linking.
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mthreads")
		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -mthreads")
		SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -mthreads")
	ENDIF()
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
	SET(CMAKE_EXTRA_INCLUDE_FILES)
	IF(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
		MESSAGE(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type found.")
		SET(PIRANHA_SYSTEM_LOGICAL_PROCESSOR_INFORMATION "#define PIRANHA_HAVE_SYSTEM_LOGICAL_PROCESSOR_INFORMATION")
	ELSE(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
		MESSAGE(STATUS "SYSTEM_LOGICAL_PROCESSOR_INFORMATION type not found.")
	ENDIF(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
	SET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)
ENDIF(WIN32)

IF(UNIX OR MINGW)
	# NOTE: here it seems like the CFLAGS for the TRY_COMPILE are inherited from the ones already defined,
	# so no need to define the explicitly.
	TRY_COMPILE(PIRANHA_IGNORE_LIBM ${CMAKE_BINARY_DIR} "${CMAKE_SOURCE_DIR}/cmake_modules/test_libm.cpp")
	IF(NOT PIRANHA_IGNORE_LIBM)
		MESSAGE(STATUS "Explicit linking to the math library is needed.")
		FIND_LIBRARY(SYSTEM_M_LIBRARY NAMES m)
		IF(SYSTEM_M_LIBRARY)
			SET(MANDATORY_LIBRARIES ${MANDATORY_LIBRARIES} ${SYSTEM_M_LIBRARY})
			MESSAGE(STATUS "Math library found: ${SYSTEM_M_LIBRARY}")
		ELSE(SYSTEM_M_LIBRARY)
			MESSAGE(FATAL_ERROR "Math library not found, aborting.")
		ENDIF(SYSTEM_M_LIBRARY)
	ELSE(NOT PIRANHA_IGNORE_LIBM)
		MESSAGE(STATUS "Explicit linking to the math library is not needed.")
	ENDIF(NOT PIRANHA_IGNORE_LIBM)
ENDIF(UNIX OR MINGW)

# OS X setup.
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	IF(CMAKE_COMPILER_IS_CLANGXX)
		# On OS X with clang we need to use libc++.
		MESSAGE(STATUS "Clang compiler on OS X detected, using libc++ as standard library.")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	ENDIF()
ENDIF()
