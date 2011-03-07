#include "traffreg.h"

#include "config.h"
#include "private/util/database.h"
#include "private/util/report.h"
#include "question.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_traffreg_t* pddby_traffreg_new_with_id(pddby_t* pddby, int64_t id, int32_t number, char const* text)
{
    pddby_traffreg_t *traffreg = malloc(sizeof(pddby_traffreg_t));
    if (!traffreg)
    {
        goto error;
    }

    traffreg->text = text ? strdup(text) : 0;
    if (text && !traffreg->text)
    {
        goto error;
    }

    traffreg->id = id;
    traffreg->number = number;
    traffreg->pddby = pddby;

    return traffreg;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create traffreg object");
    if (traffreg)
    {
        pddby_traffreg_free(traffreg);
    }
    return NULL;
}

pddby_traffreg_t* pddby_traffreg_new(pddby_t* pddby, int32_t number, char const* text)
{
    return pddby_traffreg_new_with_id(pddby, 0, number, text);
}

void pddby_traffreg_free(pddby_traffreg_t* traffreg)
{
    assert(traffreg);

    if (traffreg->text)
    {
        free(traffreg->text);
    }
    free(traffreg);
}

int pddby_traffreg_save(pddby_traffreg_t* traffreg)
{
    assert(traffreg);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(traffreg->pddby, "INSERT INTO `traffregs` (`number`, `text`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int(db_stmt, 1, traffreg->number) ||
        !pddby_db_bind_text(db_stmt, 2, traffreg->text))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    traffreg->id = pddby_db_last_insert_id(traffreg->pddby);

    return 1;

error:
    pddby_report(traffreg->pddby, pddby_message_type_error, "unable to save traffreg object");
    return 0;
}

int pddby_traffreg_set_images(pddby_traffreg_t* traffreg, pddby_images_t* images)
{
    assert(traffreg);
    assert(images);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(traffreg->pddby, "INSERT INTO `images_traffregs` (`image_id`, `traffreg_id`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    for (size_t i = 0, size = pddby_array_size(images); i < size; i++)
    {
        pddby_image_t* image = pddby_array_index(images, i);

        if (!pddby_db_reset(db_stmt) ||
            !pddby_db_bind_int64(db_stmt, 1, image->id) ||
            !pddby_db_bind_int64(db_stmt, 2, traffreg->id))
        {
            goto error;
        }

        int ret = pddby_db_step(db_stmt);
        if (ret == -1)
        {
            goto error;
        }

        assert(ret == 0);
    }

    return 1;

error:
    pddby_report(traffreg->pddby, pddby_message_type_error, "unable to set traffreg object images");
    return 0;
}

pddby_traffreg_t* pddby_traffreg_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `number`, `text` FROM `traffregs` WHERE `rowid`=? LIMIT 1");
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

    int32_t number = pddby_db_column_int(db_stmt, 0);
    char const*text = pddby_db_column_text(db_stmt, 1);

    return pddby_traffreg_new_with_id(pddby, id, number, text);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find traffreg object with id = %lld", id);
    return NULL;
}

pddby_traffreg_t* pddby_traffreg_find_by_number(pddby_t* pddby, int32_t number)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `text` FROM `traffregs` WHERE `number`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int(db_stmt, 1, number))
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
    char const*text = pddby_db_column_text(db_stmt, 1);

    return pddby_traffreg_new_with_id(pddby, id, number, text);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find traffreg object with number = %d", number);
    return NULL;
}

pddby_traffregs_t* pddby_traffregs_new(pddby_t* pddby)
{
    return pddby_array_new(pddby, (pddby_array_free_func_t)pddby_traffreg_free);
}

pddby_traffregs_t* pddby_traffregs_find_by_question(pddby_t* pddby, int64_t question_id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT t.`rowid`, t.`number`, t.`text` FROM `traffregs` t INNER JOIN "
            "`questions_traffregs` qt ON t.`rowid`=qt.`traffreg_id` WHERE qt.`question_id`=?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, question_id))
    {
        goto error;
    }

    pddby_traffregs_t* traffregs = pddby_traffregs_new(pddby);
    if (!traffregs)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        int32_t number = pddby_db_column_int(db_stmt, 1);
        char const*text = pddby_db_column_text(db_stmt, 2);

        if (!pddby_array_add(traffregs, pddby_traffreg_new_with_id(pddby, id, number, text)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_traffregs_free(traffregs);
        goto error;
    }

    return traffregs;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find traffreg objects with question id = %lld",
        question_id);
    return NULL;
}

void pddby_traffregs_free(pddby_traffregs_t* traffregs)
{
    assert(traffregs);

    pddby_array_free(traffregs, 1);
}
