#include "string.h"

#include "report.h"

#include <CoreFoundation/CoreFoundation.h>

struct pddby_iconv
{
    pddby_t* pddby;

    CFStringEncoding src_encoding;
    CFStringEncoding dst_encoding;
};

static CFStringEncoding pddby_iconv_encoding_from_string(char const* code)
{
    if (!strcasecmp(code, "cp1251"))
    {
        return kCFStringEncodingWindowsCyrillic;
    }
    if (!strcasecmp(code, "utf-8"))
    {
        return kCFStringEncodingUTF8;
    }
    return kCFStringEncodingInvalidId;
}

pddby_iconv_t* pddby_iconv_new(pddby_t* pddby, char const* from_code, char const* to_code)
{
    assert(to_code);

    pddby_iconv_t* result = calloc(1, sizeof(pddby_iconv_t));
    if (!result)
    {
        goto error;
    }

    result->pddby = pddby;

    result->src_encoding = pddby_iconv_encoding_from_string(from_code);
    result->dst_encoding = pddby_iconv_encoding_from_string(to_code);

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

    free(conv);
}

char* pddby_string_convert(pddby_iconv_t* conv, char const* string, size_t length)
{
    assert(conv);
    assert(string);

    char* result = NULL;
    CFStringRef src = NULL;

    if (!length)
    {
        result = calloc(1, sizeof(char));
        if (!result)
        {
            goto error;
        }
        return result;
    }

    src = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8 const*)string, length, conv->src_encoding, 0,
        kCFAllocatorNull);
    if (!src)
    {
        goto error;
    }

    CFIndex result_length;
    if (!CFStringGetBytes(src, CFRangeMake(0, length), conv->dst_encoding, 0, 0, NULL, 0, &result_length) ||
        !result_length)
    {
        goto error;
    }

    result = malloc(result_length + 1);
    if (!result)
    {
        goto error;
    }

    if (!CFStringGetCString(src, result, result_length + 1, conv->dst_encoding))
    {
        goto error;
    }

    CFRelease(src);

    return result;

error:
    pddby_report(conv->pddby, pddby_message_type_error, "unable to convert string");

    if (src)
    {
        CFRelease(src);
    }
    if (result)
    {
        free(result);
    }

    return NULL;
}
