#include "chooser_dialog.h"
#include "config.h"
#include "pddby/section.h"
#include "pddby/topic.h"
#include "settings.h"

#include <string.h>

#if !GTK_CHECK_VERSION(2, 20, 0)
#define gtk_widget_get_visible(x) GTK_WIDGET_VISIBLE(x)
#endif

gint last_index[2] = {0, 0};

void on_chooser_dialog_item_changed(GtkWidget *widget);
GtkListStore *sections_model_new(pddby_t* pddby);
GtkListStore *topics_model_new(pddby_t* pddby);

static GtkWidget *chooser_dialog_new(const gchar *title, GtkListStore *model, gint text_column, gint index,
    gboolean need_title)
{
    GError *err = NULL;
    GtkBuilder *builder = gtk_builder_new();
    gchar *ui_filename = g_build_filename(get_share_dir(), "gtk", "chooser_dialog.ui", NULL);
    gtk_builder_add_from_file(builder, ui_filename, &err);
    g_free(ui_filename);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    gtk_builder_connect_signals(builder, NULL);
    GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(builder, "chooser_dialog"));

    g_object_set_data_full(G_OBJECT(dialog), "pdd-builder", builder, g_object_unref);

    if (!need_title)
    {
        GtkWidget *title_label = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_title"));
        gtk_widget_hide(title_label);
    }

    GtkWidget *items_combo = GTK_WIDGET(gtk_builder_get_object(builder, "cb_items"));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(items_combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(items_combo), renderer, "text", text_column, NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(items_combo), GTK_TREE_MODEL(model));
    gtk_combo_box_set_active(GTK_COMBO_BOX(items_combo), index);
    on_chooser_dialog_item_changed(items_combo);

    gtk_window_set_title(GTK_WINDOW(dialog), title);

    return dialog;
}

GtkWidget *chooser_dialog_new_with_sections(pddby_t* pddby)
{
    return chooser_dialog_new("Выберите главу", sections_model_new(pddby), 2, last_index[0], TRUE);
}

GtkWidget *chooser_dialog_new_with_topics(pddby_t* pddby)
{
    return chooser_dialog_new("Выберите тематический раздел", topics_model_new(pddby), 1, last_index[1], FALSE);
}

gint64 chooser_dialog_get_id(GtkWidget *dialog)
{
    GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(dialog), "pdd-builder"));
    GtkComboBox *items_combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cb_items"));
    GtkListStore *model = GTK_LIST_STORE(gtk_combo_box_get_model(items_combo));
    GtkTreeIter iter;
    gtk_combo_box_get_active_iter(items_combo, &iter);
    GValue id;
    memset(&id, 0, sizeof(id));
    gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, 0, &id);
    return g_value_get_int64(&id);
}

static void add_section_to_model(pddby_section_t *section, GtkListStore *model)
{
    GtkTreeIter iter;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, 0, section->id, 1, section->name, 2, section->title_prefix, 3, section->title, 4,
        pddby_section_get_question_count(section), -1);
}

GtkListStore *sections_model_new(pddby_t* pddby)
{
    GtkListStore *model = gtk_list_store_new(5, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
    pddby_sections_t *sections = pddby_sections_find_all(pddby);
    pddby_array_foreach(sections, (GFunc)add_section_to_model, model);
    pddby_sections_free(sections);
    return model;
}

static void add_topic_to_model(pddby_topic_t *topic, GtkListStore *model)
{
    GtkTreeIter iter;
    gtk_list_store_append(model, &iter);
    gtk_list_store_set(model, &iter, 0, topic->id, 1, topic->title, 2, (pddby_topic_get_question_count(topic) + 9) / 10, -1);
}

GtkListStore *topics_model_new(pddby_t* pddby)
{
    GtkListStore *model = gtk_list_store_new(3, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_INT);
    pddby_topics_t *topics = pddby_topics_find_all(pddby);
    pddby_array_foreach(topics, (GFunc)add_topic_to_model, model);
    pddby_topics_free(topics);
    return model;
}

GNUC_VISIBLE void on_chooser_dialog_destroy(GtkWidget *widget)
{
    GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gtk_widget_get_toplevel(widget)), "pdd-builder"));
    GtkComboBox *items_combo = GTK_COMBO_BOX(gtk_builder_get_object(builder, "cb_items"));
    GtkLabel *title_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_title"));
    last_index[gtk_widget_get_visible(GTK_WIDGET(title_label)) ? 0 : 1] = gtk_combo_box_get_active(items_combo);
}

GNUC_VISIBLE void on_chooser_dialog_item_changed(GtkWidget *widget)
{
    GtkListStore *model = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(widget)));
    GtkTreeIter iter;
    gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter);

    GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(gtk_widget_get_toplevel(widget)), "pdd-builder"));

    GtkLabel *title_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_title"));
    if (gtk_widget_get_visible(GTK_WIDGET(title_label)))
    {
        GValue title;
        memset(&title, 0, sizeof(title));
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, 3, &title);
        gchar *title_text = g_strdup_printf("<big><b>%s</b></big>", g_value_get_string(&title));
        gtk_label_set_markup(title_label, title_text);
        g_free(title_text);
    }

    GtkLabel *count_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_count"));
    GValue count;
    memset(&count, 0, sizeof(count));
    gchar *count_text = NULL;
    if (gtk_widget_get_visible(GTK_WIDGET(title_label)))
    {
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, 4, &count);
        count_text = g_strdup_printf("Количество вопросов: %d", g_value_get_int(&count));
    }
    else
    {
        gtk_tree_model_get_value(GTK_TREE_MODEL(model), &iter, 2, &count);
        count_text = g_strdup_printf("Количество билетов: %d", g_value_get_int(&count));
    }
    gtk_label_set_text(count_label, count_text);
    g_free(count_text);
}
