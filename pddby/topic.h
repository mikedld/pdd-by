#ifndef PDDBY_TOPIC_H
#define PDDBY_TOPIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"

#include <stdint.h>

struct pddby_topic_s
{
    int64_t id;
    int number;
    char* title;
};

typedef struct pddby_topic_s pddby_topic_t;
typedef pddby_array_t pddby_topics_t;

pddby_topics_t* pddby_get_topics();

pddby_topic_t* pddby_topic_new(int number, char const* title);
void pddby_topic_free(pddby_topic_t* topic);

int pddby_topic_save(pddby_topic_t* topic);

pddby_topic_t* pddby_topic_find_by_id(int64_t id);
pddby_topic_t* pddby_topic_find_by_number(int number);

pddby_topics_t* pddby_topic_find_all();
void pddby_topic_free_all(pddby_topics_t* topics);

size_t pddby_topic_get_question_count(pddby_topic_t const* topic);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_TOPIC_H
