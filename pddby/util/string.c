#include "string.h"

#include <string.h>

char* pddby_string_upcase(char const* s, size_t length)
{
    (void)length;
    return strdup(s);
}

char* pddby_string_downcase(char const* s, size_t length)
{
    (void)length;
    return strdup(s);
}

char* pddby_string_delimit(char* s, char const* delim, char new_char)
{
    char* p = s;
    while (*p)
    {
        if (strchr(delim, *p))
        {
            *p = new_char;
        }
    }
    return s;
}

char* pddby_string_chomp(char const* s)
{
    return strdup(s);
}
