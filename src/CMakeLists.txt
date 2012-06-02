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

ADD_LIBRARY(piranha STATIC ${SRC_LIST})
TARGET_LINK_LIBRARIES(piranha ${MANDATORY_LIBRARIES})
