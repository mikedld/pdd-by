#include "image.h"

#include "config.h"
#include "database.h"
#include "util/aux.h"
#include "util/string.h"

#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_image_t* pddby_image_new_with_id(int64_t id, char const* name, void const* data, size_t data_length)
{
    pddby_image_t *image = malloc(sizeof(pddby_image_t));
    image->id = id;
    image->name = strdup(name);
    //image->data = g_memdup(data, data_length);
    image->data = malloc(data_length);
    memcpy(image->data, data, data_length);
    image->data_length = data_length;
    return image;
}

static pddby_image_t* pddby_image_copy(pddby_image_t const* image)
{
    return pddby_image_new_with_id(image->id, image->name, image->data, image->data_length);
}

pddby_image_t* pddby_image_new(char const* name, void const* data, size_t data_length)
{
    return pddby_image_new_with_id(0, name, data, data_length);
}

void pddby_image_free(pddby_image_t* image)
{
    free(image->name);
    free(image->data);
    free(image);
}

int pddby_image_save(pddby_image_t* image)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `images` (`name`, `data`) VALUES (?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    char* image_name = pddby_string_downcase(image->name);
    free(image->name);
    image->name = image_name;
    result = sqlite3_bind_text(stmt, 1, image->name, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_blob(stmt, 2, image->data, image->data_length, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    image->id = sqlite3_last_insert_rowid(db);

    return 1;
}

pddby_image_t* pddby_image_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `name`, `data` FROM `images` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
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
        return 0;
    }

    char const* name = (char const*)sqlite3_column_text(stmt, 0);
    void const* data = sqlite3_column_blob(stmt, 1);
    size_t data_length = sqlite3_column_bytes(stmt, 1);

    return pddby_image_new_with_id(id, name, data, data_length);
}

pddby_image_t* pddby_image_find_by_name(char const* name)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `data` FROM `images` WHERE `name`=? LIMIT 1", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    char *image_name = pddby_string_downcase(name);
    result = sqlite3_bind_text(stmt, 1, image_name, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    if (result != SQLITE_ROW)
    {
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
        return 0;
    }

    int64_t id = sqlite3_column_int64(stmt, 0);
    void const* data = sqlite3_column_blob(stmt, 1);
    size_t data_length = sqlite3_column_bytes(stmt, 1);

    pddby_image_t* image = pddby_image_new_with_id(id, image_name, data, data_length);

    free(image_name);

    return image;
}

pddby_images_t* pddby_image_find_by_traffreg(int64_t traffreg_id)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT i.`rowid`, i.`name`, i.`data` FROM `images` i INNER JOIN "
            "`images_traffregs` it ON i.`rowid`=it.`image_id` WHERE it.`traffreg_id`=?", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, traffreg_id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    pddby_images_t* images = pddby_array_new((pddby_array_free_func_t)pddby_image_free);

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
        void const* data = sqlite3_column_blob(stmt, 2);
        size_t data_length = sqlite3_column_bytes(stmt, 2);

        pddby_array_add(images, pddby_image_new_with_id(id, name, data, data_length));
    }

    return images;
}

pddby_images_t* pddby_image_copy_all(pddby_images_t const* images)
{
    pddby_images_t* images_copy = pddby_array_new((pddby_array_free_func_t)pddby_image_free);
    for (size_t i = 0; i < pddby_array_size(images); i++)
    {
        pddby_image_t const* image = pddby_array_index(images, i);
        pddby_array_add(images_copy, pddby_image_copy(image));
    }
    return images_copy;
}

void pddby_image_free_all(pddby_images_t *images)
{
    //pddby_array_foreach(images, (GFunc)image_free, NULL);
    pddby_array_free(images, 1);
}
