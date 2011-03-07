#include "decode_progress_window.h"
#include "main_window.h"
#include "pddby/pddby.h"
#include "settings.h"

#include <gtk/gtk.h>

GtkWidget *main_window = NULL;

int main(int argc, char *argv[])
{
#ifdef WIN32
    return 1;
#else
    gtk_init(&argc, &argv);

    gchar* cache_dir = g_build_filename(g_get_user_cache_dir(), "pddby", NULL);
    g_mkdir_with_parents(cache_dir, 0755);

    GtkWidget* decode_progress_window = decode_progress_window_new();

    pddby_t* pddby = pddby_init(get_share_dir(), cache_dir,
        decode_progress_window_get_callbacks(decode_progress_window));

    g_free(cache_dir);

    gboolean result = FALSE;

    if (pddby_cache_exists(pddby))
    {
        pddby_use_cache(pddby, TRUE);
        result = TRUE;
    }
    else
    {
        GtkWidget *directory_dialog = gtk_file_chooser_dialog_new("Укажите путь к директории Pdd32 на компакт-диске",
            NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
            GTK_RESPONSE_ACCEPT, NULL);
        GtkWidget *use_cache_checkbutton = gtk_check_button_new_with_label("Кэшировать данные на жёсткий диск "
            "(подумайте дважды: \"Новый поворот\" не одобряет и может обидеться)");
        gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(directory_dialog), use_cache_checkbutton);
        if (gtk_dialog_run(GTK_DIALOG(directory_dialog)) != GTK_RESPONSE_ACCEPT)
        {
            return 1;
        }

        gchar *pdd32_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(directory_dialog));
        pddby_use_cache(pddby, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_cache_checkbutton)));
        gtk_widget_destroy(directory_dialog);

        gtk_widget_show_all(decode_progress_window);

        while (gtk_events_pending())
        {
            gtk_main_iteration();
        }

        result = pddby_decode(pddby, pdd32_path);
        g_free(pdd32_path);

        if (result)
        {
            gtk_widget_destroy(decode_progress_window);
        }
        else
        {
            decode_progress_window_enable_close(decode_progress_window);
        }
    }

    if (result)
    {
        main_window = main_window_new(pddby);
        gtk_widget_show(main_window);
    }

    gtk_main();

    pddby_close(pddby);

    return 0;
#endif
}
