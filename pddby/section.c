#include "section.h"
#include "config.h"
#include "database.h"
#include "question.h"

#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_section_t* pddby_section_new_with_id(int64_t id, char const* name, char const* title_prefix,
    char const* title)
{
    pddby_section_t* section = malloc(sizeof(pddby_section_t));
    section->id = id;
    section->name = strdup(name);
    section->title_prefix = strdup(title_prefix);
    section->title = strdup(title);
    return section;
}

static pddby_section_t* pddby_section_copy(pddby_section_t const* section)
{
    return pddby_section_new_with_id(section->id, section->name, section->title_prefix, section->title);
}

pddby_section_t* pddby_section_new(char const* name, char const* title_prefix, char const* title)
{
    return pddby_section_new_with_id(0, name, title_prefix, title);
}

void pddby_section_free(pddby_section_t* section)
{
    free(section->name);
    free(section->title_prefix);
    free(section->title);
    free(section);
}

int pddby_section_save(pddby_section_t* section)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `sections` (`name`, `title_prefix`, `title`) VALUES (?, ?, ?)", -1,
            &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_text(stmt, 1, section->name, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, section->title_prefix, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 3, section->title, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    section->id = sqlite3_last_insert_rowid(db);

    return 1;
}

pddby_section_t* pddby_section_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `name`, `title_prefix`, `title` FROM `sections` WHERE `rowid`=? "
            "LIMIT 1", -1, &stmt, NULL);
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

    char const* name = (char const*)sqlite3_column_text(stmt, 0);
    char const* title_prefix = (char const*)sqlite3_column_text(stmt, 1);
    char const* title = (char const*)sqlite3_column_text(stmt, 2);

    return pddby_section_new_with_id(id, name, title_prefix, title);
}

pddby_section_t* pddby_section_find_by_name(char const* name)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `title_prefix`, `title` FROM `sections` WHERE `name`=? "
            "LIMIT 1", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_text(stmt, 1, name, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return NULL;
    }

    int64_t id = sqlite3_column_int64(stmt, 0);
    char const* title_prefix = (char const*)sqlite3_column_text(stmt, 1);
    char const* title = (char const*)sqlite3_column_text(stmt, 2);

    return pddby_section_new_with_id(id, name, title_prefix, title);
}

pddby_sections_t* pddby_section_find_all()
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `name`, `title_prefix`, `title` FROM `sections`", -1, &stmt,
            NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    pddby_sections_t* sections = pddby_array_new((pddby_array_free_func_t)pddby_section_free);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        char const* name = (char const*)sqlite3_column_text(stmt, 1);
        char const* title_prefix = (char const*)sqlite3_column_text(stmt, 2);
        char const* title = (char const*)sqlite3_column_text(stmt, 3);

        pddby_array_add(sections, pddby_section_new_with_id(id, name, title_prefix, title));
    }

    return sections;
}

pddby_sections_t* pddby_section_copy_all(pddby_sections_t* sections)
{
    pddby_sections_t* sections_copy = pddby_array_new((pddby_array_free_func_t)pddby_section_free);
    for (size_t i = 0; i < pddby_array_size(sections); i++)
    {
        pddby_section_t const* section = pddby_array_index(sections, i);
        pddby_array_add(sections_copy, pddby_section_copy(section));
    }
    return sections_copy;
}

void pddby_section_free_all(pddby_sections_t* sections)
{
    //pddby_array_foreach(sections, (GFunc)pddby_section_free, NULL);
    pddby_array_free(sections, 1);
}

size_t pddby_section_get_question_count(pddby_section_t* section)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM `questions_sections` WHERE `section_id`=?", -1, &stmt,
            NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, section->id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

    return sqlite3_column_int(stmt, 0);
}
