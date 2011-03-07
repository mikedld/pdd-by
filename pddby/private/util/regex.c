#include "regex.h"

#include "report.h"
#include "string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_regex
{
    pddby_t* pddby;
};

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
