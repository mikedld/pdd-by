#include "database.h"
#include "section.h"
#include "topic.h"
#include "yaml_helper.h"

#include <glib/gstdio.h>
#include <unistd.h>
#include <yaml.h>

extern gchar *program_dir;

gchar *database_file = NULL;
sqlite3 *database = NULL;
gint database_tx_count = 0;

gboolean create_tables();
gboolean bootstrap_data();

gboolean database_exists()
{
	if (database_file == NULL)
	{
		database_file = g_build_filename(g_get_user_cache_dir(), "pdd.db", NULL);
		// for testing purposes
		//g_unlink(database_file);
	}

	return g_access(database_file, R_OK) == 0;
}

void database_cleanup()
{
	if (database)
	{
		sqlite3_close(database);
	}
	if (database_file)
	{
		g_free(database_file);
	}
}

sqlite3 *database_get()
{
	if (database)
	{
		return database;
	}
	if (database_file == NULL)
	{
		g_error("tried getting database while not initialized\n");
	}

	gboolean is_new = !database_exists();

	int result = sqlite3_open(database_file, &database);

	if (result != SQLITE_OK)
	{
		g_error("unable to open database (%d)\n", result);
	}
	else if (!create_tables())
	{
		sqlite3_close(database);
		g_unlink(database_file);
		database = NULL;
	}
	else if (is_new && !bootstrap_data())
	{
		sqlite3_close(database);
		g_unlink(database_file);
		database = NULL;
	}

	if (database == NULL)
	{
		g_error("failed initializing database\n");
	}

	return database;
}

void database_tx_begin()
{
	database_tx_count++;
	if (database_tx_count > 1)
	{
		return;
	}
	int result = sqlite3_exec(database_get(), "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to begin transaction (%d)\n", result);
	}
}

void database_tx_commit()
{
	if (!database_tx_count)
	{
		g_error("unable to commit (no transaction in effect)\n");
	}
	database_tx_count--;
	if (database_tx_count)
	{
		return;
	}
	int result = sqlite3_exec(database_get(), "COMMIT TRANSACTION", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to commit transaction (%d)\n", result);
	}
}

gboolean create_tables()
{
	int result;

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `images` (`name` TEXT, `data` BLOB)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `images` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `comments` (`number` INT, `text` TEXT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `comments` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `traffregs` (`number` INT, `text` TEXT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `traffregs` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `images_traffregs` (`image_id` INT, `traffreg_id` INT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `images_traffregs` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `sections` (`name` TEXT, `title_prefix` TEXT, `title` TEXT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `sections` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `topics` (`number` TEXT, `title` TEXT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `topics` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `questions` (`topic_id` INT, `text` TEXT, `image_id` INT, `advice` TEXT, `comment_id` INT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `questions` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `questions_sections` (`question_id` INT, `section_id`)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `questions_sections` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `questions_traffregs` (`question_id` INT, `traffreg_id`)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `questions_traffregs` table (%d)\n", result);
	}

	result = sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS `answers` (`question_id` INT, `text` TEXT, `is_correct` INT)", NULL, NULL, NULL);
	if (result != SQLITE_OK)
	{
		g_error("unable to create `answers` table (%d)\n", result);
	}

	return TRUE;
}

gboolean bootstrap_sections()
{
	g_print("bootstrapping sections...\n");
	yaml_parser_t parser;
	FILE *f = yaml_open_file();
	if (!f)
	{
		g_error("unable to open YAML file\n");
	}
	if (!yaml_init_and_find_node(&parser, f, "sections"))
	{
		return FALSE;
	}
	if (yaml_get_event(&parser, NULL) != YAML_MAPPING_START_EVENT)
	{
		g_error("malformed YAML\n");
	}
	void database_tx_begin();
	while (TRUE)
	{
		gchar *name, *title_prefix, *title;
		yaml_event_type_t event1 = yaml_get_event(&parser, &name);
		if (event1 == YAML_MAPPING_END_EVENT)
		{
			break;
		}
		yaml_event_type_t event2 = yaml_get_event(&parser, NULL);
		yaml_event_type_t event3 = yaml_get_event(&parser, &title_prefix);
		yaml_event_type_t event4 = yaml_get_event(&parser, &title);
		yaml_event_type_t event5 = yaml_get_event(&parser, NULL);

		if (event1 != YAML_SCALAR_EVENT ||
			event2 != YAML_SEQUENCE_START_EVENT ||
			event3 != YAML_SCALAR_EVENT ||
			event4 != YAML_SCALAR_EVENT ||
			event5 != YAML_SEQUENCE_END_EVENT)
		{
			g_error("malformed YAML\n");
		}

		pdd_section_t *section = section_new(name, title_prefix, title);
		section_save(section);
		section_free(section);
		g_free(title);
		g_free(title_prefix);
		g_free(name);
	}
	void database_tx_commit();
	yaml_parser_delete(&parser);
	fclose(f);
	return TRUE;
}

gboolean bootstrap_topics()
{
	g_print("bootstrapping topics...\n");
	yaml_parser_t parser;
	FILE *f = yaml_open_file();
	if (!f)
	{
		g_error("unable to open YAML file\n");
	}
	if (!yaml_init_and_find_node(&parser, f, "topics"))
	{
		return FALSE;
	}
	if (yaml_get_event(&parser, NULL) != YAML_MAPPING_START_EVENT)
	{
		g_error("malformed YAML\n");
	}
	void database_tx_begin();
	while (TRUE)
	{
		gchar *number, *title;
		yaml_event_type_t event1 = yaml_get_event(&parser, &number);
		if (event1 == YAML_MAPPING_END_EVENT)
		{
			break;
		}
		yaml_event_type_t event2 = yaml_get_event(&parser, &title);

		if (event1 != YAML_SCALAR_EVENT ||
			event2 != YAML_SCALAR_EVENT)
		{
			g_error("malformed YAML\n");
		}

		pdd_topic_t *topic = topic_new(atoi(number), title);
		topic_save(topic);
		topic_free(topic);
		g_free(number);
		g_free(title);
	}
	void database_tx_commit();
	yaml_parser_delete(&parser);
	fclose(f);
	return TRUE;
}

gboolean bootstrap_data()
{
	if (!bootstrap_sections())
	{
		return FALSE;
	}

	if (!bootstrap_topics())
	{
		return FALSE;
	}

	return TRUE;
}
