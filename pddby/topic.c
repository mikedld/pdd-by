#include "topic.h"

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

static pddby_topic_t* pddby_topic_new_with_id(pddby_t* pddby, int64_t id, int number, char const* title)
{
    pddby_topic_t* topic = calloc(1, sizeof(pddby_topic_t));
    if (!topic)
    {
        goto error;
    }

    topic->title = title ? strdup(title) : NULL;
    if (title && !topic->title)
    {
        goto error;
    }

    topic->id = id;
    topic->number = number;
    topic->pddby = pddby;

    return topic;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create topic object");
    return NULL;
}

pddby_topic_t* pddby_topic_new(pddby_t* pddby, int number, char const* title)
{
    return pddby_topic_new_with_id(pddby, 0, number, title);
}

void pddby_topic_free(pddby_topic_t* topic)
{
    assert(topic);

    if (topic->title)
    {
        free(topic->title);
    }
    free(topic);
}

int pddby_topic_save(pddby_topic_t* topic)
{
    assert(topic);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(topic->pddby, "INSERT INTO `topics` (`number`, `title`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int(db_stmt, 1, topic->number) ||
        !pddby_db_bind_text(db_stmt, 2, topic->title))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    topic->id = pddby_db_last_insert_id(topic->pddby);

    return 1;

error:
    pddby_report(topic->pddby, pddby_message_type_error, "unable to save topic object");
    return 0;
}

pddby_topic_t* pddby_topic_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `number`, `title` FROM `topics` WHERE `rowid`=? LIMIT 1");
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

    int number = pddby_db_column_int(db_stmt, 0);
    char const* title = pddby_db_column_text(db_stmt, 1);

    return pddby_topic_new_with_id(pddby, id, number, title);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find topic object with id = %lld", id);
    return NULL;
}

pddby_topic_t* pddby_topic_find_by_number(pddby_t* pddby, int number)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `title` FROM `topics` WHERE `number`=? LIMIT 1");
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
    char const* title = pddby_db_column_text(db_stmt, 1);

    return pddby_topic_new_with_id(pddby, id, number, title);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find topic object with number = %d", number);
    return NULL;
}

pddby_topics_t* pddby_topics_new(pddby_t* pddby)
{
    return pddby_array_new(pddby, (pddby_array_free_func_t)pddby_topic_free);
}

pddby_topics_t* pddby_topics_find_all(pddby_t* pddby)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `number`, `title` FROM `topics`");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt))
    {
        goto error;
    }

    pddby_topics_t* topics = pddby_topics_new(pddby);
    if (!topics)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        int number = pddby_db_column_int(db_stmt, 1);
        char const* title = pddby_db_column_text(db_stmt, 2);

        if (!pddby_array_add(topics, pddby_topic_new_with_id(pddby, id, number, title)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_topics_free(topics);
        goto error;
    }

    return topics;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find all topic objects");
    return NULL;
}

void pddby_topics_free(pddby_topics_t* topics)
{
    assert(topics);

    pddby_array_free(topics, 1);
}

size_t pddby_topic_get_question_count(pddby_topic_t const* topic)
{
    assert(topic);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(topic->pddby, "SELECT COUNT(*) FROM `questions` WHERE `topic_id`=?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, topic->id))
    {
        goto error;
    }

    if (pddby_db_step(db_stmt) != 1)
    {
        goto error;
    }

    return pddby_db_column_int(db_stmt, 0);

error:
    pddby_report(topic->pddby, pddby_message_type_error, "unable to get questions count of topic object");
    return 0;
}
