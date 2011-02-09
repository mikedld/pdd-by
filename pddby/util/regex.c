#include "regex.h"
#include "../callback.h"

#include <pcre.h>
#include <string.h>

struct pddby_regex_s
{
    pcre* regex;
    pcre_extra* regex_extra;
};

struct pddby_regex_match_s
{
    int* matches;
    
};

pddby_regex_t* pddby_regex_new(char const* pattern, int options)
{
    pddby_regex_t* result = malloc(sizeof(pddby_regex_t));

    char const* error_text;
    int error_offset;
    result->regex = pcre_compile(pattern, options, &error_text, &error_offset, 0);
    if (!result->regex)
    {
        pddby_report_error("unable to compile regular expression: %s (%d)", error_text, error_offset);
    }

    result->regex_extra = pcre_study(result->regex, 0, &error_text);
    if (!result->regex_extra)
    {
        pddby_report_error("unable to study regular expression: %s", error_text);
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
    (void)regex;
    (void)string;
    (void)replacement;
    return 0;
}

char* pddby_regex_replace_literal(pddby_regex_t* regex, char const* string, char const* replacement)
{
    (void)regex;
    (void)string;
    (void)replacement;
    return 0;
}

char** pddby_regex_split(pddby_regex_t* regex, char const* string)
{
    (void)regex;
    (void)string;
    return 0;
}

int pddby_regex_match(pddby_regex_t* regex, char const* string, pddby_regex_match_t** regex_match)
{
    *regex_match = malloc(sizeof(pddby_regex_match_t));
    (*regex_match)->matches = malloc(10 * sizeof(int));
    return pcre_exec(regex->regex, regex->regex_extra, string, strlen(string), 0, 0, (*regex_match)->matches, 10);
}

void pddby_regex_match_free(pddby_regex_match_t* regex_match)
{
    free(regex_match->matches);
    free(regex_match);
}

char* pddby_regex_match_fetch(pddby_regex_match_t* regex_match, int match_num)
{
    (void)regex_match;
    (void)match_num;
    return 0;
}
