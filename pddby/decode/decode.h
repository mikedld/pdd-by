#ifndef PDDBY_DECODE_H
#define PDDBY_DECODE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "util/string.h"

#include <stddef.h>
#include <stdint.h>

typedef char* (*pddby_decode_string_func_t)(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number);

struct pddby_decode_context_s
{
    char const* root_path;
    pddby_iconv_t* iconv;
    uint16_t data_magic;
    uint16_t image_magic;
    pddby_decode_string_func_t decode_string;
};

typedef struct pddby_decode_context_s pddby_decode_context_t;

int pddby_decode_init_magic(pddby_decode_context_t* context);
int pddby_decode_images(pddby_decode_context_t* context);
int pddby_decode_comments(pddby_decode_context_t* context);
int pddby_decode_traffregs(pddby_decode_context_t* context);
int pddby_decode_questions(pddby_decode_context_t* context);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_DECODE_H
