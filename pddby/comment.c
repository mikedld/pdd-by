#include "comment.h"

#include "config.h"
#include "private/util/database.h"
#include "private/util/report.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_comment_t* pddby_comment_new_with_id(pddby_t* pddby, int64_t id, int32_t number, char const* text)
{
    pddby_comment_t* comment = calloc(1, sizeof(pddby_comment_t));
    if (!comment)
    {
        goto error;
    }

    comment->text = text ? strdup(text) : NULL;
    if (text && !comment->text)
    {
        goto error;
    }

    comment->id = id;
    comment->number = number;
    comment->pddby = pddby;

    return comment;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create comment object");
    if (comment)
    {
        pddby_comment_free(comment);
    }
    return NULL;
}

pddby_comment_t* pddby_comment_new(pddby_t* pddby, int32_t number, char const* text)
{
    return pddby_comment_new_with_id(pddby, 0, number, text);
}

void pddby_comment_free(pddby_comment_t* comment)
{
    assert(comment);

    if (comment->text)
    {
        free(comment->text);
    }
    free(comment);
}

int pddby_comment_save(pddby_comment_t* comment)
{
    assert(comment);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(comment->pddby, "INSERT INTO `comments` (`number`, `text`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int(db_stmt, 1, comment->number) ||
        !pddby_db_bind_text(db_stmt, 2, comment->text))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    comment->id = pddby_db_last_insert_id(comment->pddby);

    return 1;

error:
    pddby_report(comment->pddby, pddby_message_type_error, "unable to save comment object");
    return 0;
}

pddby_comment_t* pddby_comment_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `number`, `text` FROM `comments` WHERE `rowid`=? LIMIT 1");
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
    char const* text = pddby_db_column_text(db_stmt, 1);

    return pddby_comment_new_with_id(pddby, id, number, text);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find comment object with id = %lld", id);
    return NULL;
}

pddby_comment_t* pddby_comment_find_by_number(pddby_t* pddby, int32_t number)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `text` FROM `comments` WHERE `number`=? LIMIT 1");
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
    char const* text = pddby_db_column_text(db_stmt, 1);

    return pddby_comment_new_with_id(pddby, id, number, text);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find comment object with number = %d", number);
    return NULL;
}
