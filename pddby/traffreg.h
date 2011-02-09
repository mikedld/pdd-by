#ifndef PDDBY_TRAFFREG_H
#define PDDBY_TRAFFREG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "image.h"

#include <stdint.h>

struct pddby_traffreg_s
{
    int64_t id;
    int32_t number;
    char* text;
};

typedef struct pddby_traffreg_s pddby_traffreg_t;
typedef pddby_array_t pddby_traffregs_t;

pddby_traffreg_t* pddby_traffreg_new(int32_t number, char const* text);
void pddby_traffreg_free(pddby_traffreg_t* traffreg);

int pddby_traffreg_save(pddby_traffreg_t* traffreg);

int pddby_traffreg_set_images(pddby_traffreg_t* traffreg, pddby_images_t* images);

pddby_traffreg_t* pddby_traffreg_find_by_id(int64_t id);
pddby_traffreg_t* pddby_traffreg_find_by_number(int32_t number);

pddby_traffregs_t* pddby_traffreg_find_by_question(int64_t question_id);
pddby_traffregs_t* pddby_traffreg_copy_all(pddby_traffregs_t* traffregs);
void pddby_traffreg_free_all(pddby_traffregs_t* traffregs);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_TRAFFREG_H
