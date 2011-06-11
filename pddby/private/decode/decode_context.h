#ifndef PDDBY_PRIVATE_DECODE_CONTEXT_H
#define PDDBY_PRIVATE_DECODE_CONTEXT_H

#include "pddby.h"
#include "private/util/string.h"

#include <stdint.h>
#include <unistd.h>

struct pddby_topic_question;

typedef struct pddby_decode_context pddby_decode_context_t;

typedef char* (*pddby_decode_string_func_t)(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size,
    int8_t topic_number, off_t pos_in_file);
typedef struct pddby_topic_question* (*pddby_decode_topic_questions_table_func_t)(pddby_decode_context_t* context,
    char const* path, size_t* table_size);

struct pddby_decode_context
{
    pddby_t* pddby;

    char const* root_path;
    pddby_iconv_t* iconv;
    uint16_t magic;
    uint16_t simple_table_magic;
    uint16_t image_magic;
    uint32_t part_magic;
    uint32_t table_magic;
    pddby_decode_string_func_t decode_string;
    pddby_decode_string_func_t decode_question_string;
    pddby_decode_topic_questions_table_func_t decode_topic_questions_table;
};

pddby_decode_context_t* pddby_decode_context_new(pddby_t* pddby, char const* root_path);
void pddby_decode_context_free(pddby_decode_context_t* context);

#endif // PDDBY_PRIVATE_DECODE_CONTEXT_H
