#ifndef PDDBY_GTK_TICKET_DIALOG_H
#define PDDBY_GTK_TICKET_DIALOG_H

#include <gtk/gtk.h>

GtkWidget *ticket_dialog_new(gint maximum);

gint ticket_dialog_get_number(GtkWidget *dialog);

#endif // PDDBY_GTK_TICKET_DIALOG_H
