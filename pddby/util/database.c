#include "database.h"

#include "aux.h"
#include "../callback.h"
#include "config.h"
#include "settings.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static int pddby_use_cache = 0;
static char* pddby_share_dir = 0;
static char* pddby_database_file = 0;
static sqlite3* pddby_database = 0;
static int pddby_database_tx_count = 0;

sqlite3* pddby_db_get();

static int pddby_db_expect(int result, int expected_result, char const* scope, char const* message)
{
    if (result != expected_result)
    {
        pddby_report_error("%s: %s (%d: %s)\n", scope, message, result, sqlite3_errmsg(pddby_db_get()));
        return 0;
    }
    return 1;
}

int pddby_db_exists()
{
    return access(pddby_database_file, R_OK) == 0;
}

void pddby_db_init(char const* share_dir, char const* cache_dir)
{
    pddby_share_dir = strdup(share_dir);
    pddby_database_file = pddby_aux_build_filename(cache_dir, "pddby.sqlite", 0);
}

void pddby_db_cleanup()
{
    if (pddby_database)
    {
        sqlite3_close(pddby_database);
    }
    if (pddby_database_file)
    {
        free(pddby_database_file);
    }
    if (pddby_share_dir)
    {
        free(pddby_share_dir);
    }
}

void pddby_db_use_cache(int value)
{
    pddby_use_cache = value;
}

sqlite3* pddby_db_get()
{
    if (pddby_database)
    {
        return pddby_database;
    }

    if (pddby_use_cache && pddby_db_exists())
    {
        int result = sqlite3_open(pddby_database_file, &pddby_database);
        if (result != SQLITE_OK)
        {
            pddby_report_error("unable to open database (%d)\n", result);
        }
        return pddby_database;
    }

    int result = sqlite3_open(pddby_use_cache ? pddby_database_file : ":memory:", &pddby_database);

    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to open database (%d)\n", result);
    }

    char* bootstrap_sql_filename = pddby_aux_build_filename(pddby_share_dir, "data", "10.sql", 0);
    char* bootstrap_sql;
    //GError *err = NULL;
    if (!pddby_aux_file_get_contents(bootstrap_sql_filename, &bootstrap_sql, 0))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }
    free(bootstrap_sql_filename);

    char* error_text;
    result = sqlite3_exec(pddby_database, bootstrap_sql, NULL, NULL, &error_text);
    if (result != SQLITE_OK)
    {
        sqlite3_close(pddby_database);
        unlink(pddby_database_file);
        pddby_report_error("unable to bootstrap database: %s (%d)\n", error_text, result);
    }
    free(bootstrap_sql);

    return pddby_database;
}

void pddby_db_tx_begin()
{
    if (!pddby_use_cache)
    {
        return;
    }
    pddby_database_tx_count++;
    if (pddby_database_tx_count > 1)
    {
        return;
    }
    int result = sqlite3_exec(pddby_db_get(), "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to begin transaction (%d)\n", result);
    }
}

void pddby_db_tx_commit()
{
    if (!pddby_use_cache)
    {
        return;
    }
    if (!pddby_database_tx_count)
    {
        pddby_report_error("unable to commit (no transaction in effect)\n");
    }
    pddby_database_tx_count--;
    if (pddby_database_tx_count)
    {
        return;
    }
    int result = sqlite3_exec(pddby_db_get(), "COMMIT TRANSACTION", NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to commit transaction (%d)\n", result);
    }
}

pddby_db_stmt_t* pddby_db_prepare(char const* sql)
{
    sqlite3_stmt* result;
    int error = sqlite3_prepare_v2(pddby_db_get(), sql, -1, &result, NULL);
    pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    return result;
}

int pddby_db_reset(pddby_db_stmt_t* stmt)
{
    int error = sqlite3_reset(stmt);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");
}

int pddby_db_bind_null(pddby_db_stmt_t* stmt, int field)
{
    int error = sqlite3_bind_null(stmt, field);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_int(pddby_db_stmt_t* stmt, int field, int value)
{
    int error = sqlite3_bind_int(stmt, field, value);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_int64(pddby_db_stmt_t* stmt, int field, int64_t value)
{
    int error = sqlite3_bind_int64(stmt, field, value);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_text(pddby_db_stmt_t* stmt, int field, char const* value)
{
    int error = sqlite3_bind_text(stmt, field, value, -1, NULL);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_bind_blob(pddby_db_stmt_t* stmt, int field, void const* value, size_t value_size)
{
    int error = sqlite3_bind_blob(stmt, field, value, value_size, NULL);
    return pddby_db_expect(error, SQLITE_OK, __FUNCTION__, "unable to bind param");
}

int pddby_db_column_int(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_int(stmt, column);
}

int64_t pddby_db_column_int64(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_int64(stmt, column);
}

char const* pddby_db_column_text(pddby_db_stmt_t* stmt, int column)
{
    return (char const*)sqlite3_column_text(stmt, column);
}

void const* pddby_db_column_blob(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_blob(stmt, column);
}

size_t pddby_db_column_bytes(pddby_db_stmt_t* stmt, int column)
{
    return sqlite3_column_bytes(stmt, column);
}

int pddby_db_step(pddby_db_stmt_t* stmt)
{
    int error = sqlite3_step(stmt);
    if (error == SQLITE_DONE)
    {
        return 0;
    }
    return pddby_db_expect(error, SQLITE_ROW, __FUNCTION__, "unable to perform statement") ? 1 : -1;
}

int64_t pddby_db_last_insert_id()
{
    return sqlite3_last_insert_rowid(pddby_db_get());
}
