#ifndef PDDBY_SECTION_H
#define PDDBY_SECTION_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "pddby.h"

#include <stdint.h>

struct pddby_section
{
    pddby_t* pddby;

    int64_t id;
    char* name;
    char* title_prefix;
    char* title;
};

typedef struct pddby_section pddby_section_t;
typedef pddby_array_t pddby_sections_t;

pddby_section_t* pddby_section_new(pddby_t* pddby, char const* name, char const* title_prefix, char const* title);
void pddby_section_free(pddby_section_t* section);

int pddby_section_save(pddby_section_t* section);

pddby_section_t* pddby_section_find_by_id(pddby_t* pddby, int64_t id);
pddby_section_t* pddby_section_find_by_name(pddby_t* pddby, char const* name);

pddby_sections_t* pddby_sections_new(pddby_t* pddby);
pddby_sections_t* pddby_sections_find_all(pddby_t* pddby);
void pddby_sections_free(pddby_sections_t* sections);

size_t pddby_section_get_question_count(pddby_section_t* section);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_SECTION_H
