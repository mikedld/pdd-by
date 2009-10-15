#include "question.h"
#include "common.h"
#include "database.h"
#include "topic.h"

const gint ticket_topics_distribution[] = {1, 2, 2, 2, 1, 1, 1};

inline pdd_questions_t *get_questions()
{
	static pdd_questions_t *questions = NULL;
	if (!questions)
	{
		questions = g_ptr_array_new();
	}
	return questions;
}

inline GHashTable *get_questions_sections()
{
	static GHashTable *questions_sections = NULL;
	if (!questions_sections)
	{
		questions_sections = g_hash_table_new(NULL, NULL);
	}
	return questions_sections;
}

inline GHashTable *get_questions_traffregs()
{
	static GHashTable *questions_traffregs = NULL;
	if (!questions_traffregs)
	{
		questions_traffregs = g_hash_table_new(NULL, NULL);
	}
	return questions_traffregs;
}

static pdd_question_t *question_new_with_id(gint64 id, gint64 topic_id, const gchar *text, gint64 image_id, const gchar *advice, gint64 comment_id)
{
	pdd_question_t *question = g_new(pdd_question_t, 1);
	question->id = id;
	question->topic_id = topic_id;
	question->text = g_strdup(text);
	question->image_id = image_id;
	question->advice = g_strdup(advice);
	question->comment_id = comment_id;
	return question;
}

static pdd_question_t *question_copy(const pdd_question_t *question)
{
	return question_new_with_id(question->id, question->topic_id, question->text, question->image_id, question->advice, question->comment_id);
}

pdd_question_t *question_new(gint64 topic_id, const gchar *text, gint64 image_id, const gchar *advice, gint64 comment_id)
{
	return question_new_with_id(0, topic_id, text, image_id, advice, comment_id);
}

void question_free(pdd_question_t *question)
{
	g_free(question->text);
	g_free(question->advice);
	g_free(question);
}

gboolean question_save(pdd_question_t *question)
{
	if (!use_cache)
	{
		static gint64 id = 0;
		question->id = ++id;
		g_ptr_array_add(get_questions(), question_copy(question));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `questions` (`topic_id`, `text`, `image_id`, `advice`, `comment_id`) VALUES (?, ?, ?, ?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = question->topic_id ? sqlite3_bind_int64(stmt, 1, question->topic_id) : sqlite3_bind_null(stmt, 1);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 2, question->text, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = question->image_id ? sqlite3_bind_int64(stmt, 3, question->image_id) : sqlite3_bind_null(stmt, 3);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 4, question->advice, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = question->comment_id ? sqlite3_bind_int64(stmt, 5, question->comment_id) : sqlite3_bind_null(stmt, 5);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	question->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

gboolean question_set_sections(pdd_question_t *question, pdd_sections_t *sections)
{
	if (!use_cache)
	{
		g_hash_table_insert(get_questions_sections(), question_copy(question), section_copy_all(sections));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `questions_sections` (`question_id`, `section_id`) VALUES (?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	gsize i;
	for (i = 0; i < sections->len; i++)
	{
		pdd_section_t *section = ((pdd_section_t **)sections->pdata)[i];

		result = sqlite3_reset(stmt);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_bind_int64(stmt, 1, question->id);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_bind_int64(stmt, 2, section->id);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_step(stmt);
		if (result != SQLITE_DONE)
		{
			g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	return TRUE;
}

gboolean question_set_traffregs(pdd_question_t *question, pdd_traffregs_t *traffregs)
{
	if (!use_cache)
	{
		g_hash_table_insert(get_questions_traffregs(), question_copy(question), traffreg_copy_all(traffregs));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `questions_traffregs` (`question_id`, `traffreg_id`) VALUES (?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	gsize i;
	for (i = 0; i < traffregs->len; i++)
	{
		pdd_traffreg_t *traffreg = ((pdd_traffreg_t **)traffregs->pdata)[i];

		result = sqlite3_reset(stmt);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_bind_int64(stmt, 1, question->id);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_bind_int64(stmt, 2, traffreg->id);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		result = sqlite3_step(stmt);
		if (result != SQLITE_DONE)
		{
			g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	return TRUE;
}

pdd_question_t *question_find_by_id(gint64 id)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_questions()->len; i++)
		{
			const pdd_question_t *question = g_ptr_array_index(get_questions(), i);
			if (question->id == id)
			{
				return question_copy(question);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `topic_id`, `text`, `image_id`, `advice`, `comment_id` FROM `questions` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint64 topic_id = sqlite3_column_int64(stmt, 0);
	const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);
	gint64 image_id = sqlite3_column_int64(stmt, 2);
	const gchar *advice = (const gchar *)sqlite3_column_text(stmt, 3);
	gint64 comment_id = sqlite3_column_int64(stmt, 4);

	return question_new_with_id(id, topic_id, text, image_id, advice, comment_id);
}

static void find_questions_by_section(const pdd_question_t *question, const pdd_sections_t *sections, id_pointer_t *id_ptr)
{
	gsize i;
	for (i = 0; i < sections->len; i++)
	{
		const pdd_section_t *section = g_ptr_array_index(sections, i);
		if (section->id == id_ptr->id)
		{
			g_ptr_array_add(id_ptr->ptr, question_copy(question));
			break;
		}
	}
}

pdd_questions_t *question_find_by_section(gint64 section_id)
{
	if (!use_cache)
	{
		pdd_questions_t *questions = g_ptr_array_new();
		id_pointer_t id_ptr = {section_id, questions};
		g_hash_table_foreach(get_questions_sections(), (GHFunc)find_questions_by_section, &id_ptr);
		return questions;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT q.`rowid`, q.`topic_id`, q.`text`, q.`image_id`, q.`advice`, q.`comment_id` FROM `questions` q INNER JOIN `questions_sections` qs ON q.`rowid`=qs.`question_id` WHERE qs.`section_id`=?", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, section_id);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	pdd_questions_t *questions = g_ptr_array_new();

	while (TRUE)
	{
		result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			break;
		}
		if (result != SQLITE_ROW)
		{
			g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		gint64 id = sqlite3_column_int64(stmt, 0);
		gint64 topic_id = sqlite3_column_int64(stmt, 1);
		const gchar *text = (const gchar *)sqlite3_column_text(stmt, 2);
		gint64 image_id = sqlite3_column_int64(stmt, 3);
		const gchar *advice = (const gchar *)sqlite3_column_text(stmt, 4);
		gint64 comment_id = sqlite3_column_int64(stmt, 5);
	
		g_ptr_array_add(questions, question_new_with_id(id, topic_id, text, image_id, advice, comment_id));
	}

	return questions;
}

pdd_questions_t *question_find_with_offset(gint64 topic_id, gint offset, gint count)
{
	if (!use_cache)
	{
		pdd_questions_t *questions = g_ptr_array_new();
		gsize i;
		gint offs = 0;
		for (i = 0; i < get_questions()->len; i++)
		{
			const pdd_question_t *question = g_ptr_array_index(get_questions(), i);
			if (question->topic_id == topic_id)
			{
				if (++offs < offset)
				{
					continue;
				}
				g_ptr_array_add(questions, question_copy(question));
				if ((gint)questions->len == count)
				{
					break;
				}
			}
		}
		return questions;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text`, `image_id`, `advice`, `comment_id` FROM `questions` WHERE `topic_id`=? LIMIT ?,?", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("question: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, topic_id);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 2, offset);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 3, count);
	if (result != SQLITE_OK)
	{
		g_error("question: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	pdd_questions_t *questions = g_ptr_array_new();

	while (TRUE)
	{
		result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			break;
		}
		if (result != SQLITE_ROW)
		{
			g_error("question: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		gint64 id = sqlite3_column_int64(stmt, 0);
		const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);
		gint64 image_id = sqlite3_column_int64(stmt, 2);
		const gchar *advice = (const gchar *)sqlite3_column_text(stmt, 3);
		gint64 comment_id = sqlite3_column_int64(stmt, 4);
	
		g_ptr_array_add(questions, question_new_with_id(id, topic_id, text, image_id, advice, comment_id));
	}

	return questions;
}

pdd_questions_t *question_find_by_topic(gint64 topic_id, gint ticket_number)
{
	if (ticket_number > 0)
	{
		return question_find_with_offset(topic_id, (ticket_number - 1) * 10, 10);
	}
	return question_find_with_offset(topic_id, 0, -1);
}

pdd_questions_t *question_find_by_ticket(gint ticket_number)
{
	const pdd_topics_t *topics = topic_find_all();
	gsize i;
	gint j;
	pdd_questions_t *questions = g_ptr_array_new();
	for (i = 0; i < topics->len; i++)
	{
		const pdd_topic_t *topic = g_ptr_array_index(topics, i);
		gint32 count = topic_get_question_count(topic);
		for (j = 0; j < ticket_topics_distribution[i]; j++)
		{
			pdd_questions_t *topic_questions = question_find_with_offset(topic->id, ((ticket_number - 1) * 10 + j) % count, 1);
			g_ptr_array_add(questions, g_ptr_array_index(topic_questions, 0));
			g_ptr_array_free(topic_questions, TRUE);
		}
	}
	return questions;
}

pdd_questions_t *question_find_random()
{
	const pdd_topics_t *topics = topic_find_all();
	gsize i;
	gint j;
	pdd_questions_t *questions = g_ptr_array_new();
	for (i = 0; i < topics->len; i++)
	{
		const pdd_topic_t *topic = g_ptr_array_index(topics, i);
		gint32 count = topic_get_question_count(topic);
		for (j = 0; j < ticket_topics_distribution[i]; j++)
		{
			pdd_questions_t *topic_questions = question_find_with_offset(topic->id, g_random_int_range(0, count - 1), 1);
			g_ptr_array_add(questions, g_ptr_array_index(topic_questions, 0));
			g_ptr_array_free(topic_questions, TRUE);
		}
	}
	return questions;
}

void question_free_all(pdd_questions_t *questions)
{
	g_ptr_array_foreach(questions, (GFunc)question_free, NULL);
	g_ptr_array_free(questions, TRUE);
}
