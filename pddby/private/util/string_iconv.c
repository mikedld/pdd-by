#include "string.h"

#include <assert.h>
#include <errno.h>
#include <iconv.h>
#include <stdlib.h>

struct pddby_iconv
{
    iconv_t conv;
};

pddby_iconv_t* pddby_iconv_new(char const* from_code, char const* to_code)
{
    assert(from_code);
    assert(to_code);

    pddby_iconv_t* result = malloc(sizeof(pddby_iconv_t));
    if (!result)
    {
        // TODO: report error
        return 0;
    }

    result->conv = iconv_open(to_code, from_code);
    if (result->conv == (iconv_t)-1)
    {
        // TODO: report error
        free(result);
        return 0;
    }

    return result;
}

void pddby_iconv_free(pddby_iconv_t* conv)
{
    assert(conv);

    if (iconv_close(conv->conv) == -1)
    {
        // TODO: report warning
    }

    free(conv);
}

char* pddby_string_convert(pddby_iconv_t* conv, char const* string, size_t length)
{
    assert(conv);
    assert(string);
    assert(length);

    char* result = malloc(length);
    if (!result)
    {
        // TODO: report error
        return 0;
    }

    size_t result_len = length;

    char* src = (char*)string;
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
            // TODO: report error
            free(result);
            return 0;
        }

        ptrdiff_t offset = dst - result;
        result_len += length;
        result = realloc(result, result_len);
        if (!result)
        {
            // TODO: report error
            return 0;
        }

        dst = result + offset;
        dst_len = length;
    }

    result_len -= dst_len;
    result = realloc(result, result_len);
    if (!result)
    {
        // TODO: report error
        return 0;
    }

    result[result_len - 1] = '\0';
    return result;
}
