cmake_minimum_required(VERSION 2.6)
project(pddby-main)

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

#if (APPLE)
#    set(PDD_TARGET_NAME "ПДД-Беларусь")
#    set(PDD_BINDIR_NAME MacOS)
#    set(PDD_SHAREDIR_NAME Resources)
#    set(CMAKE_INSTALL_PREFIX "/Applications/${PDD_TARGET_NAME}.app/Contents")
#else (APPLE)
#    set(PDD_TARGET_NAME pdd-by)
#    set(PDD_BINDIR_NAME bin)
#    set(PDD_SHAREDIR_NAME share/pdd-by)
#endif (APPLE)

add_definitions(
    -W -Wall -Wextra -Werror -std=c99 -fvisibility=hidden
    -DPDD_SHARE_DIR="${CMAKE_INSTALL_PREFIX}/${PDD_SHAREDIR_NAME}"
)

if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-dead_strip_dylibs")
elseif(UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-as-needed")
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

#install(TARGETS ${PDD_TARGET_NAME} DESTINATION ${PDD_BINDIR_NAME})
#install(DIRECTORY data ui DESTINATION ${PDD_SHAREDIR_NAME})
