#include "array.h"

#include <stdlib.h>

struct pddby_array_s
{
    size_t used_size;
    size_t reserved_size;
    void** data;
    pddby_array_free_func_t free_func;
};

static int pddby_array_realloc(pddby_array_t* arr, size_t new_size)
{
    arr->reserved_size = new_size;
    arr->data = realloc(arr->data, new_size);
    return arr->data != 0;
}

pddby_array_t* pddby_array_new(pddby_array_free_func_t free_func)
{
    pddby_array_t* result = malloc(sizeof(pddby_array_t));
    if (!result)
    {
        return NULL;
    }

    result->data = NULL;
    if (!pddby_array_realloc(result, 16))
    {
        free(result);
        return NULL;
    }

    result->used_size = 0;
    result->free_func = free_func;

    return result;
}

void pddby_array_free(pddby_array_t* arr)
{
    if (arr->free_func)
    {
        for (size_t i = 0; i < arr->used_size; i++)
        {
            arr->free_func(arr->data[i]);
        }
    }
    free(arr);
}

void pddby_array_add(pddby_array_t* arr, void* object)
{
    if (arr->used_size == arr->reserved_size)
    {
        pddby_array_realloc(arr, arr->reserved_size + 16);
    }
    arr->data[arr->used_size++] = object;
}

void* pddby_array_index(pddby_array_t const* arr, size_t index)
{
    if (arr->used_size <= index)
    {
        return NULL;
    }
    return arr->data[index];
}

void pddby_array_foreach(pddby_array_t* arr, pddby_array_foreach_func_t foreach_func, void* user_data)
{
    for (size_t i = 0; i < arr->used_size; i++)
    {
        foreach_func(arr->data[i], user_data);
    }
}

size_t pddby_array_size(pddby_array_t const* arr)
{
    return arr->used_size;
}
