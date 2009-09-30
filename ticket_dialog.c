#include "ticket_dialog.h"

#include <gtk/gtk.h>

GtkWidget *ticket_dialog_new(gint maximum)
{
	GError *err = NULL;
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "ui/ticket_dialog.ui", &err);
	if (err)
	{
		g_error("%s\n", err->message);
	}

	gtk_builder_connect_signals(builder, NULL);
	GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(builder, "ticket_dialog"));

	g_object_set_data_full(G_OBJECT(dialog), "pdd-builder", builder, g_object_unref);

	GtkAdjustment *adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "number_adjustment"));
	gtk_adjustment_set_upper(adjustment, maximum);
	gtk_adjustment_set_value(adjustment, 1);

	return dialog;
}

gint ticket_dialog_get_number(GtkWidget *dialog)
{
	GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(dialog), "pdd-builder"));
	GtkSpinButton *number_spin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spin_number"));
	return gtk_spin_button_get_value_as_int(number_spin);
}
