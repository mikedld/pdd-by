#include "string.h"

// temporarily, for darwin w/macports
#define LIBICONV_PLUG

#include <ctype.h>
#include <iconv.h>
#include <stdlib.h>
#include <string.h>

struct pddby_iconv_s
{
    iconv_t conv;
};

pddby_iconv_t* pddby_iconv_new(char const* from_code, char const* to_code)
{
    pddby_iconv_t* result = malloc(sizeof(pddby_iconv_t));
    result->conv = iconv_open(to_code, from_code);
    return result;
}

void pddby_iconv_free(pddby_iconv_t* conv)
{
    iconv_close(conv->conv);
    free(conv);
}

char* pddby_string_convert(pddby_iconv_t* conv, char const* string, size_t length)
{
    char* result = malloc(length);
    size_t result_len = length;

    char* src = string;
    size_t src_len = length;
    char* dst = result;
    size_t dst_len = length;

    for (;;)
    {
        size_t ret = iconv(conv->conv, &src, &src_len, &dst, &dst_len);
        if (ret != (size_t)-1)
        {
            break;
        }
        if (errno != E2BIG)
        {
            free(result);
            return 0;
        }
        ptrdiff_t offset = dst - result;
        result_len += length;
        result = realloc(result, result_len);
        dst = result + offset;
        dst_len = length;
    }

    result_len -= dst_len - 1;
    result = realloc(result, result_len);
    result[result_len] = '\0';
    return result;
}

char* pddby_string_upcase(char const* string)
{
    char* result = strdup(string);
    for (size_t i = 0, len = strlen(result); i < len; i++)
    {
        result[i] = toupper(result[i]);
    }
    return result;
}

char* pddby_string_downcase(char const* string)
{
    char* result = strdup(string);
    for (size_t i = 0, len = strlen(result); i < len; i++)
    {
        result[i] = tolower(result[i]);
    }
    return result;
}

char* pddby_string_delimit(char* string, char const* delimiters, char new_delmiter)
{
    char* p = string;
    while (*p)
    {
        if (strchr(delimiters, *p))
        {
            *p = new_delmiter;
        }
        p++;
    }
    return string;
}

char* pddby_string_chomp(char* string)
{
    char* p = string + strlen(string) - 1;
    while (p >= string && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'))
    {
        *p = '\0';
        p--;
    }
    return string;
}

char* pddby_string_ndup(char const* string, size_t length)
{
    if (length == (size_t)-1)
    {
        length = strlen(string);
    }
    char* result = malloc(length + 1);
    memcpy(result, string, length);
    result[length] = '\0';
    return result;
}

char* pddby_string_replace(char const* string, size_t start, size_t end, char const* replacement,
    size_t replacement_length)
{
    if (replacement_length == (size_t)-1)
    {
        replacement_length = strlen(replacement);
    }
    size_t const match_length = end - start;
    size_t const old_length = strlen(string);
    char* result = malloc(old_length - match_length + replacement_length + 1);
    memcpy(result, string, start);
    memcpy(result + start, replacement, replacement_length);
    memcpy(result + start + replacement_length, string + end, old_length - end);
    result[old_length - match_length + replacement_length] = '\0';
    return result;
}

char** pddby_string_split(char const* string, char const* delimiter)
{
    size_t const delimiter_length = strlen(delimiter);
    size_t size = 1;
    char** result = malloc((size + 1) * sizeof(char*));
    char const* p1 = string;
    char const* p2;
    while ((p2 = strstr(p1, delimiter)))
    {
        size_t const part_length = p2 - p1;
        result[size - 1] = pddby_string_ndup(p1, part_length);
        size++;
        result = realloc(result, (size + 1) * sizeof(char*));
        p1 = p2 + delimiter_length;
    }
    result[size - 1] = pddby_string_ndup(p1, -1);
    result[size] = 0;
    return result;
}

size_t pddby_stringv_length(char* const* str_array)
{
    size_t result = 0;
    while (*str_array)
    {
        result++;
        str_array++;
    }
    return result;
}

void pddby_stringv_free(char** str_array)
{
    char** p = str_array;
    while (*p)
    {
        free(*p);
        p++;
    }
    free(str_array);
}
