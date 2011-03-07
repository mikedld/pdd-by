project(pddby-cocoa NONE)

set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")

add_definitions(-std=c99)

set(${PROJECT_NAME}_SOURCES
    main.m
)

# CMake doesn't know how to handle .m files
set_source_files_properties(${${PROJECT_NAME}_SOURCES} PROPERTIES
    LANGUAGE C
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
