#ifndef PDDBY_PRIVATE_DECODE_CONTEXT_H
#define PDDBY_PRIVATE_DECODE_CONTEXT_H

#include "pddby.h"
#include "private/util/string.h"

#include <stdint.h>

typedef struct pddby_decode_context pddby_decode_context_t;

typedef char* (*pddby_decode_string_func_t)(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number);

struct pddby_decode_context
{
    pddby_t* pddby;

    char const* root_path;
    pddby_iconv_t* iconv;
    uint16_t data_magic;
    uint16_t image_magic;
    pddby_decode_string_func_t decode_string;
};

pddby_decode_context_t* pddby_decode_context_new(pddby_t* pddby, char const* root_path);
void pddby_decode_context_free(pddby_decode_context_t* context);

#endif // PDDBY_PRIVATE_DECODE_CONTEXT_H
