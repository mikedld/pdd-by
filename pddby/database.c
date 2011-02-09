#include "database.h"
#include "util/aux.h"
#include "callback.h"
#include "config.h"
#include "util/settings.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int use_cache = 0;
static char* share_dir = 0;
static char* database_file = 0;
static sqlite3* database = 0;
static int database_tx_count = 0;

int pddby_database_exists()
{
    if (!database_file)
    {
        database_file = pddby_aux_build_filename(pddby_aux_get_user_cache_dir(), "pdd.db", 0);
    }

    return access(database_file, R_OK) == 0;
}

void pddby_database_init(char const* dir)
{
    share_dir = strdup(dir);
}

void pddby_database_cleanup()
{
    if (database)
    {
        sqlite3_close(database);
    }
    if (database_file)
    {
        free(database_file);
    }
    if (share_dir)
    {
        free(share_dir);
    }
}

void pddby_database_use_cache(int value)
{
    use_cache = value;
}

sqlite3* pddby_database_get()
{
    if (database)
    {
        return database;
    }

    if (use_cache && pddby_database_exists())
    {
        int result = sqlite3_open(database_file, &database);
        if (result != SQLITE_OK)
        {
            pddby_report_error("unable to open database (%d)\n", result);
        }
        return database;
    }

    int result = sqlite3_open(use_cache ? database_file : ":memory:", &database);

    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to open database (%d)\n", result);
    }

    char* bootstrap_sql_filename = pddby_aux_build_filename(share_dir, "data", "10.sql", 0);
    char* bootstrap_sql;
    //GError *err = NULL;
    if (!pddby_aux_file_get_contents(bootstrap_sql_filename, &bootstrap_sql, 0))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }
    free(bootstrap_sql_filename);

    result = sqlite3_exec(database, bootstrap_sql, NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        sqlite3_close(database);
        unlink(database_file);
        pddby_report_error("unable to bootstrap database (%d)\n", result);
    }

    return database;
}

void pddby_database_tx_begin()
{
    if (!use_cache)
    {
        return;
    }
    database_tx_count++;
    if (database_tx_count > 1)
    {
        return;
    }
    int result = sqlite3_exec(pddby_database_get(), "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to begin transaction (%d)\n", result);
    }
}

void pddby_database_tx_commit()
{
    if (!use_cache)
    {
        return;
    }
    if (!database_tx_count)
    {
        pddby_report_error("unable to commit (no transaction in effect)\n");
    }
    database_tx_count--;
    if (database_tx_count)
    {
        return;
    }
    int result = sqlite3_exec(pddby_database_get(), "COMMIT TRANSACTION", NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        pddby_report_error("unable to commit transaction (%d)\n", result);
    }
}

void pddby_database_expect(int result, int expected_result, char const* scope, char const* message)
{
    if (result != expected_result)
    {
        pddby_report_error("%s: %s (%d: %s)\n", scope, message, result, sqlite3_errmsg(pddby_database_get()));
    }
}
