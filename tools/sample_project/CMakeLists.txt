cmake_minimum_required(VERSION 3.2.0)

project(sample_project)

find_package(Piranha REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main Piranha::piranha)
