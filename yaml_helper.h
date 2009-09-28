#ifndef YAML_HELPER_H
#define YAML_HELPER_H

#include <glib.h>
#include <yaml.h>

FILE *yaml_open_file();

gboolean yaml_init_and_find_node(yaml_parser_t *parser, FILE *yaml_file, const gchar *node_path);

yaml_event_type_t yaml_get_event(yaml_parser_t *parser, gchar **event_value);

#endif // YAML_HELPER_H
