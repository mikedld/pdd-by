#ifndef PDDBY_STRING_H
#define PDDBY_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

char* pddby_string_upcase(char const* s, size_t length);
char* pddby_string_downcase(char const* s, size_t length);
char* pddby_string_delimit(char* s, char const* delim, char new_char);
char* pddby_string_chomp(char const* s);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_STRING_H
