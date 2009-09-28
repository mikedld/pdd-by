#include "yaml_helper.h"

#include <glib/gstdio.h>

extern gchar *program_dir;

FILE *yaml_open_file()
{
	gchar *path = g_build_filename(program_dir, "data", "10.yml", NULL);
	FILE *f = g_fopen(path, "r");
	g_free(path);
	return f;
}

gboolean yaml_init_and_find_node(yaml_parser_t *parser, FILE *yaml_file, const gchar *node_path)
{
	yaml_event_t event;
	if (!yaml_parser_initialize(parser))
	{
		g_error("unable to initialize YAML parser\n");
	}
	yaml_parser_set_input_file(parser, yaml_file);
	gchar **node_path_parts = g_strsplit(node_path, "/", 0);
	if (node_path_parts[0] == NULL)
	{
		return TRUE;
	}
	gint level = 0;
	gint part = 0;
	while (TRUE)
	{
		if (!yaml_parser_parse(parser, &event))
		{
			g_error("unable to parse YAML data\n");
		}
		switch (event.type)
		{
		case YAML_SCALAR_EVENT:
			if (level == part + 2 && !g_strcmp0((const gchar *)event.data.scalar.value, node_path_parts[part]))
			{
				part++;
				if (node_path_parts[part] == NULL)
				{
					g_strfreev(node_path_parts);
					return TRUE;
				}
			}
			break;
		case YAML_DOCUMENT_START_EVENT:
		case YAML_SEQUENCE_START_EVENT:
		case YAML_MAPPING_START_EVENT:
			level++;
			break;
		case YAML_DOCUMENT_END_EVENT:
		case YAML_SEQUENCE_END_EVENT:
		case YAML_MAPPING_END_EVENT:
			level--;
			break;
		default:
			;
		}
		if (event.type == YAML_STREAM_END_EVENT)
		{
			break;
		}
	}
	g_error("unable to find YAML node: %s\n", node_path);
}

yaml_event_type_t yaml_get_event(yaml_parser_t *parser, gchar **event_value)
{
	yaml_event_t event;
	if (!yaml_parser_parse(parser, &event))
	{
		g_error("unable to parse YAML data\n");
	}
	if (event_value != NULL)
	{
		if (event.type == YAML_SCALAR_EVENT)
		{
			*event_value = g_strdup((const gchar *)event.data.scalar.value);
		}
		else
		{
			*event_value = NULL;
		}
	}
	return event.type;
}
