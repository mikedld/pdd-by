#include "image.h"
#include "common.h"
#include "database.h"

inline pdd_images_t *get_images()
{
	static pdd_images_t *images = NULL;
	if (!images)
	{
		images = g_ptr_array_new();
	}
	return images;
}

pdd_image_t *image_new_with_id(gint64 id, const gchar *name, gconstpointer data, gsize data_length)
{
	pdd_image_t *image = g_new(pdd_image_t, 1);
	image->id = id;
	image->name = g_strdup(name);
	image->data = g_memdup(data, data_length);
	image->data_length = data_length;
	return image;
}

pdd_image_t *image_copy(pdd_image_t *image)
{
	return image_new_with_id(image->id, image->name, image->data, image->data_length);
}

pdd_image_t *image_new(const gchar *name, gconstpointer data, gsize data_length)
{
	return image_new_with_id(0, name, data, data_length);
}

void image_free(pdd_image_t *image)
{
	g_free(image->name);
	g_free(image->data);
	g_free(image);
}

gboolean image_save(pdd_image_t *image)
{
	if (!use_cache)
	{
		static gint64 id = 0;
		image->id = ++id;
		g_ptr_array_add(get_images(), image_copy(image));
		return TRUE;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "INSERT INTO `images` (`name`, `data`) VALUES (?, ?)", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("image: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	gchar *image_name = g_utf8_strdown(image->name, -1);
	g_free(image->name);
	image->name = image_name;
	result = sqlite3_bind_text(stmt, 1, image->name, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_blob(stmt, 2, image->data, image->data_length, NULL);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_DONE)
	{
		g_error("image: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}
	
	image->id = sqlite3_last_insert_rowid(db);

	return TRUE;
}

pdd_image_t *image_find_by_id(gint64 id)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_images()->len; i++)
		{
			pdd_image_t *image = g_ptr_array_index(get_images(), i);
			if (image->id == id)
			{
				return image_copy(image);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `name`, `data` FROM `images` WHERE `rowid`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("image: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_bind_int64(stmt, 1, id);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("image: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	const gchar *name = (const gchar *)sqlite3_column_text(stmt, 0);
	gconstpointer data = sqlite3_column_blob(stmt, 1);
	gsize data_length = sqlite3_column_bytes(stmt, 1);

	return image_new_with_id(id, name, data, data_length);
}

pdd_image_t *image_find_by_name(const gchar *name)
{
	if (!use_cache)
	{
		gsize i;
		for (i = 0; i < get_images()->len; i++)
		{
			pdd_image_t *image = g_ptr_array_index(get_images(), i);
			if (!g_strcmp0(image->name, name))
			{
				return image_copy(image);
			}
		}
		return NULL;
	}

	static sqlite3_stmt *stmt = NULL;
	sqlite3 *db = database_get();
	int result;

	if (!stmt)
	{
		result = sqlite3_prepare_v2(db, "SELECT `rowid`, `data` FROM `images` WHERE `name`=? LIMIT 1", -1, &stmt, NULL);
		if (result != SQLITE_OK)
		{
			g_error("image: unable to prepare statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
	}

	result = sqlite3_reset(stmt);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to reset prepared statement (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	gchar *image_name = g_utf8_strdown(name, -1);
	result = sqlite3_bind_text(stmt, 1, image_name, -1, NULL);
	if (result != SQLITE_OK)
	{
		g_error("image: unable to bind param (%d: %s)\n", result, sqlite3_errmsg(db));
	}

	result = sqlite3_step(stmt);
	if (result != SQLITE_ROW)
	{
		if (result != SQLITE_DONE)
		{
			g_error("image: unable to perform statement (%d: %s)\n", result, sqlite3_errmsg(db));
		}
		return NULL;
	}

	gint64 id = sqlite3_column_int64(stmt, 0);
	gconstpointer data = sqlite3_column_blob(stmt, 1);
	gsize data_length = sqlite3_column_bytes(stmt, 1);

	pdd_image_t *image = image_new_with_id(id, image_name, data, data_length);

	g_free(image_name);

	return image;
}

pdd_images_t *image_copy_all(pdd_images_t *images)
{
	pdd_images_t *images_copy = g_ptr_array_new();
	gsize i;
	for (i = 0; i < images->len; i++)
	{
		pdd_image_t *image = g_ptr_array_index(images, i);
		g_ptr_array_add(images_copy, image_copy(image));
	}
	return images_copy;
}

void image_free_all(pdd_images_t *images)
{
	g_ptr_array_foreach(images, (GFunc)image_free, NULL);
	g_ptr_array_free(images, TRUE);
}
