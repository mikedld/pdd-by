#include "string.h"

#include "report.h"

#include <assert.h>
#include <errno.h>
#include <iconv.h>
#include <stdlib.h>

struct pddby_iconv
{
    pddby_t* pddby;

    iconv_t conv;
};

pddby_iconv_t* pddby_iconv_new(pddby_t* pddby, char const* from_code, char const* to_code)
{
    assert(from_code);
    assert(to_code);

    pddby_iconv_t* result = calloc(1, sizeof(pddby_iconv_t));
    if (!result)
    {
        goto error;
    }

    result->pddby = pddby;

    result->conv = iconv_open(to_code, from_code);
    if (result->conv == (iconv_t)-1)
    {
        result->conv = 0;
        goto error;
    }

    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create iconv context");

    if (result)
    {
        pddby_iconv_free(result);
    }

    return NULL;
}

void pddby_iconv_free(pddby_iconv_t* conv)
{
    assert(conv);

    if (conv->conv && iconv_close(conv->conv) == -1)
    {
        pddby_report(conv->pddby, pddby_message_type_warning, "unable to close iconv context");
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
        goto error;
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
            goto error;
        }

        ptrdiff_t offset = dst - result;
        result_len += length;
        result = realloc(result, result_len);
        if (!result)
        {
            goto error;
        }

        dst = result + offset;
        dst_len = length;
    }

    result_len -= dst_len;
    result = realloc(result, result_len);
    if (!result)
    {
        goto error;
    }

    result[result_len - 1] = '\0';
    return result;

error:
    pddby_report(conv->pddby, pddby_message_type_error, "unable to convert string");

    if (result)
    {
        free(result);
    }

    return NULL;
}
