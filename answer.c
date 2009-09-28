#include "answer.h"
#include "database.h"

pdd_answer_t *answer_new_with_id(gint64 id, gint64 question_id, const gchar *text, gboolean is_correct)
{
	pdd_answer_t *answer = g_new(pdd_answer_t, 1);
	answer->id = id;
	answer->question_id = question_id;
	answer->text = g_strdup(text);
	answer->is_correct = is_correct;
	return answer;
}

pdd_answer_t *answer_new(gint64 question_id, const gchar *text, gboolean is_correct)
{
	return answer_new_with_id(0, question_id, text, is_correct);
}

void answer_free(pdd_answer_t *answer)
{
	g_free(answer->text);
	g_free(answer);
}

gboolean answer_save(pdd_answer_t *answer)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `answers` (`question_id`, `text`, `is_correct`) VALUES (?, ?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("answer: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = answer->question_id ? sqlite3_bind_int64(stmt, 1, answer->question_id) : sqlite3_bind_null(stmt, 1);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 2, answer->text, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 3, answer->is_correct);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("answer: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	answer->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

pdd_answer_t *answer_find_by_id(gint64 id)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `question_id`, `text`, `is_correct` FROM `answers` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("answer: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("answer: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint64 question_id = sqlite3_column_int64(stmt, 0);
	const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);
	gboolean is_correct = sqlite3_column_int(stmt, 2);

	return answer_new_with_id(id, question_id, text, is_correct);
}

pdd_answers_t *answer_find_by_question(gint64 question_id)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text`, `is_correct` FROM `answers` WHERE `question_id`=?", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("answer: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, question_id);
	if (result != SQLITE_OK)
	{
		g_error("answer: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	pdd_answers_t *answers = g_ptr_array_new();

	while (TRUE)
	{
		result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			break;
		}
		if (result != SQLITE_ROW)
		{
			g_error("answer: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		gint64 id = sqlite3_column_int64(stmt, 0);
		const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);
		gboolean is_correct = sqlite3_column_int(stmt, 2);
	
		g_ptr_array_add(answers, answer_new_with_id(id, question_id, text, is_correct));
	}

	return answers;
}

void answer_free_all(pdd_answers_t *answers)
{
	g_ptr_array_foreach(answers, (GFunc)answer_free, NULL);
	g_ptr_array_free(answers, TRUE);
}
