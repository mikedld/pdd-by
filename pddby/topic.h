#ifndef PDDBY_TOPIC_H
#define PDDBY_TOPIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "pddby.h"

#include <stdint.h>

struct pddby_topic
{
    pddby_t* pddby;

    int64_t id;
    int number;
    char* title;
};

typedef struct pddby_topic pddby_topic_t;
typedef pddby_array_t pddby_topics_t;

pddby_topic_t* pddby_topic_new(pddby_t* pddby, int number, char const* title);
void pddby_topic_free(pddby_topic_t* topic);

int pddby_topic_save(pddby_topic_t* topic);

pddby_topic_t* pddby_topic_find_by_id(pddby_t* pddby, int64_t id);
pddby_topic_t* pddby_topic_find_by_number(pddby_t* pddby, int number);

pddby_topics_t* pddby_topics_new(pddby_t* pddby);
pddby_topics_t* pddby_topics_find_all(pddby_t* pddby);
void pddby_topics_free(pddby_topics_t* topics);

size_t pddby_topic_get_question_count(pddby_topic_t const* topic);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_TOPIC_H
