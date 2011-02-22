#include "string.h"

#include <CoreFoundation/CoreFoundation.h>

struct pddby_iconv_s
{
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

pddby_iconv_t* pddby_iconv_new(char const* from_code, char const* to_code)
{
    assert(to_code);

    pddby_iconv_t* result = malloc(sizeof(pddby_iconv_t));
    if (!result)
    {
        // TODO: report error
        return NULL;
    }

    result->src_encoding = pddby_iconv_encoding_from_string(from_code);
    result->dst_encoding = pddby_iconv_encoding_from_string(to_code);

    return result;
}

void pddby_iconv_free(pddby_iconv_t* conv)
{
    assert(conv);

    free(conv);
}

char* pddby_string_convert(pddby_iconv_t* conv, char const* string, size_t length)
{
    assert(conv);

    // TODO: error checking

    CFStringRef src = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8 const*)string, length,
        conv->src_encoding, 0, kCFAllocatorNull);

    CFIndex dst_length;
    CFStringGetBytes(src, CFRangeMake(0, length), conv->dst_encoding, 0, 0, NULL, 0, &dst_length);
    char* dst = malloc(dst_length + 1);
    CFStringGetCString(src, dst, dst_length + 1, conv->dst_encoding);

    CFRelease(src);

    return dst;
}
