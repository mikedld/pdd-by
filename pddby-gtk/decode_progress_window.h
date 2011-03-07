#ifndef PDDBY_GTK_DECODE_PROGRESS_WINDOW_H
#define PDDBY_GTK_DECODE_PROGRESS_WINDOW_H

#include "pddby/pddby.h"

#include <gtk/gtk.h>

GtkWidget* decode_progress_window_new();
void decode_progress_window_enable_close(GtkWidget* window);

pddby_callbacks_t const* decode_progress_window_get_callbacks(GtkWidget* window);

#endif // PDDBY_GTK_DECODE_PROGRESS_WINDOW_H
