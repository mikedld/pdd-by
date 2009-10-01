cmake_minimum_required(VERSION 2.6)
project(PDD)

include(FindPkgConfig)

pkg_check_modules(PDD glib-2.0 sqlite3 gmodule-2.0 gtk+-2.0)

find_library(YAML_LIBRARY yaml)

include_directories(${PDD_INCLUDE_DIRS} ${YAML_INCLUDE_DIR})
link_directories(${PDD_LIBRARY_DIRS} ${YAML_LIBRARY_DIR})

add_executable(pdd
    answer.c
    chooser_dialog.c
    comment.c
    database.c
    decode.c
    delphi_helper.c
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

target_link_libraries(pdd ${PDD_LIBRARIES} ${YAML_LIBRARY})

if(NOT APPLE)
set_target_properties (pdd
PROPERTIES
    LINK_FLAGS "-export-dynamic"
)
endif(NOT APPLE)
