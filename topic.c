#include "topic.h"
#include "database.h"

#include <glib.h>

pdd_topic_t *topic_new_with_id(gint64 id, gint number, const gchar *title)
{
	pdd_topic_t *topic = g_new(pdd_topic_t, 1);
	topic->id = id;
	topic->number = number;
	topic->title = g_strdup(title);
	return topic;
}

pdd_topic_t *topic_new(gint number, const gchar *title)
{
	return topic_new_with_id(0, number, title);
}

void topic_free(pdd_topic_t *topic)
{
	g_free(topic->title);
	g_free(topic);
}

gboolean topic_save(pdd_topic_t *topic)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `topics` (`number`, `title`) VALUES (?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("topic: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 1, topic->number);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 2, topic->title, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("topic: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	topic->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

pdd_topic_t *topic_find_by_id(gint64 id)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `number`, `title` FROM `topics` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("topic: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("topic: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint number = sqlite3_column_int(stmt, 0);
	const gchar *title = (const gchar *)sqlite3_column_text(stmt, 1);

	return topic_new_with_id(id, number, title);
}

pdd_topic_t *topic_find_by_number(gint number)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `title` FROM `topics` WHERE `number`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("topic: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 1, number);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("topic: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}
	gint64 id = sqlite3_column_int64(stmt, 0);
	const gchar *title = (const gchar *)sqlite3_column_text(stmt, 1);

	return topic_new_with_id(id, number, title);
}

pdd_topics_t *topic_find_all()
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `number`, `title` FROM `topics`", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("topic: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	pdd_topics_t *topics = g_ptr_array_new();

	while (TRUE)
	{
		result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			break;
		}
		if (result != SQLITE_ROW)
		{
			g_error("topic: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
			topic_free_all(topics);
			return NULL;
		}

		gint64 id = sqlite3_column_int64(stmt, 0);
		gint number = sqlite3_column_int(stmt, 1);
		const gchar *title = (const gchar *)sqlite3_column_text(stmt, 2);
	
		g_ptr_array_add(topics, topic_new_with_id(id, number, title));
	}

	return topics;
}

void topic_free_all(pdd_topics_t *topics)
{
	g_ptr_array_foreach(topics, (GFunc)topic_free, NULL);
	g_ptr_array_free(topics, TRUE);
}

gint32 topic_get_question_count(pdd_topic_t *topic)
{
	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM `questions` WHERE `topic_id`=?", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("topic: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, topic->id);
	if (result != SQLITE_OK)
	{
		g_error("topic: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		g_error("topic: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	return sqlite3_column_int(stmt, 0);
}
