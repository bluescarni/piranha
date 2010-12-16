# Add the unit test library.
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

ADD_EXECUTABLE(runtime_info runtime_info.cpp)
TARGET_LINK_LIBRARIES(runtime_info piranha_static ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
ADD_TEST(runtime_info runtime_info)

ADD_EXECUTABLE(thread_group thread_group.cpp)
TARGET_LINK_LIBRARIES(thread_group piranha_static ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
ADD_TEST(thread_group thread_group)

ADD_EXECUTABLE(thread_management thread_management.cpp)
TARGET_LINK_LIBRARIES(thread_management piranha_static ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
ADD_TEST(thread_management thread_management)
