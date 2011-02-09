#ifndef PDDBY_DATABASE_H
#define PDDBY_DATABASE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sqlite3.h>

int pddby_database_exists();
void pddby_database_init(char const* dir);
void pddby_database_cleanup();
void pddby_database_use_cache(int value);

sqlite3* pddby_database_get();

void pddby_database_tx_begin();
void pddby_database_tx_commit();
void pddby_database_tx_rollback();

void pddby_database_expect(int result, int expected_result, char const* scope, char const* message);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_DATABASE_H
