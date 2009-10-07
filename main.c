#include "main_window.h"
#include "database.h"
#include "decode.h"

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

gboolean use_cache = FALSE;
GtkWidget *main_window = NULL;

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	GConfClient *conf = gconf_client_get_default();
	gconf_client_add_dir(conf, "/apps/pdd-by", GCONF_CLIENT_PRELOAD_NONE, NULL);
	use_cache = gconf_client_get_bool(conf, "/apps/pdd-by/use-cache", NULL);

	if (!use_cache || !database_exists())
	{
		GtkWidget *directory_dialog = gtk_file_chooser_dialog_new("Укажите путь к директории Pdd32 на компакт-диске", NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		GtkWidget *use_cache_checkbutton = gtk_check_button_new_with_label("Кэшировать данные на жёсткий диск "
			"(подумайте дважды: \"Новый поворот\" не одобряет и может обидеться)");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_cache_checkbutton), use_cache);
		gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(directory_dialog), use_cache_checkbutton);
		if (gtk_dialog_run(GTK_DIALOG(directory_dialog)) != GTK_RESPONSE_ACCEPT)
		{
			return 1;
		}
		gchar *pdd32_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(directory_dialog));
		use_cache = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_cache_checkbutton));
		gconf_client_set_bool(conf, "/apps/pdd-by/use-cache", use_cache, NULL);
		gtk_widget_destroy(directory_dialog);
		// TODO: check if path corresponds to mounted CD-ROM device
		if (!decode(pdd32_path))
		{
			g_error("ERROR: unable to decode");
		}
		g_free(pdd32_path);
	}

	main_window = main_window_new();
	gtk_widget_show(main_window);
	gtk_main();

	database_cleanup();
	
	return 0;
}
