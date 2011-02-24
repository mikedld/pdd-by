#ifndef PDDBY_GTK_CHOOSER_DIALOG_H
#define PDDBY_GTK_CHOOSER_DIALOG_H

#include "pddby/pddby.h"

#include <gtk/gtk.h>

GtkWidget *chooser_dialog_new_with_sections(pddby_t* pddby);
GtkWidget *chooser_dialog_new_with_topics(pddby_t* pddby);

gint64 chooser_dialog_get_id(GtkWidget *dialog);

#endif // PDDBY_GTK_CHOOSER_DIALOG_H
