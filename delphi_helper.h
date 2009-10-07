#ifndef DELPHI_HELPER_H
#define DELPHI_HELPER_H

#include <glib.h>

guint64 delphi_random(guint32 limit);

void set_randseed(guint64 seed);
void init_randseed_for_image(const gchar *name, guint16 magic);

#endif // DELPHI_HELPER_H
