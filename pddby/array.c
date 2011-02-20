#include "array.h"

#include <assert.h>
#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_array_s
{
    size_t used_size;
    size_t reserved_size;
    void** data;
    pddby_array_free_func_t free_func;
};

static int pddby_array_realloc(pddby_array_t* arr, size_t new_size)
{
    assert(arr);

    arr->reserved_size = new_size;
    arr->data = realloc(arr->data, new_size * sizeof(void*));
    if (!arr->data)
    {
        // TODO: report error
    }
    return arr->data != 0;
}

pddby_array_t* pddby_array_new(pddby_array_free_func_t free_func)
{
    pddby_array_t* result = malloc(sizeof(pddby_array_t));
    if (!result)
    {
        // TODO: report error
        return 0;
    }

    result->data = 0;
    if (!pddby_array_realloc(result, 16))
    {
        // TODO: report error
        free(result);
        return 0;
    }

    result->used_size = 0;
    result->free_func = free_func;

    return result;
}

void pddby_array_free(pddby_array_t* arr, int free_objects)
{
    assert(arr);

    if (free_objects && arr->free_func)
    {
        for (size_t i = 0; i < arr->used_size; i++)
        {
            arr->free_func(arr->data[i]);
        }
    }
    free(arr->data);
    free(arr);
}

int pddby_array_add(pddby_array_t* arr, void* object)
{
    assert(arr);

    if (!object)
    {
        // TODO: report error
        return 0;
    }

    if (arr->used_size == arr->reserved_size)
    {
        if (!pddby_array_realloc(arr, arr->reserved_size + 16))
        {
            // TODO: report error
            return 0;
        }
    }

    arr->data[arr->used_size++] = object;
    return 1;
}

void* pddby_array_index(pddby_array_t const* arr, size_t index)
{
    assert(arr);

    if (arr->used_size <= index)
    {
        // TODO: report warning
        return 0;
    }

    return arr->data[index];
}

void pddby_array_foreach(pddby_array_t* arr, pddby_array_foreach_func_t foreach_func, void* user_data)
{
    assert(arr);

    for (size_t i = 0; i < arr->used_size; i++)
    {
        foreach_func(arr->data[i], user_data);
    }
}

size_t pddby_array_size(pddby_array_t const* arr)
{
    assert(arr);

    return arr->used_size;
}
