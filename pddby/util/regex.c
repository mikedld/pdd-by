#include "regex.h"
#include "../callback.h"
#include "string.h"

#include <pcre.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_regex_s
{
    pcre* regex;
    pcre_extra* regex_extra;
    int cap_count;
};

struct pddby_regex_match_s
{
    int* caps;
    int count;
    char const* string;
};

pddby_regex_t* pddby_regex_new(char const* pattern, int options)
{
    pddby_regex_t* result = malloc(sizeof(pddby_regex_t));

    char const* error_text;
    int error_offset;

    int pcre_options = 0;
    pcre_options |= (options & PDDBY_REGEX_MULTILINE) ? PCRE_MULTILINE : 0;
    pcre_options |= (options & PDDBY_REGEX_DOTALL) ? PCRE_DOTALL : 0;
    pcre_options |= (options & PDDBY_REGEX_NEWLINE_ANY) ? PCRE_NEWLINE_ANY : 0;

    result->regex = pcre_compile(pattern, pcre_options, &error_text, &error_offset, 0);
    if (!result->regex)
    {
        pddby_report_error("unable to compile regular expression: %s (%d)", error_text, error_offset);
    }

    result->regex_extra = pcre_study(result->regex, 0, &error_text);
    if (!result->regex_extra)
    {
        //pddby_report_error("unable to study regular expression: %s", error_text);
    }

    if (pcre_fullinfo(result->regex, result->regex_extra, PCRE_INFO_CAPTURECOUNT, &result->cap_count))
    {
        pddby_report_error("unable to get regular expression capturing subpatterns count");
    }

    return result;
}

void pddby_regex_free(pddby_regex_t* regex)
{
    pcre_free(regex->regex_extra);
    pcre_free(regex->regex);
    free(regex);
}

char* pddby_regex_replace(pddby_regex_t* regex, char const* string, char const* replacement)
{
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
    pddby_regex_match_t* match;
    size_t start = 0;
    while (pddby_regex_match(regex, result + start, &match))
    {
        char* r = strdup(replacement);
        for (size_t i = 0; i < refs_count; i++)
        {
            char* new_r = pddby_string_replace(r, refs[i].pos, refs[i].pos + 2, match->string +
                match->caps[refs[i].num * 2], match->caps[refs[i].num * 2 + 1] - match->caps[refs[i].num * 2]);
            free(r);
            r = new_r;
        }
        size_t const r_length = strlen(r);
        char* new_result = pddby_string_replace(result, start + match->caps[0], start + match->caps[1], r, r_length);
        free(r);
        free(result);
        result = new_result;
        start += match->caps[0] + r_length;
        pddby_regex_match_free(match);
    }
    return result;
}

char* pddby_regex_replace_literal(pddby_regex_t* regex, char const* string, char const* replacement)
{
    size_t const replacement_length = strlen(replacement);
    char* result = strdup(string);
    pddby_regex_match_t* match;
    size_t start = 0;
    while (pddby_regex_match(regex, result + start, &match))
    {
        char* new_result = pddby_string_replace(result, start + match->caps[0], start + match->caps[1], replacement,
            replacement_length);
        free(result);
        result = new_result;
        start += match->caps[0] + replacement_length;
        pddby_regex_match_free(match);
    }
    return result;
}

char** pddby_regex_split(pddby_regex_t* regex, char const* string)
{
    size_t size = 1;
    char** result = malloc((size + 1) * sizeof(char*));
    pddby_regex_match_t* match;
    char const* p = string;
    while (pddby_regex_match(regex, p, &match))
    {
        result[size - 1] = pddby_string_ndup(p, match->caps[0]);
        size++;
        result = realloc(result, (size + 1) * sizeof(char*));
        p += match->caps[1];
        pddby_regex_match_free(match);
    }
    result[size - 1] = pddby_string_ndup(p, -1);
    result[size] = 0;
    return result;
}

int pddby_regex_match(pddby_regex_t* regex, char const* string, pddby_regex_match_t** regex_match)
{
    *regex_match = malloc(sizeof(pddby_regex_match_t));
    (*regex_match)->caps = malloc((regex->cap_count + 1) * 3 * sizeof(int));
    (*regex_match)->count = regex->cap_count + 1;
    (*regex_match)->string = string;
    int result = pcre_exec(regex->regex, regex->regex_extra, string, strlen(string), 0, 0, (*regex_match)->caps,
        (regex->cap_count + 1) * 3);
    if (result < 0)
    {
        pddby_regex_match_free(*regex_match);
        *regex_match = 0;
    }
    return result >= 0;
}

void pddby_regex_match_free(pddby_regex_match_t* regex_match)
{
    free(regex_match->caps);
    free(regex_match);
}

char* pddby_regex_match_fetch(pddby_regex_match_t* regex_match, int match_num)
{
    if (match_num >= regex_match->count)
    {
        return 0;
    }

    return pddby_string_ndup(regex_match->string + regex_match->caps[match_num * 2],
        regex_match->caps[match_num * 2 + 1] - regex_match->caps[match_num * 2]);
}
