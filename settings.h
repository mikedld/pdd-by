#ifndef SETTINGS_H
#define SETTINGS_H

#include <glib.h>

gchar *get_settings(const gchar *key);

const gchar *get_share_dir();

#endif // SETTINGS_H
