project(pddby-qt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

find_package(Qt4 4.8 REQUIRED QtCore QtGui)
include(${QT_USE_FILE})

set(${PROJECT_NAME}_SOURCES
    main.cpp
    MainWindow.cpp
    SectionDialog.cpp
    TicketDialog.cpp
    TicketNumberDialog.cpp
    TopicDialog.cpp
)

set(${PROJECT_NAME}_HEADERS
    MainWindow.h
    SectionDialog.h
    TicketDialog.h
    TicketNumberDialog.h
    TopicDialog.h
)

qt4_wrap_ui(${PROJECT_NAME}_UI_SOURCES
    MainWindow.ui
    SectionDialog.ui
    TicketDialog.ui
    TicketNumberDialog.ui
    TopicDialog.ui
)

qt4_add_resources(${PROJECT_NAME}_QRC_SOURCES
    PddBy.qrc
)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_UI_SOURCES}
    ${${PROJECT_NAME}_QRC_SOURCES}
    PddBy.icns
    PddBy.ico)

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    MACOSX_BUNDLE_INFO_STRING "Belarusian traffic regulations"
    MACOSX_BUNDLE_ICON_FILE "PddBy.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.mikedld.pdd-by"
    MACOSX_BUNDLE_LONG_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_NAME "PddBy"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_COPYRIGHT "(c) 2013 mike.dld"
    OUTPUT_NAME "PddBy"
    VERSION "1.0")

set_source_files_properties(PddBy.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

target_link_libraries(${PROJECT_NAME}
    pddby
    ${QT_LIBRARIES})
