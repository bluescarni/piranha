cmake_minimum_required(VERSION 3.3.0)

project(sample_project)

find_package(piranha REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main piranha::piranha)
set_property(TARGET main PROPERTY CXX_STANDARD 11)
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET main PROPERTY CXX_EXTENSIONS NO)
