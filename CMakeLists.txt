cmake_minimum_required(VERSION 2.6)
project(pddby-main C)

set(PDDBY_VERSION "0.4")

option(PDDBY_BUILD_GTK_FRONTEND "Build GTK frontend" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

include(FindPkgConfig)

pkg_search_module(GLIB2 REQUIRED glib-2.0)
pkg_search_module(SQLITE3 REQUIRED sqlite3)

if(PDDBY_BUILD_GTK_FRONTEND)
    pkg_search_module(GTK2 REQUIRED gtk+-2.0)
endif()

add_definitions(-W -Wall -Wextra -std=c99 -fvisibility=hidden)

if(APPLE)
    add_definitions(-D_DARWIN_PLATFORM_)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-dead_strip_dylibs")
elseif(UNIX)
    set(PDDBY_SHARE_DIR "share/pddby")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-as-needed")
endif()

configure_file(config.h.in ${${PROJECT_NAME}_BINARY_DIR}/config.h)
include_directories(
    ${${PROJECT_NAME}_BINARY_DIR}
    ${${PROJECT_NAME}_SOURCE_DIR}
)

add_subdirectory(pddby)

if(PDDBY_BUILD_GTK_FRONTEND)
    add_subdirectory(pddby-gtk)
endif()

message(STATUS "------------------------------")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Options: GTK(${PDDBY_BUILD_GTK_FRONTEND})")
message(STATUS "------------------------------")
