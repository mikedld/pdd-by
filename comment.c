#include "comment.h"
#include "common.h"
#include "database.h"

static inline pdd_comments_t *get_comments()
{
	static pdd_comments_t *comments = NULL;
	if (!comments)
	{
		comments = g_ptr_array_new();
	}
	return comments;
}

static pdd_comment_t *comment_new_with_id(gint64 id, gint32 number, const gchar *text)
{
	pdd_comment_t *comment = g_new(pdd_comment_t, 1);
	comment->id = id;
	comment->number = number;
	comment->text = g_strdup(text);
	return comment;
}

static pdd_comment_t *comment_copy(pdd_comment_t *comment)
{
	return comment_new_with_id(comment->id, comment->number, comment->text);
}

pdd_comment_t *comment_new(gint32 number, const gchar *text)
{
	return comment_new_with_id(0, number, text);
}

void comment_free(pdd_comment_t *comment)
{
	g_free(comment->text);
	g_free(comment);
}

gboolean comment_save(pdd_comment_t *comment)
{
	if (!use_cache)
	{
		static gint64 id = 0;
		comment->id = ++id;
		g_ptr_array_add(get_comments(), comment_copy(comment));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `comments` (`number`, `text`) VALUES (?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("comment: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 1, comment->number);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 2, comment->text, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("comment: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	comment->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

pdd_comment_t *comment_find_by_id(gint64 id)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_comments()->len; i++)
		{
			pdd_comment_t *comment = g_ptr_array_index(get_comments(), i);
			if (comment->id == id)
			{
				return comment_copy(comment);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `number`, `text` FROM `comments` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("comment: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("comment: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint32 number = sqlite3_column_int(stmt, 0);
	const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);

	return comment_new_with_id(id, number, text);
}

pdd_comment_t *comment_find_by_number(gint32 number)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_comments()->len; i++)
		{
			pdd_comment_t *comment = g_ptr_array_index(get_comments(), i);
			if (comment->number == number)
			{
				return comment_copy(comment);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `text` FROM `comments` WHERE `number`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("comment: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int(stmt, 1, number);
	if (result != SQLITE_OK)
	{
		g_error("comment: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("comment: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint32 id = sqlite3_column_int64(stmt, 0);
	const gchar *text = (const gchar *)sqlite3_column_text(stmt, 1);

	return comment_new_with_id(id, number, text);
}
