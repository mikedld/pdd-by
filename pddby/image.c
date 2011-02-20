#include "image.h"

#include "config.h"
#include "util/aux.h"
#include "util/database.h"
#include "util/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_image_t* pddby_image_new_with_id(int64_t id, char const* name, void const* data, size_t data_length)
{
    pddby_image_t *image = calloc(1, sizeof(pddby_image_t));
    if (!image)
    {
        goto error;
    }

    image->name = name ? strdup(name) : NULL;
    if (name && !image->name)
    {
        goto error;
    }

    if (data && data_length)
    {
        image->data = malloc(data_length);
        if (!image->data)
        {
            goto error;
        }

        memcpy(image->data, data, data_length);
        image->data_length = data_length;
    }

    image->id = id;

    return image;

error:
    // TODO: report error
    if (image)
    {
        pddby_image_free(image);
    }
    return NULL;
}

pddby_image_t* pddby_image_new(char const* name, void const* data, size_t data_length)
{
    return pddby_image_new_with_id(0, name, data, data_length);
}

void pddby_image_free(pddby_image_t* image)
{
    assert(image);

    if (image->name)
    {
        free(image->name);
    }
    if (image->data)
    {
        free(image->data);
    }
    free(image);
}

int pddby_image_save(pddby_image_t* image)
{
    assert(image);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare("INSERT INTO `images` (`name`, `data`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    char* image_name = pddby_string_downcase(image->name);
    if (!image_name)
    {
        goto error;
    }

    free(image->name);
    image->name = image_name;

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_text(db_stmt, 1, image->name) ||
        !pddby_db_bind_blob(db_stmt, 2, image->data, image->data_length))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    image->id = pddby_db_last_insert_id();

    return 1;

error:
    // TODO: report error
    return 0;
}

pddby_image_t* pddby_image_find_by_id(int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare("SELECT `name`, `data` FROM `images` WHERE `rowid`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, id))
    {
        goto error;
    }

    switch (pddby_db_step(db_stmt))
    {
    case -1:
        goto error;
    case 0:
        return NULL;
    }

    char const* name = pddby_db_column_text(db_stmt, 0);
    void const* data = pddby_db_column_blob(db_stmt, 1);
    size_t data_length = pddby_db_column_bytes(db_stmt, 1);

    return pddby_image_new_with_id(id, name, data, data_length);

error:
    // TODO: report error
    return NULL;
}

pddby_image_t* pddby_image_find_by_name(char const* name)
{
    assert(name);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare("SELECT `rowid`, `data` FROM `images` WHERE `name`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    char *image_name = pddby_string_downcase(name);
    if (!image_name)
    {
        goto error;
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_text(db_stmt, 1, image_name))
    {
        goto error;
    }

    switch (pddby_db_step(db_stmt))
    {
    case -1:
        goto error;
    case 0:
        return NULL;
    }

    int64_t id = pddby_db_column_int64(db_stmt, 0);
    void const* data = pddby_db_column_blob(db_stmt, 1);
    size_t data_length = pddby_db_column_bytes(db_stmt, 1);

    pddby_image_t* image = pddby_image_new_with_id(id, image_name, data, data_length);

    free(image_name);

    return image;

error:
    // TODO: report error
    return NULL;
}

pddby_images_t* pddby_images_new()
{
    return pddby_array_new((pddby_array_free_func_t)pddby_image_free);
}

pddby_images_t* pddby_images_find_by_traffreg(int64_t traffreg_id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare("SELECT i.`rowid`, i.`name`, i.`data` FROM `images` i INNER JOIN "
            "`images_traffregs` it ON i.`rowid`=it.`image_id` WHERE it.`traffreg_id`=?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, traffreg_id))
    {
        goto error;
    }

    pddby_images_t* images = pddby_images_new();
    if (!images)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        char const* name = pddby_db_column_text(db_stmt, 1);
        void const* data = pddby_db_column_blob(db_stmt, 2);
        size_t data_length = pddby_db_column_bytes(db_stmt, 2);

        if (!pddby_array_add(images, pddby_image_new_with_id(id, name, data, data_length)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_images_free(images);
        goto error;
    }

    return images;

error:
    // TODO: report error
    return NULL;
}

void pddby_images_free(pddby_images_t* images)
{
    assert(images);

    pddby_array_free(images, 1);
}
