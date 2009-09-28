#ifndef DELPHI_HELPER_H
#define DELPHI_HELPER_H

#include <gtk/gtk.h>

inline guint64 delphi_random(guint32 limit);

inline void set_randseed(guint64 seed);
void init_randseed_for_image(const gchar *name, guint16 magic);

#endif // DELPHI_HELPER_H
