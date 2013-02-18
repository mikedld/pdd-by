project(pddby-project)
cmake_minimum_required(VERSION 2.8)

add_definitions(-std=c++98
    -Wall -Wextra -Werror -pedantic
    -Wno-long-long)

set(CMAKE_OSX_ARCHITECTURES x86_64)

find_package(Boost 1.49 REQUIRED filesystem system unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(include/pddby)

add_subdirectory(lib)
add_subdirectory(qt)
add_subdirectory(test)
