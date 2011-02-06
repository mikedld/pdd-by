#include "section.h"
#include "config.h"
#include "database.h"
#include "question.h"

static pdd_section_t *section_new_with_id(gint64 id, const gchar *name, const gchar *title_prefix, const gchar *title)
{
    pdd_section_t *section = g_new(pdd_section_t, 1);
    section->id = id;
    section->name = g_strdup(name);
    section->title_prefix = g_strdup(title_prefix);
    section->title = g_strdup(title);
    return section;
}

static pdd_section_t *section_copy(const pdd_section_t *section)
{
    return section_new_with_id(section->id, section->name, section->title_prefix, section->title);
}

pdd_section_t *section_new(const gchar *name, const gchar *title_prefix, const gchar *title)
{
    return section_new_with_id(0, name, title_prefix, title);
}

void section_free(pdd_section_t *section)
{
    g_free(section->name);
    g_free(section->title_prefix);
    g_free(section->title);
    g_free(section);
}

gboolean section_save(pdd_section_t *section)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `sections` (`name`, `title_prefix`, `title`) VALUES (?, ?, ?)", -1,
            &stmt, NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_text(stmt, 1, section->name, -1, NULL);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, section->title_prefix, -1, NULL);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 3, section->title, -1, NULL);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    section->id = sqlite3_last_insert_rowid(db);

    return TRUE;
}

pdd_section_t *section_find_by_id(gint64 id)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `name`, `title_prefix`, `title` FROM `sections` WHERE `rowid`=? "
            "LIMIT 1", -1, &stmt, NULL);
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

    const gchar *name = (const gchar *)sqlite3_column_text(stmt, 0);
    const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 1);
    const gchar *title = (const gchar *)sqlite3_column_text(stmt, 2);

    return section_new_with_id(id, name, title_prefix, title);
}

pdd_section_t *section_find_by_name(const gchar *name)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `title_prefix`, `title` FROM `sections` WHERE `name`=? "
            "LIMIT 1", -1, &stmt, NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_text(stmt, 1, name, -1, NULL);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    gint64 id = sqlite3_column_int64(stmt, 0);
    const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 1);
    const gchar *title = (const gchar *)sqlite3_column_text(stmt, 2);

    return section_new_with_id(id, name, title_prefix, title);
}

pdd_sections_t *section_find_all()
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `name`, `title_prefix`, `title` FROM `sections`", -1, &stmt,
            NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    pdd_sections_t *sections = g_ptr_array_new();

    while (TRUE)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        gint64 id = sqlite3_column_int64(stmt, 0);
        const gchar *name = (const gchar *)sqlite3_column_text(stmt, 1);
        const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 2);
        const gchar *title = (const gchar *)sqlite3_column_text(stmt, 3);

        g_ptr_array_add(sections, section_new_with_id(id, name, title_prefix, title));
    }

    return sections;
}

pdd_sections_t *section_copy_all(pdd_sections_t *sections)
{
    pdd_sections_t *sections_copy = g_ptr_array_new();
    gsize i;
    for (i = 0; i < sections->len; i++)
    {
        const pdd_section_t *section = g_ptr_array_index(sections, i);
        g_ptr_array_add(sections_copy, section_copy(section));
    }
    return sections_copy;
}

void section_free_all(pdd_sections_t *sections)
{
    g_ptr_array_foreach(sections, (GFunc)section_free, NULL);
    g_ptr_array_free(sections, TRUE);
}

gint32 section_get_question_count(pdd_section_t *section)
{
    static sqlite3_stmt *stmt = NULL;
    sqlite3 *db = database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM `questions_sections` WHERE `section_id`=?", -1, &stmt,
            NULL);
        database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, section->id);
    database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

    return sqlite3_column_int(stmt, 0);
}
