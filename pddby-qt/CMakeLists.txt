project(pddby-qt NONE)

find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
include(${QT_USE_FILE})

set(MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}")

add_definitions(-std=c++98)

set(${PROJECT_NAME}_SOURCES
    main.cpp
)

include_directories(
    ${SQLITE3_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_UI_FILES}
    ${pddby_SQL_FILES}
)

target_link_libraries(${PROJECT_NAME}
    pddby
    ${QT_LIBRARIES}
)
