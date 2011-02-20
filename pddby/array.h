#ifndef PDDBY_ARRAY_H
#define PDDBY_ARRAY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

struct pddby_array_s;
typedef struct pddby_array_s pddby_array_t;

typedef void (*pddby_array_free_func_t)(void* object);
typedef void (*pddby_array_foreach_func_t)(void* object, void* user_data);

pddby_array_t* pddby_array_new(pddby_array_free_func_t free_func);
void pddby_array_free(pddby_array_t* arr, int free_objects);
int pddby_array_add(pddby_array_t* arr, void* object);
void* pddby_array_index(pddby_array_t const* arr, size_t index);
void pddby_array_foreach(pddby_array_t* arr, pddby_array_foreach_func_t foreach_func, void* user_data);
size_t pddby_array_size(pddby_array_t const* arr);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_ARRAY_H
