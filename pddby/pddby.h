#ifndef PDDBY_PDDBY_H
#define PDDBY_PDDBY_H

#ifdef __cplusplus
extern "C"
{
#endif

struct pddby_s;
typedef struct pddby_s pddby_t;

pddby_t* pddby_init(char const* share_dir, char const* cache_dir);
void pddby_close(pddby_t* pddby);

int pddby_decode(pddby_t* pddby, char const* root_path);

int pddby_cache_exists(pddby_t* pddby);
void pddby_use_cache(pddby_t* pddby, int value);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_PDDBY_H
