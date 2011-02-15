#include "settings.h"
#include "config.h"
#include "../database.h"

#include <assert.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

char* pddby_settings_get(char const* key)
{
    assert(key);

    static sqlite3_stmt* stmt = 0;

    sqlite3* db = pddby_database_get();
    if (!db)
    {
        // TODO: report error
        return 0;
    }

    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `value` FROM `settings` WHERE `key`=? LIMIT 1", -1, &stmt, 0);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_text(stmt, 1, key, -1, 0);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return 0;
    }

    char const* value = (char const*)sqlite3_column_text(stmt, 0);

    return strdup(value);
}
