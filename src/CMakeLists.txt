SET(SRC_LIST
	exceptions.cpp
	runtime_info.cpp
	settings.cpp
	thread_barrier.cpp
	thread_group.cpp
	thread_management.cpp
)

IF(BUILD_MAIN OR BUILD_TESTS)
	ADD_LIBRARY(piranha_static STATIC ${SRC_LIST})
	TARGET_LINK_LIBRARIES(piranha_static ${MANDATORY_LIBRARIES})
ENDIF(BUILD_MAIN OR BUILD_TESTS)
