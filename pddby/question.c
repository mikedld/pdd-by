#include "question.h"

#include "config.h"
#include "private/util/aux.h"
#include "private/util/database.h"
#include "private/util/report.h"
#include "private/util/settings.h"
#include "private/util/string.h"
#include "topic.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static int* ticket_topics_distribution = NULL;

static pddby_question_t* pddby_question_new_with_id(pddby_t* pddby, int64_t id, int64_t topic_id, char const* text, int64_t image_id,
    char const* advice, int64_t comment_id)
{
    pddby_question_t* question = calloc(1, sizeof(pddby_question_t));
    if (!question)
    {
        goto error;
    }

    question->text = text ? strdup(text) : NULL;
    if (text && !question->text)
    {
        goto error;
    }

    question->advice = advice ? strdup(advice) : NULL;
    if (advice && !question->advice)
    {
        goto error;
    }

    question->id = id;
    question->topic_id = topic_id;
    question->image_id = image_id;
    question->comment_id = comment_id;
    question->pddby = pddby;

    return question;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create question object");
    if (question)
    {
        pddby_question_free(question);
    }
    return NULL;
}

pddby_question_t* pddby_question_new(pddby_t* pddby, int64_t topic_id, char const* text, int64_t image_id, char const* advice,
    int64_t comment_id)
{
    return pddby_question_new_with_id(pddby, 0, topic_id, text, image_id, advice, comment_id);
}

void pddby_question_free(pddby_question_t *question)
{
    assert(question);

    if (question->text)
    {
        free(question->text);
    }
    if (question->advice)
    {
        free(question->advice);
    }
    free(question);
}

int pddby_question_save(pddby_question_t* question)
{
    assert(question);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(question->pddby, "INSERT INTO `questions` (`topic_id`, `text`, `image_id`, `advice`, `comment_id`) "
            "VALUES (?, ?, ?, ?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !(question->topic_id ?
            pddby_db_bind_int64(db_stmt, 1, question->topic_id) :
            pddby_db_bind_null(db_stmt, 1)) ||
        !pddby_db_bind_text(db_stmt, 2, question->text) ||
        !(question->image_id ?
            pddby_db_bind_int64(db_stmt, 3, question->image_id) :
            pddby_db_bind_null(db_stmt, 3)) ||
        !pddby_db_bind_text(db_stmt, 4, question->advice) ||
        !(question->comment_id ?
            pddby_db_bind_int64(db_stmt, 5, question->comment_id) :
            pddby_db_bind_null(db_stmt, 5)))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    question->id = pddby_db_last_insert_id(question->pddby);

    return 1;

error:
    pddby_report(question->pddby, pddby_message_type_error, "unable to save question object");
    return 0;
}

int pddby_question_set_sections(pddby_question_t* question, pddby_sections_t* sections)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(question->pddby, "INSERT INTO `questions_sections` (`question_id`, `section_id`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    for (size_t i = 0, size = pddby_array_size(sections); i < size; i++)
    {
        pddby_section_t* section = pddby_array_index(sections, i);

        if (!pddby_db_reset(db_stmt) ||
            !pddby_db_bind_int64(db_stmt, 1, question->id) ||
            !pddby_db_bind_int64(db_stmt, 2, section->id))
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
    pddby_report(question->pddby, pddby_message_type_error, "unable to set question object sections");
    return 0;
}

int pddby_question_set_traffregs(pddby_question_t* question, pddby_traffregs_t* traffregs)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(question->pddby, "INSERT INTO `questions_traffregs` (`question_id`, `traffreg_id`) VALUES (?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    for (size_t i = 0, size = pddby_array_size(traffregs); i < size; i++)
    {
        pddby_traffreg_t* traffreg = pddby_array_index(traffregs, i);

        if (!pddby_db_reset(db_stmt) ||
            !pddby_db_bind_int64(db_stmt, 1, question->id) ||
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
    pddby_report(question->pddby, pddby_message_type_error, "unable to set question object traffregs");
    return 0;
}

pddby_question_t* pddby_question_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `topic_id`, `text`, `image_id`, `advice`, `comment_id` FROM "
            "`questions` WHERE `rowid`=? LIMIT 1");
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

    int64_t topic_id = pddby_db_column_int64(db_stmt, 0);
    char const* text = pddby_db_column_text(db_stmt, 1);
    int64_t image_id = pddby_db_column_int64(db_stmt, 2);
    char const* advice = pddby_db_column_text(db_stmt, 3);
    int64_t comment_id = pddby_db_column_int64(db_stmt, 4);

    return pddby_question_new_with_id(pddby, id, topic_id, text, image_id, advice, comment_id);

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find question object with id = %lld", id);
    return NULL;
}

pddby_questions_t* pddby_questions_new(pddby_t* pddby)
{
    return pddby_array_new(pddby, (pddby_array_free_func_t)pddby_question_free);
}

pddby_questions_t* pddby_questions_find_by_section(pddby_t* pddby, int64_t section_id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT q.`rowid`, q.`topic_id`, q.`text`, q.`image_id`, q.`advice`, "
            "q.`comment_id` FROM `questions` q INNER JOIN `questions_sections` qs ON q.`rowid`=qs.`question_id` WHERE "
            "qs.`section_id`=?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, section_id))
    {
        goto error;
    }

    pddby_questions_t* questions = pddby_questions_new(pddby);
    if (!questions)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        int64_t topic_id = pddby_db_column_int64(db_stmt, 1);
        char const* text = pddby_db_column_text(db_stmt, 2);
        int64_t image_id = pddby_db_column_int64(db_stmt, 3);
        char const* advice = pddby_db_column_text(db_stmt, 4);
        int64_t comment_id = pddby_db_column_int64(db_stmt, 5);

        if (!pddby_array_add(questions, pddby_question_new_with_id(pddby, id, topic_id, text, image_id, advice, comment_id)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_questions_free(questions);
        goto error;
    }

    return questions;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find question objects with section id = %lld", section_id);
    return NULL;
}

static pddby_questions_t* pddby_questions_find_with_offset(pddby_t* pddby, int64_t topic_id, int offset, int count)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `text`, `image_id`, `advice`, `comment_id` FROM `questions` "
            "WHERE `topic_id`=? LIMIT ?,?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, topic_id) ||
        !pddby_db_bind_int(db_stmt, 2, offset) ||
        !pddby_db_bind_int(db_stmt, 3, count))
    {
        goto error;
    }

    pddby_questions_t* questions = pddby_questions_new(pddby);
    if (!questions)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        char const* text = pddby_db_column_text(db_stmt, 1);
        int64_t image_id = pddby_db_column_int64(db_stmt, 2);
        char const* advice = pddby_db_column_text(db_stmt, 3);
        int64_t comment_id = pddby_db_column_int64(db_stmt, 4);

        if (!pddby_array_add(questions, pddby_question_new_with_id(pddby, id, topic_id, text, image_id, advice, comment_id)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_questions_free(questions);
        goto error;
    }

    return questions;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to find %d question object(s) with topic id = %lld and "
        "offset = %d", count, topic_id, offset);
    return NULL;
}

pddby_questions_t* pddby_questions_find_by_topic(pddby_t* pddby, int64_t topic_id, int ticket_number)
{
    if (ticket_number > 0)
    {
        return pddby_questions_find_with_offset(pddby, topic_id, (ticket_number - 1) * 10, 10);
    }
    return pddby_questions_find_with_offset(pddby, topic_id, 0, -1);
}

static void pddby_load_ticket_topics_distribution(pddby_t* pddby)
{
    if (ticket_topics_distribution)
    {
        return;
    }

    char *raw_ttd = pddby_settings_get(pddby, "ticket_topics_distribution");
    char **ttd = pddby_string_split(pddby, raw_ttd, ":");
    free(raw_ttd);

    ticket_topics_distribution = malloc(pddby_stringv_length(ttd) * sizeof(int));

    char **it = ttd;
    size_t i = 0;
    while (*it)
    {
        ticket_topics_distribution[i] = atoi(*it);
        it++;
        i++;
    }

    pddby_stringv_free(ttd);
}

pddby_questions_t* pddby_questions_find_by_ticket(pddby_t* pddby, int ticket_number)
{
    pddby_load_ticket_topics_distribution(pddby);

    pddby_topics_t* topics = pddby_topics_find_all(pddby);
    pddby_questions_t* questions = pddby_questions_new(pddby);
    for (size_t i = 0, size = pddby_array_size(topics); i < size; i++)
    {
        pddby_topic_t const* topic = pddby_array_index(topics, i);
        int32_t count = pddby_topic_get_question_count(topic);
        for (int j = 0; j < ticket_topics_distribution[i]; j++)
        {
            pddby_questions_t* topic_questions = pddby_questions_find_with_offset(pddby, topic->id,
                ((ticket_number - 1) * 10 + j) % count, 1);
            pddby_array_add(questions, pddby_array_index(topic_questions, 0));
            pddby_array_free(topic_questions, 0);
        }
    }
    pddby_topics_free(topics);
    return questions;
}

pddby_questions_t* pddby_questions_find_random(pddby_t* pddby)
{
    pddby_load_ticket_topics_distribution(pddby);

    pddby_topics_t* topics = pddby_topics_find_all(pddby);
    pddby_questions_t* questions = pddby_questions_new(pddby);
    for (size_t i = 0, size = pddby_array_size(topics); i < size; i++)
    {
        pddby_topic_t const* topic = pddby_array_index(topics, i);
        int32_t count = pddby_topic_get_question_count(topic);
        for (int j = 0; j < ticket_topics_distribution[i]; j++)
        {
            pddby_questions_t* topic_questions = pddby_questions_find_with_offset(pddby, topic->id,
                pddby_aux_random_int_range(0, count - 1), 1);
            pddby_array_add(questions, pddby_array_index(topic_questions, 0));
            pddby_array_free(topic_questions, 0);
        }
    }
    pddby_topics_free(topics);
    return questions;
}

void pddby_questions_free(pddby_questions_t* questions)
{
    assert(questions);

    pddby_array_free(questions, 1);
}
