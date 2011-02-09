#ifndef PDDBY_REGEX_H
#define PDDBY_REGEX_H

#ifdef __cplusplus
extern "C"
{
#endif

#define PDDBY_REGEX_MULTILINE   (1 << 0)
#define PDDBY_REGEX_DOTALL      (1 << 1)
#define PDDBY_REGEX_NEWLINE_ANY (1 << 2)

struct pddby_regex_s;
struct pddby_regex_match_s;

typedef struct pddby_regex_s pddby_regex_t;
typedef struct pddby_regex_match_s pddby_regex_match_t;

pddby_regex_t* pddby_regex_new(char const* pattern, int options);
void pddby_regex_free(pddby_regex_t* regex);

char* pddby_regex_replace(pddby_regex_t* regex, char const* string, char const* replacement);
char* pddby_regex_replace_literal(pddby_regex_t* regex, char const* string, char const* replacement);

char** pddby_regex_split(pddby_regex_t* regex, char const* string);

int pddby_regex_match(pddby_regex_t* regex, char const* string, pddby_regex_match_t** regex_match);
void pddby_regex_match_free(pddby_regex_match_t* regex_match);

char* pddby_regex_match_fetch(pddby_regex_match_t* regex_match, int match_num);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_REGEX_H
