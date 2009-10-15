#include "section.h"
#include "common.h"
#include "database.h"
#include "question.h"

static inline pdd_sections_t *get_sections()
{
	static pdd_sections_t *sections = NULL;
	if (!sections)
	{
		sections = g_ptr_array_new();
	}
	return sections;
}

static pdd_section_t *section_new_with_id(gint64 id, const gchar *name, const gchar *title_prefix, const gchar *title)
{
	pdd_section_t *section = g_new(pdd_section_t, 1);
	section->id = id;
	section->name = g_strdup(name);
	section->title_prefix = g_strdup(title_prefix);
	section->title = g_strdup(title);
	return section;
}

static pdd_section_t *section_copy(const pdd_section_t *section)
{
	return section_new_with_id(section->id, section->name, section->title_prefix, section->title);
}

pdd_section_t *section_new(const gchar *name, const gchar *title_prefix, const gchar *title)
{
	return section_new_with_id(0, name, title_prefix, title);
}

void section_free(pdd_section_t *section)
{
	g_free(section->name);
	g_free(section->title_prefix);
	g_free(section->title);
	g_free(section);
}

gboolean section_save(pdd_section_t *section)
{
	if (!use_cache)
	{
		static gint64 id = 0;
		section->id = ++id;
		g_ptr_array_add(get_sections(), section_copy(section));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `sections` (`name`, `title_prefix`, `title`) VALUES (?, ?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("section: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 1, section->name, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 2, section->title_prefix, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 3, section->title, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("section: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	section->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

pdd_section_t *section_find_by_id(gint64 id)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_sections()->len; i++)
		{
			const pdd_section_t *section = g_ptr_array_index(get_sections(), i);
			if (section->id == id)
			{
				return section_copy(section);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `name`, `title_prefix`, `title` FROM `sections` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("section: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("section: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	const gchar *name = (const gchar *)sqlite3_column_text(stmt, 0);
	const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 1);
	const gchar *title = (const gchar *)sqlite3_column_text(stmt, 2);

	return section_new_with_id(id, name, title_prefix, title);
}

pdd_section_t *section_find_by_name(const gchar *name)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_sections()->len; i++)
		{
			const pdd_section_t *section = g_ptr_array_index(get_sections(), i);
			if (!g_strcmp0(section->name, name))
			{
				return section_copy(section);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `title_prefix`, `title` FROM `sections` WHERE `name`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("section: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_text(stmt, 1, name, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("section: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint64 id = sqlite3_column_int64(stmt, 0);
	const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 1);
	const gchar *title = (const gchar *)sqlite3_column_text(stmt, 2);

	return section_new_with_id(id, name, title_prefix, title);
}

pdd_sections_t *section_find_all()
{
	if (!use_cache)
	{
		return section_copy_all(get_sections());
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `name`, `title_prefix`, `title` FROM `sections`", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("section: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	pdd_sections_t *sections = g_ptr_array_new();

	while (TRUE)
	{
		result = sqlite3_step(stmt);
		if (result == SQLITE_DONE)
		{
			break;
		}
		if (result != SQLITE_ROW)
		{
			g_error("section: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}

		gint64 id = sqlite3_column_int64(stmt, 0);
		const gchar *name = (const gchar *)sqlite3_column_text(stmt, 1);
		const gchar *title_prefix = (const gchar *)sqlite3_column_text(stmt, 2);
		const gchar *title = (const gchar *)sqlite3_column_text(stmt, 3);
	
		g_ptr_array_add(sections, section_new_with_id(id, name, title_prefix, title));
	}

	return sections;
}

pdd_sections_t *section_copy_all(pdd_sections_t *sections)
{
	pdd_sections_t *sections_copy = g_ptr_array_new();
	gsize i;
	for (i = 0; i < sections->len; i++)
	{
		const pdd_section_t *section = g_ptr_array_index(sections, i);
		g_ptr_array_add(sections_copy, section_copy(section));
	}
	return sections_copy;
}

void section_free_all(pdd_sections_t *sections)
{
	g_ptr_array_foreach(sections, (GFunc)section_free, NULL);
	g_ptr_array_free(sections, TRUE);
}

void count_questions_by_section(G_GNUC_UNUSED pdd_question_t *question, pdd_sections_t *sections, id_pointer_t *id_ptr)
{
	gsize i;
	for (i = 0; i < sections->len; i++)
	{
		pdd_section_t *section = g_ptr_array_index(sections, i);
		if (section->id == id_ptr->id)
		{
			(*(gint32 *)id_ptr->ptr)++;
			break;
		}
	}
}

gint32 section_get_question_count(pdd_section_t *section)
{
	if (!use_cache)
	{
		gint32 count = 0;
		id_pointer_t id_ptr = {section->id, &count};
		g_hash_table_foreach(get_questions_sections(), (GHFunc)count_questions_by_section, &id_ptr);
		return count;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM `questions_sections` WHERE `section_id`=?", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("section: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, section->id);
	if (result != SQLITE_OK)
	{
		g_error("section: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		g_error("section: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	return sqlite3_column_int(stmt, 0);
}
