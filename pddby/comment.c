#include "comment.h"
#include "config.h"
#include "database.h"

static pdd_comment_t *comment_new_with_id(gint64 id, gint32 number, const gchar *text)
{
    pdd_comment_t *comment = g_new(pdd_comment_t, 1);
    comment->id = id;
    comment->number = number;
    comment->text = g_strdup(text);
    return comment;
}

pdd_comment_t *comment_new(gint32 number, const gchar *text)
{
    return comment_new_with_id(0, number, text);
}

void comment_free(pdd_comment_t *comment)
{
    g_free(comment->text);
    g_free(comment);
}

gboolean comment_save(pdd_comment_t *comment)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `comments` (`number`, `text`) VALUES (?, ?)", -1, &stmt, NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, comment->number);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, comment->text, -1, NULL);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    comment->id = sqlite3_last_insert_rowid(db);

    return TRUE;
}

pdd_comment_t *comment_find_by_id(gint64 id)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `number`, `text` FROM `comments` WHERE `rowid`=? LIMIT 1", -1, &stmt,
            NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, id);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    gint32 number = sqlite3_column_int(stmt, 0);
    const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);

    return comment_new_with_id(id, number, text);
}

pdd_comment_t *comment_find_by_number(gint32 number)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text` FROM `comments` WHERE `number`=? LIMIT 1", -1, &stmt,
            NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, number);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    gint32 id = sqlite3_column_int64(stmt, 0);
    const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);

    return comment_new_with_id(id, number, text);
}
