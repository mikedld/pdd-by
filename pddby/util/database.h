#ifndef PDDBY_DATABASE_H
#define PDDBY_DATABASE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

typedef void pddby_db_t;
typedef void pddby_db_stmt_t;

int pddby_db_exists();
void pddby_db_init(char const* share_dir, char const* cache_dir);
void pddby_db_cleanup();
void pddby_db_use_cache(int value);

void pddby_db_tx_begin();
void pddby_db_tx_commit();
void pddby_db_tx_rollback();

pddby_db_stmt_t* pddby_db_prepare(char const* sql);
int pddby_db_reset(pddby_db_stmt_t* stmt);

int pddby_db_bind_null(pddby_db_stmt_t* stmt, int field);
int pddby_db_bind_int(pddby_db_stmt_t* stmt, int field, int value);
int pddby_db_bind_int64(pddby_db_stmt_t* stmt, int field, int64_t value);
int pddby_db_bind_text(pddby_db_stmt_t* stmt, int field, char const* value);
int pddby_db_bind_blob(pddby_db_stmt_t* stmt, int field, void const* value, size_t value_size);

int pddby_db_column_int(pddby_db_stmt_t* stmt, int column);
int64_t pddby_db_column_int64(pddby_db_stmt_t* stmt, int column);
char const* pddby_db_column_text(pddby_db_stmt_t* stmt, int column);
void const* pddby_db_column_blob(pddby_db_stmt_t* stmt, int column);
size_t pddby_db_column_bytes(pddby_db_stmt_t* stmt, int column);

int pddby_db_step(pddby_db_stmt_t* stmt);
int64_t pddby_db_last_insert_id();

#ifdef __cplusplus
}
#endif

#endif // PDDBY_DATABASE_H
