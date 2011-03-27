cmake_minimum_required(VERSION 2.6)
project(pddby-main C)

set(PDDBY_VERSION "0.4")

set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PDDBY_VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2009-2011 mike.dld")

option(PDDBY_FRONTEND_GTK "Build GTK frontend." ON)
option(PDDBY_FRONTEND_QT "Build Qt frontend." OFF)
option(PDDBY_FRONTEND_COCOA "Build Cocoa frontend." OFF)

option(PDDBY_STATIC_LIBS "Install static libraries." ON)

if(APPLE)
    set(_conv_backend "cfstring")
else()
    set(_conv_backend "iconv")
endif()

set(PDDBY_BACKEND_CONV "${_conv_backend}" CACHE STRING "Charset conversion backend (iconv/cfstring).")
set(PDDBY_BACKEND_REGEX "pcre" CACHE STRING "Regular expressions backend (pcre).")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (None/Debug/Release/RelWithDebInfo/MinSizeRel)." FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${${PROJECT_NAME}_SOURCE_DIR}/CMake")

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
if(PDDBY_BACKEND_CONV STREQUAL "iconv")
    find_package(Iconv REQUIRED)
endif()
if(PDDBY_BACKEND_REGEX STREQUAL "pcre")
    find_package(PCRE REQUIRED)
endif()

include(TestBigEndian)
test_big_endian(PDDBY_BIG_ENDIAN)

add_definitions(-W -Wall -Wextra -fvisibility=hidden -D_GNU_SOURCE)

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

if(PDDBY_FRONTEND_GTK)
    add_subdirectory(pddby-gtk)
endif()
if(PDDBY_FRONTEND_QT)
    enable_language(CXX)
    add_subdirectory(pddby-qt)
endif()
if(PDDBY_FRONTEND_COCOA)
    add_subdirectory(pddby-cocoa)
endif()

message(STATUS "----------------------------------------")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Frontends:"
    " gtk(${PDDBY_FRONTEND_GTK})"
    " qt(${PDDBY_FRONTEND_QT})"
    " cocoa(${PDDBY_FRONTEND_COCOA})")
message(STATUS "Backends:"
    " conv(${PDDBY_BACKEND_CONV})"
    " regex(${PDDBY_BACKEND_REGEX})")
message(STATUS "----------------------------------------")
