#ifndef PDDBY_PRIVATE_STRING_H
#define PDDBY_PRIVATE_STRING_H

#include "pddby.h"

#include <stddef.h>

struct pddby_iconv;
typedef struct pddby_iconv pddby_iconv_t;

pddby_iconv_t* pddby_iconv_new(pddby_t* pddby, char const* from_code, char const* to_code);
void pddby_iconv_free(pddby_iconv_t* conv);
char* pddby_string_convert(pddby_iconv_t* conv, char const* string, size_t length);

char* pddby_string_upcase(pddby_t* pddby, char const* string);
char* pddby_string_downcase(pddby_t* pddby, char const* string);
char* pddby_string_delimit(char* string, char const* delimiters, char new_delimiter);
char* pddby_string_chomp(char* string);
char* pddby_string_ndup(pddby_t* pddby, char const* string, size_t length);
char* pddby_string_replace(pddby_t* pddby, char const* string, size_t start, size_t end, char const* replacement,
    size_t replacement_length);

char** pddby_string_split(pddby_t* pddby, char const* string, char const* delimiter);
size_t pddby_stringv_length(char* const* str_array);
void pddby_stringv_free(char** str_array);

#endif // PDDBY_PRIVATE_STRING_H
