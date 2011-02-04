project(pddby-gtk C)

set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "0.3")
set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2009-2011 mike.dld")

set(${PROJECT_NAME}_HEADERS
    chooser_dialog.h
    help_dialog.h
    main_window.h
    question_window.h
    settings.h
    ticket_dialog.h
)

set(${PROJECT_NAME}_SOURCES
    chooser_dialog.c
    help_dialog.c
    main.c
    main_window.c
    question_window.c
    settings.c
    ticket_dialog.c
)

set(${PROJECT_NAME}_UI_FILES
    ui/chooser_dialog.ui
    ui/help_dialog.ui
    ui/main_window.ui
    ui/question_window.ui
    ui/ticket_dialog.ui
)

set_source_files_properties(${${PROJECT_NAME}_UI_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/gtk
)

set_source_files_properties(${pddby_SQL_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources/data
)

include_directories(
    ${GTK2_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

link_directories(
    ${GTK2_LIBRARY_DIRS}
    ${SQLITE3_LIBRARY_DIRS}
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_UI_FILES}
    ${pddby_SQL_FILES}
)

add_dependencies(${PROJECT_NAME}
    pddby
)

target_link_libraries(${PROJECT_NAME}
    pddby
    ${GTK2_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        "-framework CoreFoundation"
    )
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-export-dynamic"
    )
endif()
