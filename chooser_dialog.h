#ifndef CHOOSER_DIALOG_H
#define CHOOSER_DIALOG_H

#include <gtk/gtk.h>

GtkWidget *chooser_dialog_new_with_sections();
GtkWidget *chooser_dialog_new_with_topics();

gint64 chooser_dialog_get_id(GtkWidget *dialog);

#endif // CHOOSER_DIALOG_H