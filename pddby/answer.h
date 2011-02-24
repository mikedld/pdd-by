#ifndef PDDBY_ANSWER_H
#define PDDBY_ANSWER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "pddby.h"

#include <stdint.h>

struct pddby_answer
{
    pddby_t* pddby;

    int64_t id;
    int64_t question_id;
    char* text;
    int is_correct;
};

typedef struct pddby_answer pddby_answer_t;
typedef pddby_array_t pddby_answers_t;

pddby_answer_t* pddby_answer_new(pddby_t* pddby, int64_t question_id, char const* text, int is_correct);
void pddby_answer_free(pddby_answer_t* answer);

int pddby_answer_save(pddby_answer_t* answer);

pddby_answer_t* pddby_answer_find_by_id(pddby_t* pddby, int64_t id);

pddby_answers_t* pddby_answers_new(pddby_t* pddby);
pddby_answers_t* pddby_answers_find_by_question(pddby_t* pddby, int64_t question_id);
void pddby_answers_free(pddby_answers_t* answers);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_ANSWER_H
