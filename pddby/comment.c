#include "comment.h"
#include "config.h"
#include "database.h"

#include <stdlib.h>
#include <string.h>

static pddby_comment_t* pddby_comment_new_with_id(int64_t id, int32_t number, char const* text)
{
    pddby_comment_t* comment = malloc(sizeof(pddby_comment_t));
    comment->id = id;
    comment->number = number;
    comment->text = text ? strdup(text) : 0;
    return comment;
}

pddby_comment_t* pddby_comment_new(int32_t number, char const* text)
{
    return pddby_comment_new_with_id(0, number, text);
}

void pddby_comment_free(pddby_comment_t* comment)
{
    free(comment->text);
    free(comment);
}

int pddby_comment_save(pddby_comment_t* comment)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `comments` (`number`, `text`) VALUES (?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, comment->number);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, comment->text, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    comment->id = sqlite3_last_insert_rowid(db);

    return 1;
}

pddby_comment_t* pddby_comment_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `number`, `text` FROM `comments` WHERE `rowid`=? LIMIT 1", -1, &stmt,
            NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    int32_t number = sqlite3_column_int(stmt, 0);
    char const* text = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_comment_new_with_id(id, number, text);
}

pddby_comment_t* pddby_comment_find_by_number(int32_t number)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text` FROM `comments` WHERE `number`=? LIMIT 1", -1, &stmt,
            NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, number);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    int64_t id = sqlite3_column_int64(stmt, 0);
    char const* text = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_comment_new_with_id(id, number, text);
}
