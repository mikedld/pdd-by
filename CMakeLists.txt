cmake_minimum_required(VERSION 2.6)
project(PDD)

include(FindPkgConfig)

pkg_check_modules(PDD glib-2.0 sqlite3 gmodule-2.0 gtk+-2.0 gconf-2.0)

find_library(YAML_LIBRARY yaml)

include_directories(${PDD_INCLUDE_DIRS} ${YAML_INCLUDE_DIR})
link_directories(${PDD_LIBRARY_DIRS} ${YAML_LIBRARY_DIR})

if (APPLE)
    set(PDD_TARGET_NAME "ПДД-Беларусь")
    set(PDD_BINDIR_NAME MacOS)
    set(PDD_SHAREDIR_NAME Resources)
    set(CMAKE_INSTALL_PREFIX "/Applications/${PDD_TARGET_NAME}.app/Contents")
else (APPLE)
    set(PDD_TARGET_NAME pdd-by)
    set(PDD_BINDIR_NAME bin)
    set(PDD_SHAREDIR_NAME share/pdd-by)
endif (APPLE)

add_definitions(-W -Wall -Wextra -std=c99 -fvisibility=hidden -DPDD_SHARE_DIR="${CMAKE_INSTALL_PREFIX}/${PDD_SHAREDIR_NAME}")

option(DEBUG "debug build" OFF)

if(DEBUG)
    add_definitions(-g -ggdb)
endif(DEBUG)

add_executable(${PDD_TARGET_NAME}
    answer.c
    chooser_dialog.c
    comment.c
    database.c
    decode.c
    delphi_helper.c
    help_dialog.c
    image.c
    main.c
    main_window.c
    question.c
    question_window.c
    section.c
    ticket_dialog.c
    topic.c
    traffreg.c
    yaml_helper.c
)

target_link_libraries(${PDD_TARGET_NAME} ${PDD_LIBRARIES} ${YAML_LIBRARY})

if (NOT APPLE)
    set_target_properties(${PDD_TARGET_NAME} PROPERTIES LINK_FLAGS "-export-dynamic")
endif (NOT APPLE)

install(TARGETS ${PDD_TARGET_NAME} DESTINATION ${PDD_BINDIR_NAME})
install(DIRECTORY data ui DESTINATION ${PDD_SHAREDIR_NAME})
