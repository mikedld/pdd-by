project(pddby NONE)

add_definitions(-std=c99)

set(${PROJECT_NAME}_HEADERS
    answer.h
    array.h
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
    comment.c
    image.c
    pddby.c
    question.c
    section.c
    topic.c
    traffreg.c
)

set(${PROJECT_NAME}_PRIVATE_HEADERS
    private/decode/decode.h
    private/decode/decode_context.h
    private/decode/decode_image.h
    private/decode/decode_questions.h
    private/pddby.h
    private/platform.h
    private/util/aux.h
    private/util/database.h
    private/util/delphi.h
    private/util/regex.h
    private/util/report.h
    private/util/settings.h
    private/util/string.h
)

set(${PROJECT_NAME}_PRIVATE_SOURCES
    private/decode/decode.c
    private/decode/decode_context.c
    private/decode/decode_image.c
    private/decode/decode_questions.c
    private/util/aux.c
    private/util/database.c
    private/util/delphi.c
    private/util/regex.c
    private/util/regex_${PDDBY_BACKEND_REGEX}.c
    private/util/report.c
    private/util/settings.c
    private/util/string.c
    private/util/string_${PDDBY_BACKEND_CONV}.c
)

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
    ${${PROJECT_NAME}_PRIVATE_HEADERS}
    ${${PROJECT_NAME}_PRIVATE_SOURCES}
    ${${PROJECT_NAME}_SQL_FILES}
)

if(PDDBY_STATIC_LIBS)
    install(TARGETS ${PROJECT_NAME} DESTINATION lib)
endif()
