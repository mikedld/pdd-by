#include "main_window.h"
#include "database.h"
#include "decode.h"

#include <gtk/gtk.h>
#include <stdlib.h>

gchar *program_dir = NULL;
GtkWidget *main_window = NULL;

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	gint path_max;
#ifdef PATH_MAX
	path_max = PATH_MAX;
#else
	path_max = pathconf(name, _PC_PATH_MAX);
	if (path_max <= 0)
	{
		path_max = 1024;
	}
#endif
	char *path = g_malloc(path_max);
	program_dir = g_path_get_dirname(realpath(argv[0], path));
	g_free(path);

	if (!database_exists())
	{
		GtkWidget *directory_dialog = gtk_file_chooser_dialog_new("Укажите путь к директории Pdd32 на компакт-диске", NULL,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
		if (gtk_dialog_run(GTK_DIALOG(directory_dialog)) != GTK_RESPONSE_ACCEPT)
		{
			g_free(program_dir);
			return 1;
		}
		gchar *pdd32_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(directory_dialog));
		gtk_widget_destroy(directory_dialog);
		if (!decode(pdd32_path))
		{
			g_error("ERROR: unable to decode");
		}
		g_free(pdd32_path);
		if (!database_exists())
		{
			g_error("ERROR: unable to initialize database");
		}
	}

	main_window = main_window_new();
	gtk_widget_show(main_window);
	gtk_main();

	database_cleanup();
	
	g_free(program_dir);

	return 0;
}
