#ifndef PDDBY_COMMENT_H
#define PDDBY_COMMENT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"

#include <stdint.h>

struct pddby_comment_s
{
    int64_t id;
    int32_t number;
    char* text;
};

typedef struct pddby_comment_s pddby_comment_t;
typedef pddby_array_t pddby_comments_t;

pddby_comment_t* pddby_comment_new(int32_t number, char const* text);
void pddby_comment_free(pddby_comment_t* comment);

int pddby_comment_save(pddby_comment_t* comment);

pddby_comment_t* pddby_comment_find_by_id(int64_t id);
pddby_comment_t* pddby_comment_find_by_number(int32_t number);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_COMMENT_H
