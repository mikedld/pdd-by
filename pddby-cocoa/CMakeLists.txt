project(pddby-cocoa NONE)

set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")

add_definitions(-std=c99)

set(${PROJECT_NAME}_SOURCES
    main.m
)

include_directories(
    ${SQLITE3_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_XIB_FILES}
    ${pddby_SQL_FILES}
)

target_link_libraries(${PROJECT_NAME}
    pddby
    "-framework Cocoa"
)
