SET(SRC_LIST
	degree_truncator_settings.cpp
	exceptions.cpp
	real.cpp
	runtime_info.cpp
	settings.cpp
	symbol.cpp
	thread_barrier.cpp
	thread_management.cpp
	tracing.cpp
)

IF(BUILD_MAIN OR BUILD_TESTS)
	ADD_LIBRARY(piranha STATIC ${SRC_LIST})
	TARGET_LINK_LIBRARIES(piranha ${MANDATORY_LIBRARIES})
ENDIF(BUILD_MAIN OR BUILD_TESTS)

IF(BUILD_PYRANHA)
	ADD_LIBRARY(pyranha SHARED ${SRC_LIST})
	# NOTE: Python < 3 is not strict C, as per:
	# http://www.python.org/dev/peps/pep-3123/
	# When using GCC, we add -fno-strict-aliasing here and when compiling core.cpp to prevent possible problems.
	# Other compilers should probably use similar flags.
	IF(${PYTHON_LIBRARY_VERSION} LESS 30 AND CMAKE_COMPILER_IS_GNUCXX)
		MESSAGE(STATUS "Python < 3 version detected: Pyranha will be compiled with the '-fno-strict-aliasing' flag.")
		SET_TARGET_PROPERTIES(pyranha PROPERTIES COMPILE_FLAGS "-fno-strict-aliasing" LINK_FLAGS "-fno-strict-aliasing"
			COMPILE_DEFINITIONS "PIRANHA_BUILDING_DLL")
	ENDIF(${PYTHON_LIBRARY_VERSION} LESS 30 AND CMAKE_COMPILER_IS_GNUCXX)
	TARGET_LINK_LIBRARIES(pyranha ${MANDATORY_LIBRARIES})
	# TODO this needs to be fixed in Windows.
	IF(NOT WIN32)
		INSTALL(TARGETS pyranha
			RUNTIME DESTINATION ${LIB_INSTALL_PATH}
			LIBRARY DESTINATION ${LIB_INSTALL_PATH}
		)
	ENDIF(NOT WIN32)
ENDIF(BUILD_PYRANHA)
