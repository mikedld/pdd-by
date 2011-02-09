#include "traffreg.h"
#include "config.h"
#include "database.h"
#include "question.h"

#include <stdlib.h>
#include <string.h>

static pddby_traffreg_t* pddby_traffreg_new_with_id(int64_t id, int32_t number, char const* text)
{
    pddby_traffreg_t *traffreg = malloc(sizeof(pddby_traffreg_t));
    traffreg->id = id;
    traffreg->number = number;
    traffreg->text = strdup(text);
    return traffreg;
}

static pddby_traffreg_t* pddby_traffreg_copy(pddby_traffreg_t const* traffreg)
{
    return pddby_traffreg_new_with_id(traffreg->id, traffreg->number, traffreg->text);
}

pddby_traffreg_t* pddby_traffreg_new(int32_t number, char const* text)
{
    return pddby_traffreg_new_with_id(0, number, text);
}

void pddby_traffreg_free(pddby_traffreg_t* traffreg)
{
    free(traffreg->text);
    free(traffreg);
}

int pddby_traffreg_save(pddby_traffreg_t* traffreg)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `traffregs` (`number`, `text`) VALUES (?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, traffreg->number);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, traffreg->text, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    traffreg->id = sqlite3_last_insert_rowid(db);

    return 1;
}

int pddby_traffreg_set_images(pddby_traffreg_t* traffreg, pddby_images_t* images)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `images_traffregs` (`image_id`, `traffreg_id`) VALUES (?, ?)", -1,
            &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    for (size_t i = 0; i < pddby_array_size(images); i++)
    {
        pddby_image_t* image = pddby_array_index(images, i);

        result = sqlite3_reset(stmt);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

        result = sqlite3_bind_int64(stmt, 1, image->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_bind_int64(stmt, 2, traffreg->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_step(stmt);
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
    }

    return 1;
}

pddby_traffreg_t* pddby_traffreg_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `number`, `text` FROM `traffregs` WHERE `rowid`=? LIMIT 1", -1, &stmt,
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
    char const*text = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_traffreg_new_with_id(id, number, text);
}

pddby_traffreg_t* pddby_traffreg_find_by_number(int32_t number)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text` FROM `traffregs` WHERE `number`=? LIMIT 1", -1, &stmt,
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
    char const*text = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_traffreg_new_with_id(id, number, text);
}

pddby_traffregs_t* pddby_traffreg_find_by_question(int64_t question_id)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT t.`rowid`, t.`number`, t.`text` FROM `traffregs` t INNER JOIN "
            "`questions_traffregs` qt ON t.`rowid`=qt.`traffreg_id` WHERE qt.`question_id`=?", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, question_id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    pddby_traffregs_t* traffregs = pddby_array_new(0);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        int32_t number = sqlite3_column_int(stmt, 1);
        char const*text = (char const*)sqlite3_column_text(stmt, 2);

        pddby_array_add(traffregs, pddby_traffreg_new_with_id(id, number, text));
    }

    return traffregs;
}

pddby_traffregs_t* pddby_traffreg_copy_all(pddby_traffregs_t* traffregs)
{
    pddby_traffregs_t* traffregs_copy = pddby_array_new(0);
    for (size_t i = 0; i < pddby_array_size(traffregs); i++)
    {
        pddby_traffreg_t const* traffreg = pddby_array_index(traffregs, i);
        pddby_array_add(traffregs_copy, pddby_traffreg_copy(traffreg));
    }
    return traffregs_copy;
}

void pddby_traffreg_free_all(pddby_traffregs_t* traffregs)
{
    //pddby_array_foreach(traffregs, (GFunc)traffreg_free, NULL);
    pddby_array_free(traffregs);
}
