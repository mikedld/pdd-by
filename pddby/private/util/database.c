#include "database.h"

#include "aux.h"
#include "config.h"
#include "report.h"
#include "settings.h"

#include "private/pddby.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_db
{
    int use_cache;
    char* share_dir;
    char* database_file;
    sqlite3* database;
    int database_tx_count;
};

struct pddby_db_stmt
{
    pddby_t* pddby;
    sqlite3_stmt* statement;
};

sqlite3* pddby_db_get(pddby_t* pddby);

static int pddby_db_expect(pddby_t* pddby, int result, int expected_result, char const* scope, char const* message)
{
    if (result != expected_result)
    {
        pddby_report(pddby, pddby_message_type_error, "%s: %s (%d: %s)", scope, message, result, sqlite3_errmsg(pddby_db_get(pddby)));
        return 0;
    }
    return 1;
}

int pddby_db_exists(pddby_t* pddby)
{
    return access(pddby->database->database_file, R_OK) == 0;
}

void pddby_db_init(pddby_t* pddby, char const* share_dir, char const* cache_dir)
{
    pddby->database = calloc(1, sizeof(pddby_db_t));

    pddby->database->share_dir = strdup(share_dir);
    pddby->database->database_file = pddby_aux_build_filename(pddby, cache_dir, "pddby.sqlite", 0);
}

void pddby_db_cleanup(pddby_t* pddby)
{
    if (pddby->database->database)
    {
        sqlite3_close(pddby->database->database);
    }
    if (pddby->database->database_file)
    {
        free(pddby->database->database_file);
    }
    if (pddby->database->share_dir)
    {
        free(pddby->database->share_dir);
    }
    free(pddby->database);
}

void pddby_db_use_cache(pddby_t* pddby, int value)
{
    pddby->database->use_cache = value;
}

sqlite3* pddby_db_get(pddby_t* pddby)
{
    if (pddby->database->database)
    {
        return pddby->database->database;
    }

    if (pddby->database->use_cache && pddby_db_exists(pddby))
    {
        int result = sqlite3_open(pddby->database->database_file, &pddby->database->database);
        if (!pddby_db_expect(pddby, result, SQLITE_OK, __FUNCTION__, "unable to open database"))
        {
            return NULL;
        }
        return pddby->database->database;
    }

    int result = sqlite3_open(pddby->database->use_cache ? pddby->database->database_file : ":memory:", &pddby->database->database);
    if (!pddby_db_expect(pddby, result, SQLITE_OK, __FUNCTION__, "unable to open database"))
    {
        return NULL;
    }

    char* bootstrap_sql_filename = pddby_aux_build_filename(pddby, pddby->database->share_dir, "data", "10.sql", 0);
    char* bootstrap_sql;
    if (!pddby_aux_file_get_contents(pddby, bootstrap_sql_filename, &bootstrap_sql, 0))
    {
        pddby_report(pddby, pddby_message_type_error, "unable to open database");
        sqlite3_close(pddby->database->database);
        free(bootstrap_sql_filename);
        return NULL;
    }
    free(bootstrap_sql_filename);

    char* error_text;
    result = sqlite3_exec(pddby->database->database, bootstrap_sql, NULL, NULL, &error_text);
    if (!pddby_db_expect(pddby, result, SQLITE_OK, __FUNCTION__, "unable to bootstrap database"))
    {
        sqlite3_close(pddby->database->database);
        unlink(pddby->database->database_file);
        free(bootstrap_sql);
        return NULL;
    }
    free(bootstrap_sql);

    return pddby->database->database;
}

int pddby_db_tx_begin(pddby_t* pddby)
{
    if (!pddby->database->use_cache)
    {
        return 1;
    }
    pddby->database->database_tx_count++;
    if (pddby->database->database_tx_count > 1)
    {
        return 1;
    }
    int result = sqlite3_exec(pddby_db_get(pddby), "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, NULL);
    return pddby_db_expect(pddby, result, SQLITE_OK, __FUNCTION__, "unable to begin transaction");
}

int pddby_db_tx_commit(pddby_t* pddby)
{
    if (!pddby->database->use_cache)
    {
        return 1;
    }
    if (!pddby->database->database_tx_count)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to commit (no transaction in effect)");
        return 0;
    }
    pddby->database->database_tx_count--;
    if (pddby->database->database_tx_count)
    {
        return 1;
    }
    int result = sqlite3_exec(pddby_db_get(pddby), "COMMIT TRANSACTION", NULL, NULL, NULL);
    return pddby_db_expect(pddby, result, SQLITE_OK, __FUNCTION__, "unable to commit transaction");
}

pddby_db_stmt_t* pddby_db_prepare(pddby_t* pddby, char const* sql)
{
    pddby_db_stmt_t* result = malloc(sizeof(pddby_db_stmt_t));
    result->pddby = pddby;
    int error = sqlite3_prepare_v2(pddby_db_get(pddby), sql, -1, &result->statement, NULL);
    if (!pddby_db_expect(pddby, error, SQLITE_OK, __FUNCTION__, "unable to prepare statement"))
    {
        free(result);
        return NULL;
    }
    return result;
}

int pddby_db_reset(pddby_db_stmt_t* stmt)
{
    int error = sqlite3_reset(stmt->statement);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");
}

int pddby_db_bind_null(pddby_db_stmt_t* stmt, int field)
{
    int error = sqlite3_bind_null(stmt->statement, field);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_int(pddby_db_stmt_t* stmt, int field, int value)
{
    int error = sqlite3_bind_int(stmt->statement, field, value);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_int64(pddby_db_stmt_t* stmt, int field, int64_t value)
{
    int error = sqlite3_bind_int64(stmt->statement, field, value);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_text(pddby_db_stmt_t* stmt, int field, char const* value)
{
    int error = sqlite3_bind_text(stmt->statement, field, value, -1, NULL);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_blob(pddby_db_stmt_t* stmt, int field, void const* value, size_t value_size)
{
    int error = sqlite3_bind_blob(stmt->statement, field, value, value_size, NULL);
    return pddby_db_expect(stmt->pddby, error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_column_int(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_int(stmt->statement, column);
}

int64_t pddby_db_column_int64(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_int64(stmt->statement, column);
}

char const* pddby_db_column_text(pddby_db_stmt_t* stmt, int column)
{
    return (char const*)sqlite3_column_text(stmt->statement, column);
}

void const* pddby_db_column_blob(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_blob(stmt->statement, column);
}

size_t pddby_db_column_bytes(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_bytes(stmt->statement, column);
}

int pddby_db_step(pddby_db_stmt_t* stmt)
{
    int error = sqlite3_step(stmt->statement);
    if (error == SQLITE_DONE)
    {
        return 0;
    }
    return pddby_db_expect(stmt->pddby, error, SQLITE_ROW, __FUNCTION__, "unable to perform statement") ? 1 : -1;
}

int64_t pddby_db_last_insert_id(pddby_t* pddby)
{
    return sqlite3_last_insert_rowid(pddby_db_get(pddby));
}
