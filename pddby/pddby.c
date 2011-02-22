#include "pddby.h"

#include "decode/decode.h"
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

int pddby_decode(pddby_t* pddby, char const* root_path)
{
    // TODO: check if root_path corresponds to mounted CD-ROM device

    typedef int (*pddby_decode_stage_t)(pddby_decode_context_t* context);

    static pddby_decode_stage_t const s_decode_stages[] =
    {
        &pddby_decode_init_magic,
        &pddby_decode_images,
        &pddby_decode_comments,
        &pddby_decode_traffregs,
        &pddby_decode_questions,
        NULL
    };

    pddby_decode_context_t context;
    context.root_path = root_path;
    context.iconv = pddby_iconv_new("cp1251", "utf-8");

    for (pddby_decode_stage_t const* stage = s_decode_stages; *stage; stage++)
    {
        if (!(*stage)(&context))
        {
            return 0;
        }
    }

    pddby_iconv_free(context.iconv);

    return 1;
}

int pddby_cache_exists(pddby_t* pddby)
{
    return pddby_db_exists();
}

void pddby_use_cache(pddby_t* pddby, int value)
{
    pddby_db_use_cache(value);
}
