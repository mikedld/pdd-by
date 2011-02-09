cmake_minimum_required(VERSION 2.6)
project(pddby-main C)

set(PDDBY_VERSION "0.4")

option(PDDBY_BUILD_GTK_FRONTEND "Build GTK frontend" ON)
option(PDDBY_BUILD_QT_FRONTEND "Build Qt frontend" OFF)
option(PDDBY_BUILD_COCOA_FRONTEND "Build Cocoa frontend" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

list(APPEND CMAKE_MODULE_PATH "${${PROJECT_NAME}_SOURCE_DIR}")

find_package(SQLite3 REQUIRED)
find_package(PCRE REQUIRED)
find_package(OpenSSL REQUIRED)

if(PDDBY_BUILD_GTK_FRONTEND)
    find_package(GTK2 REQUIRED COMPONENTS gtk)
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

message(STATUS "----------------------------------------")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Options:"
    " GTK(${PDDBY_BUILD_GTK_FRONTEND})"
    " QT(${PDDBY_BUILD_QT_FRONTEND})"
    " COCOA(${PDDBY_BUILD_COCOA_FRONTEND})")
message(STATUS "----------------------------------------")
