#include "topic.h"
#include "config.h"
#include "database.h"
#include "question.h"

#include <stdlib.h>
#include <string.h>

static pddby_topic_t* pddby_topic_new_with_id(int64_t id, int number, char const* title)
{
    pddby_topic_t* topic = malloc(sizeof(pddby_topic_t));
    topic->id = id;
    topic->number = number;
    topic->title = strdup(title);
    return topic;
}

pddby_topic_t* pddby_topic_new(int number, char const* title)
{
    return pddby_topic_new_with_id(0, number, title);
}

void pddby_topic_free(pddby_topic_t* topic)
{
    free(topic->title);
    free(topic);
}

int pddby_topic_save(pddby_topic_t* topic)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `topics` (`number`, `title`) VALUES (?, ?)", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int(stmt, 1, topic->number);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, topic->title, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    topic->id = sqlite3_last_insert_rowid(db);

    return 1;
}

pddby_topic_t* pddby_topic_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `number`, `title` FROM `topics` WHERE `rowid`=? LIMIT 1", -1, &stmt,
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
        return 0;
    }

    int number = sqlite3_column_int(stmt, 0);
    char const* title = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_topic_new_with_id(id, number, title);
}

pddby_topic_t* pddby_topic_find_by_number(int number)
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `title` FROM `topics` WHERE `number`=? LIMIT 1", -1, &stmt,
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
    char const* title = (char const*)sqlite3_column_text(stmt, 1);

    return pddby_topic_new_with_id(id, number, title);
}

pddby_topics_t* pddby_topic_find_all()
{
    static sqlite3_stmt* stmt = 0;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `number`, `title` FROM `topics`", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    pddby_topics_t* topics = pddby_array_new(0);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        int number = sqlite3_column_int(stmt, 1);
        char const* title = (char const*)sqlite3_column_text(stmt, 2);

        pddby_array_add(topics, pddby_topic_new_with_id(id, number, title));
    }

    return topics;
}

void pddby_topic_free_all(pddby_topics_t* topics)
{
    //pddby_array_foreach(topics, (GFunc)topic_free, NULL);
    pddby_array_free(topics);
}

size_t pddby_topic_get_question_count(pddby_topic_t const* topic)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM `questions` WHERE `topic_id`=?", -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, topic->id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

    return sqlite3_column_int(stmt, 0);
}
