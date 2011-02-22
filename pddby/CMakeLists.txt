project(pddby NONE)

add_definitions(-std=c99)

set(${PROJECT_NAME}_HEADERS
    answer.h
    array.h
    callback.h
    comment.h
    image.h
    pddby.h
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
    image.c
    pddby.c
    question.c
    section.c
    topic.c
    traffreg.c
)

set(${PROJECT_NAME}_DECODE_HEADERS
    decode/decode.h
    decode/decode_image.h
)

set(${PROJECT_NAME}_DECODE_SOURCES
    decode/decode.c
    decode/decode_image.c
)

set(${PROJECT_NAME}_UTIL_HEADERS
    util/aux.h
    util/database.h
    util/delphi.h
    util/regex.h
    util/settings.h
    util/string.h
)

set(${PROJECT_NAME}_UTIL_SOURCES
    util/aux.c
    util/database.c
    util/delphi.c
    util/regex.c
    util/settings.c
    util/string.c
)

if (APPLE)
    list(APPEND ${PROJECT_NAME}_UTIL_SOURCES
        util/string_mac.c
    )
else()
    list(APPEND ${PROJECT_NAME}_UTIL_SOURCES
        util/string_iconv.c
    )
endif()

set(${PROJECT_NAME}_SQL_FILES
    ${${PROJECT_NAME}_SOURCE_DIR}/data/10.sql
    ${${PROJECT_NAME}_SOURCE_DIR}/data/11.sql
    PARENT_SCOPE
)

include_directories(
    ${${PROJECT_NAME}_SOURCE_DIR}
    ${SQLITE3_INCLUDE_DIRS}
    ${PCRE_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${ICONV_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_DECODE_HEADERS}
    ${${PROJECT_NAME}_DECODE_SOURCES}
    ${${PROJECT_NAME}_UTIL_HEADERS}
    ${${PROJECT_NAME}_UTIL_SOURCES}
    ${${PROJECT_NAME}_SQL_FILES}
)
