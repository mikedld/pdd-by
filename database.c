#include "database.h"
#include "common.h"
#include "section.h"
#include "settings.h"
#include "topic.h"

#include <glib/gstdio.h>
#include <unistd.h>

extern gboolean use_cache;

static gchar *database_file = NULL;
static sqlite3 *database = NULL;
static gint database_tx_count = 0;

gboolean database_exists()
{
	if (database_file == NULL)
	{
		database_file = g_build_filename(g_get_user_cache_dir(), "pdd.db", NULL);
	}

	return g_access(database_file, R_OK) == 0;
}

void database_cleanup()
{
	if (database)
	{
		sqlite3_close(database);
	}
	if (database_file)
	{
		g_free(database_file);
	}
}

sqlite3 *database_get()
{
	if (database)
	{
		return database;
	}

	if (use_cache && database_exists())
	{
		int result = sqlite3_open(database_file, &database);
		if (result != SQLITE_OK)
		{
			g_error("unable to open database (%d)\n", result);
		}
		return database;
	}

	int result = sqlite3_open(use_cache ? database_file : ":memory:", &database);

	if (result != SQLITE_OK)
	{
		g_error("unable to open database (%d)\n", result);
	}

	gchar *bootstrap_sql_filename = g_build_filename(get_share_dir(), "data", "10.sql", NULL);
	gchar *bootstrap_sql;
	GError *err = NULL;
	if (!g_file_get_contents(bootstrap_sql_filename, &bootstrap_sql, NULL, &err))
	{
		g_error("%s\n", err->message);
	}

	result = sqlite3_exec(database, bootstrap_sql, NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		sqlite3_close(database);
		g_unlink(database_file);
		g_error("unable to bootstrap database (%d)\n", result);
	}

	return database;
}

void database_tx_begin()
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
	int result = sqlite3_exec(database_get(), "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to begin transaction (%d)\n", result);
	}
}

void database_tx_commit()
{
    if (!use_cache)
    {
        return;
    }
	if (!database_tx_count)
	{
		g_error("unable to commit (no transaction in effect)\n");
	}
	database_tx_count--;
	if (database_tx_count)
	{
		return;
	}
	int result = sqlite3_exec(database_get(), "COMMIT TRANSACTION", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to commit transaction (%d)\n", result);
	}
}
