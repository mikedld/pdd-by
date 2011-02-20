#include "settings.h"

#include "config.h"
#include "database.h"

#include <assert.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

char* pddby_settings_get(char const* key)
{
    assert(key);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare("SELECT `value` FROM `settings` WHERE `key`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_text(db_stmt, 1, key))
    {
        goto error;
    }

    switch (pddby_db_step(db_stmt))
    {
    case -1:
        goto error;
    case 0:
        return NULL;
    }

    char const* value = pddby_db_column_text(db_stmt, 0);

    return strdup(value);

error:
    // TODO: report error
    return NULL;
}
