SET(SRC_LIST
	exceptions.cpp
	runtime_info.cpp
	settings.cpp
	symbol.cpp
	thread_barrier.cpp
	thread_group.cpp
	thread_management.cpp
	tracing.cpp
)

IF(BUILD_MAIN OR BUILD_TESTS)
	ADD_LIBRARY(piranha_static STATIC ${SRC_LIST})
	TARGET_LINK_LIBRARIES(piranha_static ${MANDATORY_LIBRARIES})
ENDIF(BUILD_MAIN OR BUILD_TESTS)

IF(BUILD_PYRANHA)
	ADD_LIBRARY(piranha SHARED ${SRC_LIST})
	TARGET_LINK_LIBRARIES(piranha ${MANDATORY_LIBRARIES})
	# TODO this needs to be fixed in Windows.
	INSTALL(TARGETS piranha
		RUNTIME DESTINATION ${LIB_INSTALL_PATH}
		LIBRARY DESTINATION ${LIB_INSTALL_PATH}
	)
ENDIF(BUILD_PYRANHA)
