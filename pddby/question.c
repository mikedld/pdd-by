#include "question.h"
#include "config.h"
#include "database.h"
#include "topic.h"
#include "util/aux.h"
#include "util/settings.h"
#include "util/string.h"

#include <stdlib.h>
#include <string.h>

static int* ticket_topics_distribution = 0;

static pddby_question_t* pddby_question_new_with_id(int64_t id, int64_t topic_id, char const* text, int64_t image_id,
    char const* advice, int64_t comment_id)
{
    pddby_question_t* question = malloc(sizeof(pddby_question_t));
    question->id = id;
    question->topic_id = topic_id;
    question->text = strdup(text);
    question->image_id = image_id;
    question->advice = strdup(advice);
    question->comment_id = comment_id;
    return question;
}

pddby_question_t* pddby_question_new(int64_t topic_id, char const* text, int64_t image_id, char const* advice,
    int64_t comment_id)
{
    return pddby_question_new_with_id(0, topic_id, text, image_id, advice, comment_id);
}

void pddby_question_free(pddby_question_t *question)
{
    free(question->text);
    free(question->advice);
    free(question);
}

int pddby_question_save(pddby_question_t* question)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `questions` (`topic_id`, `text`, `image_id`, `advice`, "
            "`comment_id`) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = question->topic_id ? sqlite3_bind_int64(stmt, 1, question->topic_id) : sqlite3_bind_null(stmt, 1);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, question->text, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = question->image_id ? sqlite3_bind_int64(stmt, 3, question->image_id) : sqlite3_bind_null(stmt, 3);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 4, question->advice, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = question->comment_id ? sqlite3_bind_int64(stmt, 5, question->comment_id) : sqlite3_bind_null(stmt, 5);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    question->id = sqlite3_last_insert_rowid(db);

    return 1;
}

int pddby_question_set_sections(pddby_question_t* question, pddby_sections_t* sections)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `questions_sections` (`question_id`, `section_id`) VALUES (?, ?)",
            -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    for (size_t i = 0; i < pddby_array_size(sections); i++)
    {
        pddby_section_t* section = pddby_array_index(sections, i);

        result = sqlite3_reset(stmt);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

        result = sqlite3_bind_int64(stmt, 1, question->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_bind_int64(stmt, 2, section->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_step(stmt);
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
    }

    return 1;
}

int pddby_question_set_traffregs(pddby_question_t* question, pddby_traffregs_t* traffregs)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `questions_traffregs` (`question_id`, `traffreg_id`) VALUES "
            "(?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    for (size_t i = 0; i < pddby_array_size(traffregs); i++)
    {
        pddby_traffreg_t* traffreg = pddby_array_index(traffregs, i);

        result = sqlite3_reset(stmt);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

        result = sqlite3_bind_int64(stmt, 1, question->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_bind_int64(stmt, 2, traffreg->id);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

        result = sqlite3_step(stmt);
        pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");
    }

    return 1;
}

pddby_question_t* pddby_question_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `topic_id`, `text`, `image_id`, `advice`, `comment_id` FROM "
            "`questions` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
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

    int64_t topic_id = sqlite3_column_int64(stmt, 0);
    char const* text = (char const*)sqlite3_column_text(stmt, 1);
    int64_t image_id = sqlite3_column_int64(stmt, 2);
    char const* advice = (char const*)sqlite3_column_text(stmt, 3);
    int64_t comment_id = sqlite3_column_int64(stmt, 4);

    return pddby_question_new_with_id(id, topic_id, text, image_id, advice, comment_id);
}

pddby_questions_t* pddby_question_find_by_section(int64_t section_id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT q.`rowid`, q.`topic_id`, q.`text`, q.`image_id`, q.`advice`, "
            "q.`comment_id` FROM `questions` q INNER JOIN `questions_sections` qs ON q.`rowid`=qs.`question_id` WHERE "
            "qs.`section_id`=?", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, section_id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    pddby_questions_t* questions = pddby_array_new(0);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        int64_t topic_id = sqlite3_column_int64(stmt, 1);
        char const* text = (char const*)sqlite3_column_text(stmt, 2);
        int64_t image_id = sqlite3_column_int64(stmt, 3);
        char const* advice = (char const*)sqlite3_column_text(stmt, 4);
        int64_t comment_id = sqlite3_column_int64(stmt, 5);

        pddby_array_add(questions, pddby_question_new_with_id(id, topic_id, text, image_id, advice, comment_id));
    }

    return questions;
}

static pddby_questions_t* pddby_question_find_with_offset(int64_t topic_id, int offset, int count)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text`, `image_id`, `advice`, `comment_id` FROM `questions` "
            "WHERE `topic_id`=? LIMIT ?,?", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, topic_id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_int(stmt, 2, offset);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_int(stmt, 3, count);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    pddby_questions_t* questions = pddby_array_new(0);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        char const* text = (char const*)sqlite3_column_text(stmt, 1);
        int64_t image_id = sqlite3_column_int64(stmt, 2);
        char const* advice = (char const*)sqlite3_column_text(stmt, 3);
        int64_t comment_id = sqlite3_column_int64(stmt, 4);

        pddby_array_add(questions, pddby_question_new_with_id(id, topic_id, text, image_id, advice, comment_id));
    }

    return questions;
}

pddby_questions_t* pddby_question_find_by_topic(int64_t topic_id, int ticket_number)
{
    if (ticket_number > 0)
    {
        return pddby_question_find_with_offset(topic_id, (ticket_number - 1) * 10, 10);
    }
    return pddby_question_find_with_offset(topic_id, 0, -1);
}

static void pddby_load_ticket_topics_distribution()
{
    if (ticket_topics_distribution)
    {
        return;
    }

    char *raw_ttd = pddby_settings_get("ticket_topics_distribution");
    char **ttd = pddby_string_split(raw_ttd, ":");
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

pddby_questions_t* pddby_question_find_by_ticket(int ticket_number)
{
    pddby_load_ticket_topics_distribution();

    pddby_topics_t const* topics = pddby_topic_find_all();
    int j;
    pddby_questions_t* questions = pddby_array_new(0);
    for (size_t i = 0; i < pddby_array_size(topics); i++)
    {
        pddby_topic_t const* topic = pddby_array_index(topics, i);
        int32_t count = pddby_topic_get_question_count(topic);
        for (j = 0; j < ticket_topics_distribution[i]; j++)
        {
            pddby_questions_t* topic_questions = pddby_question_find_with_offset(topic->id,
                ((ticket_number - 1) * 10 + j) % count, 1);
            pddby_array_add(questions, pddby_array_index(topic_questions, 0));
            pddby_array_free(topic_questions);
        }
    }
    return questions;
}

pddby_questions_t* pddby_question_find_random()
{
    pddby_load_ticket_topics_distribution();

    pddby_topics_t const* topics = pddby_topic_find_all();
    int j;
    pddby_questions_t* questions = pddby_array_new(0);
    for (size_t i = 0; i < pddby_array_size(topics); i++)
    {
        pddby_topic_t const* topic = pddby_array_index(topics, i);
        int32_t count = pddby_topic_get_question_count(topic);
        for (j = 0; j < ticket_topics_distribution[i]; j++)
        {
            pddby_questions_t* topic_questions = pddby_question_find_with_offset(topic->id, pddby_aux_random_int_range(0, count - 1),
                1);
            pddby_array_add(questions, pddby_array_index(topic_questions, 0));
            pddby_array_free(topic_questions);
        }
    }
    return questions;
}

void pddby_question_free_all(pddby_questions_t* questions)
{
    //pddby_array_foreach(questions, (GFunc)pddby_question_free, NULL);
    pddby_array_free(questions);
}
