#include "pddby.h"

#include "private/decode/decode.h"
#include "private/decode/decode_context.h"
#include "private/pddby.h"
#include "private/util/database.h"

#include <assert.h>
#include <stdlib.h>
#include <time.h>

pddby_t* pddby_init(char const* share_dir, char const* cache_dir, pddby_callbacks_t const* callbacks)
{
    srand(time(0));

    pddby_t* result = calloc(1, sizeof(pddby_t));
    if (!result)
    {
        return NULL;
    }

    result->callbacks = callbacks;

    pddby_db_init(result, share_dir, cache_dir);

    return result;
}

void pddby_close(pddby_t* pddby)
{
    assert(pddby);

    pddby_db_cleanup(pddby);

    free(pddby);
}

int pddby_decode(pddby_t* pddby, char const* root_path)
{
    assert(pddby);

    // TODO: check if root_path corresponds to mounted CD-ROM device

    typedef int (*pddby_decode_stage_t)(pddby_t* pddby);

    static pddby_decode_stage_t const s_decode_stages[] =
    {
        &pddby_decode_images,
        &pddby_decode_comments,
        &pddby_decode_traffregs,
        &pddby_decode_questions,
        NULL
    };

    pddby->decode_context = pddby_decode_context_new(pddby, root_path);
    if (!pddby->decode_context)
    {
        goto error;
    }

    for (pddby_decode_stage_t const* stage = s_decode_stages; *stage; stage++)
    {
        if (!(*stage)(pddby))
        {
            goto error;
        }
    }

    pddby_decode_context_free(pddby->decode_context);
    pddby->decode_context = NULL;

    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode");

    if (pddby->decode_context)
    {
        pddby_decode_context_free(pddby->decode_context);
        pddby->decode_context = NULL;
    }

    return 0;
}

int pddby_cache_exists(pddby_t* pddby)
{
    assert(pddby);

    return pddby_db_exists(pddby);
}

void pddby_use_cache(pddby_t* pddby, int value)
{
    assert(pddby);

    pddby_db_use_cache(pddby, value);
}
