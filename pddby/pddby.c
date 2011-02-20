#include "pddby.h"

#include "util/database.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

struct pddby_s
{
    //
};

pddby_t* pddby_init(char const* share_dir, char const* cache_dir)
{
    srand(time(0));

    pddby_t* result = calloc(1, sizeof(pddby_t));

    pddby_db_init(share_dir, cache_dir);

    return result;
}

void pddby_close(pddby_t* pddby)
{
    assert(pddby);

    pddby_db_cleanup();

    free(pddby);
}

int pddby_cache_exists(pddby_t* pddby)
{
    return pddby_db_exists();
}

void pddby_use_cache(pddby_t* pddby, int value)
{
    pddby_db_use_cache(value);
}
