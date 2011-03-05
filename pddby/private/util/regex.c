#include "regex.h"

#include "report.h"
#include "string.h"

#include <assert.h>
#include <pcre.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_regex
{
    pddby_t* pddby;

    pcre* regex;
    pcre_extra* regex_extra;
    int cap_count;
};

struct pddby_regex_match
{
    pddby_t* pddby;

    int* caps;
    int count;
    char const* string;
};

pddby_regex_t* pddby_regex_new(pddby_t* pddby, char const* pattern, int options)
{
    pddby_regex_t* result = calloc(1, sizeof(pddby_regex_t));
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

    result->regex = pcre_compile(pattern, pcre_options, &error_text, &error_offset, 0);
    if (!result->regex)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to compile regex: %s (%d)", error_text, error_offset);
        goto error;
    }

    result->regex_extra = pcre_study(result->regex, 0, &error_text);
    if (!result->regex_extra && error_offset)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to study regex: %s", error_text);
        goto error;
    }

    if (pcre_fullinfo(result->regex, result->regex_extra, PCRE_INFO_CAPTURECOUNT, &result->cap_count))
    {
        pddby_report(pddby, pddby_message_type_error, "unable to get regex capturing subpatterns count");
        goto error;
    }

    result->pddby = pddby;

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

    if (regex->regex_extra)
    {
        pcre_free(regex->regex_extra);
    }
    if (regex->regex)
    {
        pcre_free(regex->regex);
    }
    free(regex);
}

char* pddby_regex_replace(pddby_regex_t* regex, char const* string, char const* replacement)
{
    assert(regex);
    assert(string);
    assert(replacement);

    struct ref_s
    {
        ptrdiff_t pos;
        int num;
    };

    struct ref_s refs[10];
    size_t refs_count = 0;
    for (char const* p = replacement + strlen(replacement) - 2; p >= replacement; p--)
    {
        if (*p == '\\' && *(p + 1) >= '1' && *(p + 1) <= '9')
        {
            refs[refs_count].pos = p - replacement;
            refs[refs_count].num = *(p + 1) - '0';
            refs_count++;
        }
    }

    char* result = strdup(string);
    if (!result)
    {
        goto error;
    }

    pddby_regex_match_t* match;
    size_t start = 0;
    while (pddby_regex_match(regex, result + start, &match))
    {
        assert(match);
        assert(match->caps);

        char* r = strdup(replacement);
        if (!r)
        {
            goto error;
        }

        for (size_t i = 0; i < refs_count; i++)
        {
            char* new_r = pddby_string_replace(regex->pddby, r, refs[i].pos, refs[i].pos + 2, match->string +
                match->caps[refs[i].num * 2], match->caps[refs[i].num * 2 + 1] - match->caps[refs[i].num * 2]);
            free(r);
            if (!new_r)
            {
                goto error;
            }

            r = new_r;
        }

        size_t const r_length = strlen(r);
        char* new_result = pddby_string_replace(regex->pddby, result, start + match->caps[0], start + match->caps[1], r, r_length);
        free(r);
        if (!new_result)
        {
            goto error;
        }

        free(result);
        result = new_result;
        start += match->caps[0] + r_length;
        pddby_regex_match_free(match);
    }

    return result;

error:
    pddby_report(regex->pddby, pddby_message_type_error, "unable to replace substring using regex");

    if (result)
    {
        free(result);
    }

    return NULL;
}

char* pddby_regex_replace_literal(pddby_regex_t* regex, char const* string, char const* replacement)
{
    assert(regex);
    assert(string);
    assert(replacement);

    size_t const replacement_length = strlen(replacement);
    char* result = strdup(string);
    if (!result)
    {
        goto error;
    }

    pddby_regex_match_t* match;
    size_t start = 0;
    while (pddby_regex_match(regex, result + start, &match))
    {
        assert(match);
        assert(match->caps);

        char* new_result = pddby_string_replace(regex->pddby, result, start + match->caps[0], start + match->caps[1], replacement,
            replacement_length);
        free(result);
        if (!new_result)
        {
            goto error;
        }

        result = new_result;
        start += match->caps[0] + replacement_length;
        pddby_regex_match_free(match);
    }
    return result;

error:
    pddby_report(regex->pddby, pddby_message_type_error, "unable to replace substring using regex");

    return NULL;
}

char** pddby_regex_split(pddby_regex_t* regex, char const* string)
{
    assert(regex);
    assert(string);

    size_t size = 1;
    char** result = calloc(size + 1, sizeof(char*));
    if (!result)
    {
        goto error;
    }

    pddby_regex_match_t* match;
    char const* p = string;
    while (pddby_regex_match(regex, p, &match))
    {
        assert(match);
        assert(match->caps);

        result[size - 1] = pddby_string_ndup(regex->pddby, p, match->caps[0]);
        if (!result[size - 1])
        {
            goto error;
        }

        size++;
        result = realloc(result, (size + 1) * sizeof(char*));
        if (!result)
        {
            goto error;
        }

        p += match->caps[1];
        pddby_regex_match_free(match);
    }

    result[size - 1] = pddby_string_ndup(regex->pddby, p, -1);
    result[size] = 0;
    return result;

error:
    pddby_report(regex->pddby, pddby_message_type_error, "unable to split string using regex");

    if (result)
    {
        for (size_t i = 0; i < size; i++)
        {
            if (result[i])
            {
                free(result[i]);
            }
        }
        free(result);
    }

    return NULL;
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

    (*regex_match)->caps = malloc((regex->cap_count + 1) * 3 * sizeof(int));
    if (!(*regex_match)->caps)
    {
        pddby_regex_match_free(*regex_match);
        return 0;
    }

    (*regex_match)->count = regex->cap_count + 1;
    (*regex_match)->string = string;
    (*regex_match)->pddby = regex->pddby;
    int result = pcre_exec(regex->regex, regex->regex_extra, string, strlen(string), 0, 0, (*regex_match)->caps,
        (regex->cap_count + 1) * 3);
    if (result >= 0)
    {
        return 1;
    }

    if (result != PCRE_ERROR_NOMATCH)
    {
        pddby_report(regex->pddby, pddby_message_type_error, "unable to match string using regex");
    }
    pddby_regex_match_free(*regex_match);
    return 0;
}

void pddby_regex_match_free(pddby_regex_match_t* regex_match)
{
    assert(regex_match);

    if (regex_match->caps)
    {
        free(regex_match->caps);
    }
    free(regex_match);
}

char* pddby_regex_match_fetch(pddby_regex_match_t* regex_match, int match_num)
{
    assert(regex_match);

    if (match_num >= regex_match->count)
    {
        pddby_report(regex_match->pddby, pddby_message_type_warning, "unable to fetch regex match (%d >= %d)",
            match_num, regex_match->count);
        return 0;
    }

    return pddby_string_ndup(regex_match->pddby, regex_match->string + regex_match->caps[match_num * 2],
        regex_match->caps[match_num * 2 + 1] - regex_match->caps[match_num * 2]);
}
