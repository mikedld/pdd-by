project(pddby C)

set(${PROJECT_NAME}_HEADERS
    answer.h
    comment.h
    database.h
    decode.h
    delphi_helper.h
    image.h
    question.h
    section.h
    settings.h
    topic.h
    traffreg.h
)

set(${PROJECT_NAME}_SOURCES
    answer.c
    comment.c
    database.c
    decode.c
    delphi_helper.c
    image.c
    question.c
    section.c
    settings.c
    topic.c
    traffreg.c
)

include_directories(
    ${GLIB2_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
)
