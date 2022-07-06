cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

set(CMAKE_CXX_STANDARD 11)

add_executable(example src/example.cpp)

find_package(piranha CONFIG REQUIRED)
include_directories(${piranha_INCLUDE_DIRS})

find_package(Boost CONFIG REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(mpfr CONFIG REQUIRED)
include_directories(${mpfr_INCLUDE_DIRS})
target_link_libraries(example ${mpfr_LIBRARIES})

find_package(gmp CONFIG REQUIRED)
include_directories(${gmp_INCLUDE_DIRS})

find_package(mp++ CONFIG REQUIRED)
include_directories(${mp++_INCLUDE_DIRS})
