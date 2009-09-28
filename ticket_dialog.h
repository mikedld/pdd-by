#ifndef TICKET_DIALOG_H
#define TICKET_DIALOG_H

#include <gtk/gtk.h>

GtkWidget *ticket_dialog_new(gint maximum);

gint ticket_dialog_get_number(GtkWidget *dialog);

#endif // TICKET_DIALOG_H
