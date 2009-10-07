cmake_minimum_required(VERSION 2.6)
project(PDD)

include(FindPkgConfig)

pkg_check_modules(PDD glib-2.0 sqlite3 gmodule-2.0 gtk+-2.0 gconf-2.0)

find_library(YAML_LIBRARY yaml)

include_directories(${PDD_INCLUDE_DIRS} ${YAML_INCLUDE_DIR})
link_directories(${PDD_LIBRARY_DIRS} ${YAML_LIBRARY_DIR})

add_definitions(-W -Wall -Wextra -std=c99 -DPDD_SHARE_DIR="${CMAKE_INSTALL_PREFIX}/share/pdd-by")

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

if (NOT APPLE)
    set_target_properties(pdd PROPERTIES LINK_FLAGS "-export-dynamic")
endif (NOT APPLE)

install(TARGETS pdd DESTINATION bin)
install(DIRECTORY data ui DESTINATION share/pdd-by)
