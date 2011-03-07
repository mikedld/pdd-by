#ifndef PDDBY_QUESTION_H
#define PDDBY_QUESTION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "pddby.h"
#include "section.h"
#include "traffreg.h"

#include <stdint.h>

struct pddby_question
{
    pddby_t* pddby;

    int64_t id;
    int64_t topic_id;
    char* text;
    int64_t image_id;
    char* advice;
    int64_t comment_id;
};

typedef struct pddby_question pddby_question_t;
typedef pddby_array_t pddby_questions_t;

pddby_question_t* pddby_question_new(pddby_t* pddby, int64_t topic_id, char const* text, int64_t image_id, char const* advice,
    int64_t comment_id);
void pddby_question_free(pddby_question_t* question);

int pddby_question_save(pddby_question_t* question);

int pddby_question_set_sections(pddby_question_t* question, pddby_sections_t* sections);
int pddby_question_set_traffregs(pddby_question_t* question, pddby_traffregs_t* traffregs);

pddby_question_t* pddby_question_find_by_id(pddby_t* pddby, int64_t id);

pddby_questions_t* pddby_questions_new(pddby_t* pddby);
pddby_questions_t* pddby_questions_find_by_section(pddby_t* pddby, int64_t section_id);
pddby_questions_t* pddby_questions_find_by_topic(pddby_t* pddby, int64_t topic_id, int ticket_number);
pddby_questions_t* pddby_questions_find_by_ticket(pddby_t* pddby, int ticket_number);
pddby_questions_t* pddby_questions_find_random(pddby_t* pddby);
void pddby_questions_free(pddby_questions_t* questions);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_QUESTION_H
