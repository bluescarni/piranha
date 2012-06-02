SET(SRC_LIST
	degree_truncator_settings.cpp
	exceptions.cpp
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
	TARGET_LINK_LIBRARIES(pyranha ${MANDATORY_LIBRARIES})
	# TODO this needs to be fixed in Windows.
	INSTALL(TARGETS pyranha
		RUNTIME DESTINATION ${LIB_INSTALL_PATH}
		LIBRARY DESTINATION ${LIB_INSTALL_PATH}
	)
ENDIF(BUILD_PYRANHA)
