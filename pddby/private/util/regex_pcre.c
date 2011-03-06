#include "regex.h"

#include "report.h"

#include <assert.h>
#include <pcre.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_pcre_regex
{
    pddby_t* pddby;

    pcre* regex;
    pcre_extra* regex_extra;
    int cap_count;
};

#define D(x) ((struct pddby_pcre_regex*)(x))

pddby_regex_t* pddby_regex_new(pddby_t* pddby, char const* pattern, int options)
{
    pddby_regex_t* result = calloc(1, sizeof(struct pddby_pcre_regex));
    if (!result)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to create regex");
        return 0;
    }

    char const* error_text;
    int error_offset;

    int pcre_options = 0;
    pcre_options |= (options & PDDBY_REGEX_MULTILINE) ? PCRE_MULTILINE : 0;
    pcre_options |= (options & PDDBY_REGEX_DOTALL) ? PCRE_DOTALL : 0;
    pcre_options |= (options & PDDBY_REGEX_NEWLINE_ANY) ? PCRE_NEWLINE_ANY : 0;

    D(result)->regex = pcre_compile(pattern, pcre_options, &error_text, &error_offset, 0);
    if (!D(result)->regex)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to compile regex: %s (%d)", error_text, error_offset);
        goto error;
    }

    D(result)->regex_extra = pcre_study(D(result)->regex, 0, &error_text);
    if (!D(result)->regex_extra && error_offset)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to study regex: %s", error_text);
        goto error;
    }

    if (pcre_fullinfo(D(result)->regex, D(result)->regex_extra, PCRE_INFO_CAPTURECOUNT, &D(result)->cap_count))
    {
        pddby_report(pddby, pddby_message_type_error, "unable to get regex capturing subpatterns count");
        goto error;
    }

    D(result)->pddby = pddby;

    return result;

error:
    if (result)
    {
        pddby_regex_free(result);
    }

    return NULL;
}

void pddby_regex_free(pddby_regex_t* regex)
{
    assert(regex);

    if (D(regex)->regex_extra)
    {
        pcre_free(D(regex)->regex_extra);
    }
    if (D(regex)->regex)
    {
        pcre_free(D(regex)->regex);
    }
    free(regex);
}

int pddby_regex_match(pddby_regex_t* regex, char const* string, pddby_regex_match_t** regex_match)
{
    assert(regex);
    assert(string);
    assert(regex_match);

    *regex_match = calloc(1, sizeof(pddby_regex_match_t));
    if (!*regex_match)
    {
        return 0;
    }

    (*regex_match)->caps = malloc((D(regex)->cap_count + 1) * 3 * sizeof(int));
    if (!(*regex_match)->caps)
    {
        pddby_regex_match_free(*regex_match);
        return 0;
    }

    (*regex_match)->count = D(regex)->cap_count + 1;
    (*regex_match)->string = string;
    (*regex_match)->pddby = D(regex)->pddby;
    int result = pcre_exec(D(regex)->regex, D(regex)->regex_extra, string, strlen(string), 0, 0, (*regex_match)->caps,
        (D(regex)->cap_count + 1) * 3);
    if (result >= 0)
    {
        return 1;
    }

    if (result != PCRE_ERROR_NOMATCH)
    {
        pddby_report(D(regex)->pddby, pddby_message_type_error, "unable to match string using regex");
    }
    pddby_regex_match_free(*regex_match);
    return 0;
}
