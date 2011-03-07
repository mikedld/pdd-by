#include "decode_progress_window.h"

#include "settings.h"

static GtkWidget *gs_decode_progress_window = NULL;

static void on_message(pddby_t* pddby, int type, char const* text);
static void on_progress_begin(pddby_t* pddby, int size);
static void on_progress(pddby_t* pddby, int pos);
static void on_progress_end(pddby_t* pddby);

static pddby_callbacks_t const gs_callbacks =
{
    &on_message,
    &on_progress_begin,
    &on_progress,
    &on_progress_end
};

GtkWidget* decode_progress_window_new()
{
    GError* err = NULL;
    GtkBuilder* builder = gtk_builder_new();
    gchar* ui_filename = g_build_filename(get_share_dir(), "gtk", "decode_progress_window.ui", NULL);
    gtk_builder_add_from_file(builder, ui_filename, &err);
    g_free(ui_filename);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    gtk_builder_connect_signals(builder, NULL);
    gs_decode_progress_window = GTK_WIDGET(gtk_builder_get_object(builder, "decode_progress_window"));

    GtkTreeView* tv_log = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tv_log"));

    //GtkCellRenderer* type_renderer = gtk_cell_renderer_text_new();
    //gtk_tree_view_insert_column_with_attributes(tv_log, 0, "Type", type_renderer, "text", 0, NULL);

    GtkCellRenderer* message_renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(tv_log, 0, "Message", message_renderer, "text", 1, NULL);
    gtk_tree_view_column_set_sizing(gtk_tree_view_get_column(tv_log, 0), GTK_TREE_VIEW_COLUMN_GROW_ONLY);

    g_object_set_data_full(G_OBJECT(gs_decode_progress_window), "pdd-builder", builder, g_object_unref);

    return gs_decode_progress_window;
}

void decode_progress_window_enable_close(GtkWidget* window)
{
    GtkBuilder* builder = GTK_BUILDER(g_object_get_data(G_OBJECT(window), "pdd-builder"));
    GtkButton* btn_close = GTK_BUTTON(gtk_builder_get_object(builder, "btn_close"));
    gtk_widget_set_sensitive(GTK_WIDGET(btn_close), TRUE);
}

pddby_callbacks_t const* decode_progress_window_get_callbacks(G_GNUC_UNUSED GtkWidget* window)
{
    return &gs_callbacks;
}

static void on_message(G_GNUC_UNUSED pddby_t* pddby, int type, char const* text)
{
    if (!gs_decode_progress_window)
    {
        return;
    }

    GtkBuilder* builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gs_decode_progress_window), "pdd-builder"));
    GtkListStore* ls_log = GTK_LIST_STORE(gtk_builder_get_object(builder, "ls_log"));

    GtkTreeIter iter;
    gtk_list_store_append(ls_log, &iter);
    gchar const* type_text = "Unknown";
    switch (type)
    {
    case pddby_message_type_debug:
        type_text = "Debug";
        break;
    case pddby_message_type_log:
        type_text = "Log";
        break;
    case pddby_message_type_warning:
        type_text = "Warning";
        break;
    case pddby_message_type_error:
        type_text = "Error";
        break;
    }
    gtk_list_store_set(ls_log, &iter, 0, type_text, 1, text, -1);

    GtkTreeView* tv_log = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tv_log"));
    gint row_count = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(tv_log), NULL);
    GtkTreePath* path = gtk_tree_path_new_from_indices(row_count - 1, -1);
    gtk_tree_view_scroll_to_cell(tv_log, path, NULL, FALSE, 0, 0);
    gtk_tree_path_free(path);

    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
}

static void on_progress_begin(G_GNUC_UNUSED pddby_t* pddby, int size)
{
    if (!gs_decode_progress_window)
    {
        return;
    }

    GtkBuilder* builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gs_decode_progress_window), "pdd-builder"));

    GtkAdjustment* pb_progress_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "pb_progress_adjustment"));
    gtk_adjustment_set_upper(pb_progress_adjustment, size);

    GtkProgressBar* pb_progress = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "pb_progress"));
    gchar* text = g_strdup_printf("0 из %d", size);
    gtk_progress_bar_set_text(pb_progress, text);
    g_free(text);

    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
}

static void on_progress(G_GNUC_UNUSED pddby_t* pddby, int pos)
{
    if (!gs_decode_progress_window)
    {
        return;
    }

    GtkBuilder* builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gs_decode_progress_window), "pdd-builder"));

    GtkAdjustment* pb_progress_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "pb_progress_adjustment"));
    gtk_adjustment_set_value(pb_progress_adjustment, pos);

    GtkProgressBar* pb_progress = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "pb_progress"));
    gint size = gtk_adjustment_get_upper(pb_progress_adjustment);
    gchar* text = g_strdup_printf("%d из %d", pos, size);
    gtk_progress_bar_set_text(pb_progress, text);
    g_free(text);

    if (!(pos % 10) || pos == size)
    {
        while (gtk_events_pending())
        {
            gtk_main_iteration();
        }
    }
}

static void on_progress_end(G_GNUC_UNUSED pddby_t* pddby)
{
    if (!gs_decode_progress_window)
    {
        return;
    }

    GtkBuilder* builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gs_decode_progress_window), "pdd-builder"));

    GtkAdjustment* pb_progress_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "pb_progress_adjustment"));
    gtk_adjustment_set_value(pb_progress_adjustment, 0);

    GtkProgressBar* pb_progress = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "pb_progress"));
    gtk_progress_bar_set_text(pb_progress, NULL);

    while (gtk_events_pending())
    {
        gtk_main_iteration();
    }
}
