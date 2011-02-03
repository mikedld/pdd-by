project(pddby-gtk C)

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

include_directories(
    ${GTK2_INCLUDE_DIRS}
)

link_directories(
    ${GTK2_LIBRARY_DIRS}
    ${SQLITE3_LIBRARY_DIRS}
)

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_UI_FILES}
)

add_dependencies(${PROJECT_NAME}
    pddby
)

target_link_libraries(${PROJECT_NAME}
    pddby
    ${GTK2_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

if(NOT APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-export-dynamic")
endif()
