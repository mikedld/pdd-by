project(pddby C)

set(${PROJECT_NAME}_HEADERS
    answer.h
    array.h
    callback.h
    comment.h
    database.h
    decode.h
    decode_image.h
    image.h
    question.h
    section.h
    topic.h
    traffreg.h
)

set(${PROJECT_NAME}_SOURCES
    answer.c
    array.c
    callback.c
    comment.c
    database.c
    decode.c
    decode_image.c
    image.c
    question.c
    section.c
    topic.c
    traffreg.c
)

set(${PROJECT_NAME}_UTIL_HEADERS
    util/aux.h
    util/delphi.h
    util/regex.h
    util/settings.h
    util/string.h
)

set(${PROJECT_NAME}_UTIL_SOURCES
    util/aux.c
    util/delphi.c
    util/regex.c
    util/settings.c
    util/string.c
)

set(${PROJECT_NAME}_SQL_FILES
    ${${PROJECT_NAME}_SOURCE_DIR}/data/10.sql
    ${${PROJECT_NAME}_SOURCE_DIR}/data/11.sql
    PARENT_SCOPE
)

include_directories(
    ${SQLITE3_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_UTIL_HEADERS}
    ${${PROJECT_NAME}_UTIL_SOURCES}
    ${${PROJECT_NAME}_SQL_FILES}
)
