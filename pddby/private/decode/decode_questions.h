#ifndef PDDBY_PRIVATE_DECODE_QUESTIONS_H
#define PDDBY_PRIVATE_DECODE_QUESTIONS_H

#include "decode_context.h"
#include "pddby.h"

#include <stdint.h>

struct pddby_topic_question
{
    int32_t topic_number;
    int32_t question_offset;
};

typedef struct pddby_topic_question pddby_topic_question_t;

pddby_topic_question_t* pddby_decode_topic_questions_table(pddby_decode_context_t* context, char const* path,
    size_t* table_size);
pddby_topic_question_t* pddby_decode_topic_questions_table_v13(pddby_decode_context_t* context, char const* path,
    size_t* table_size);

int pddby_decode_questions_data(pddby_decode_context_t* context, char const* dbt_path, int32_t topic_number,
    pddby_topic_question_t* sections_data, size_t sections_data_size);
int pddby_compare_topic_questions(void const* first, void const* second);

#endif // PDDBY_PRIVATE_DECODE_QUESTIONS_H
