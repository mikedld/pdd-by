#include "answer.h"

#include "config.h"
#include "private/util/database.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_answer_t* pddby_answer_new_with_id(pddby_t* pddby, int64_t id, int64_t question_id, char const* text, int is_correct)
{
    pddby_answer_t *answer = calloc(1, sizeof(pddby_answer_t));
    if (!answer)
    {
        goto error;
    }

    answer->text = text ? strdup(text) : NULL;
    if (text && !answer->text)
    {
        goto error;
    }

    answer->id = id;
    answer->question_id = question_id;
    answer->is_correct = is_correct;
    answer->pddby = pddby;

    return answer;

error:
    // TODO: report error
    if (answer)
    {
        pddby_answer_free(answer);
    }
    return NULL;
}

pddby_answer_t* pddby_answer_new(pddby_t* pddby, int64_t question_id, char const* text, int is_correct)
{
    return pddby_answer_new_with_id(pddby, 0, question_id, text, is_correct);
}

void pddby_answer_free(pddby_answer_t* answer)
{
    assert(answer);

    if (answer->text)
    {
        free(answer->text);
    }
    free(answer);
}

int pddby_answer_save(pddby_answer_t* answer)
{
    assert(answer);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(answer->pddby, "INSERT INTO `answers` (`question_id`, `text`, `is_correct`) VALUES (?, ?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !(answer->question_id ?
            pddby_db_bind_int64(db_stmt, 1, answer->question_id) :
            pddby_db_bind_null(db_stmt, 1)) ||
        !pddby_db_bind_text(db_stmt, 2, answer->text) ||
        !pddby_db_bind_int(db_stmt, 3, answer->is_correct))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    answer->id = pddby_db_last_insert_id(answer->pddby);

    return 1;

error:
    // TODO: report error
    return 0;
}

pddby_answer_t* pddby_answer_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `question_id`, `text`, `is_correct` FROM `answers` WHERE `rowid`=? LIMIT 1");
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

    int64_t question_id = pddby_db_column_int64(db_stmt, 0);
    char const* text = pddby_db_column_text(db_stmt, 1);
    int is_correct = pddby_db_column_int(db_stmt, 2);

    return pddby_answer_new_with_id(pddby, id, question_id, text, is_correct);

error:
    // TODO: report error
    return NULL;
}

pddby_answers_t* pddby_answers_new(pddby_t* pddby)
{
    return pddby_array_new(pddby, (pddby_array_free_func_t)pddby_answer_free);
}

pddby_answers_t* pddby_answers_find_by_question(struct pddby* pddby, int64_t question_id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `text`, `is_correct` FROM `answers` WHERE `question_id`=?");
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

    pddby_answers_t* answers = pddby_answers_new(pddby);
    if (!answers)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        char const* text = pddby_db_column_text(db_stmt, 1);
        int is_correct = pddby_db_column_int(db_stmt, 2);

        if (!pddby_array_add(answers, pddby_answer_new_with_id(pddby, id, question_id, text, is_correct)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_answers_free(answers);
        goto error;
    }

    return answers;

error:
    // TODO: report error
    return NULL;
}

void pddby_answers_free(pddby_answers_t* answers)
{
    assert(answers);

    pddby_array_free(answers, 1);
}
