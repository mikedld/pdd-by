#ifndef DATABASE_H
#define DATABASE_H

#include <glib.h>
#include <sqlite3.h>

gboolean database_exists();
void database_init(gchar const* dir);
void database_cleanup();
void database_use_cache(gboolean value);

sqlite3 *database_get();

void database_tx_begin();
void database_tx_commit();
void database_tx_rollback();

#endif // DATABASE_H
