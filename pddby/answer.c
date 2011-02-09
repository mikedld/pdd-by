#include "answer.h"
#include "config.h"
#include "database.h"

#include <stdlib.h>
#include <string.h>

static pddby_answer_t* pddby_answer_new_with_id(int64_t id, int64_t question_id, char const* text, int is_correct)
{
    pddby_answer_t *answer = malloc(sizeof(pddby_answer_t));
    answer->id = id;
    answer->question_id = question_id;
    answer->text = strdup(text);
    answer->is_correct = is_correct;
    return answer;
}

pddby_answer_t* pddby_answer_new(int64_t question_id, char const* text, int is_correct)
{
    return pddby_answer_new_with_id(0, question_id, text, is_correct);
}

void pddby_answer_free(pddby_answer_t* answer)
{
    free(answer->text);
    free(answer);
}

int pddby_answer_save(pddby_answer_t* answer)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "INSERT INTO `answers` (`question_id`, `text`, `is_correct`) VALUES (?, ?, ?)",
            -1, &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = answer->question_id ? sqlite3_bind_int64(stmt, 1, answer->question_id) : sqlite3_bind_null(stmt, 1);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_text(stmt, 2, answer->text, -1, NULL);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_bind_int(stmt, 3, answer->is_correct);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    result = sqlite3_step(stmt);
    pddby_database_expect(result, SQLITE_DONE, __FUNCTION__, "unable to perform statement");

    answer->id = sqlite3_last_insert_rowid(db);

    return 1;
}

pddby_answer_t* pddby_answer_find_by_id(int64_t id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `question_id`, `text`, `is_correct` FROM `answers` WHERE `rowid`=? "
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

    int64_t question_id = sqlite3_column_int64(stmt, 0);
    char const* text = (char const*)sqlite3_column_text(stmt, 1);
    int is_correct = sqlite3_column_int(stmt, 2);

    return pddby_answer_new_with_id(id, question_id, text, is_correct);
}

pddby_answers_t* pddby_answer_find_by_question(int64_t question_id)
{
    static sqlite3_stmt* stmt = NULL;
    sqlite3* db = pddby_database_get();
    int result;

    if (!stmt)
    {
        result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text`, `is_correct` FROM `answers` WHERE `question_id`=?", -1,
            &stmt, NULL);
        pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to prepare statement");
    }

    result = sqlite3_reset(stmt);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to reset prepared statement");

    result = sqlite3_bind_int64(stmt, 1, question_id);
    pddby_database_expect(result, SQLITE_OK, __FUNCTION__, "unable to bind param");

    pddby_answers_t* answers = pddby_array_new(NULL);

    for (;;)
    {
        result = sqlite3_step(stmt);
        if (result == SQLITE_DONE)
        {
            break;
        }
        pddby_database_expect(result, SQLITE_ROW, __FUNCTION__, "unable to perform statement");

        int64_t id = sqlite3_column_int64(stmt, 0);
        char const* text = (char const*)sqlite3_column_text(stmt, 1);
        int is_correct = sqlite3_column_int(stmt, 2);

        pddby_array_add(answers, pddby_answer_new_with_id(id, question_id, text, is_correct));
    }

    return answers;
}

void pddby_answer_free_all(pddby_answers_t* answers)
{
    pddby_array_foreach(answers, (pddby_array_foreach_func_t)pddby_answer_free, NULL);
    pddby_array_free(answers);
}
