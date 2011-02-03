#include "settings.h"
#include "config.h"
#include "database.h"

gchar *get_settings(const gchar *key)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `value` FROM `settings` WHERE `key`=? LIMIT 1", -1, &stmt, NULL);
        if (result != SQLITE_OK)
        {
            g_error("answer: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
        }
    }

    result = sqlite3_reset(stmt);
    if (result != SQLITE_OK)
    {
        g_error("answer: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
    }

    result = sqlite3_bind_text(stmt, 1, key, -1, NULL);
    if (result != SQLITE_OK)
    {
        g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
    }

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        if (result != SQLITE_DONE)
        {
            g_error("answer: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
        }
        return NULL;
    }

    const gchar *value = (const gchar *)sqlite3_column_text(stmt, 0);

    return g_strdup(value);
}
