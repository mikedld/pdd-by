#include "main_window.h"
#include "chooser_dialog.h"
#include "config.h"
#include "pddby/topic.h"
#include "platform.h"
#include "question_window.h"
#include "settings.h"
#include "ticket_dialog.h"

extern GtkWidget *main_window;

void on_quit();

GtkWidget *main_window_new(pddby_t* pddby)
{
    GError *err = NULL;
    GtkBuilder *builder = gtk_builder_new();
    gchar *ui_filename = g_build_filename(get_share_dir(), "gtk", "main_window.ui", NULL);
    gtk_builder_add_from_file(builder, ui_filename, &err);
    g_free(ui_filename);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    gtk_builder_connect_signals(builder, NULL);
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));

    g_object_set_data(G_OBJECT(window), "pdd-pddby-handle", pddby);
    g_object_set_data_full(G_OBJECT(window), "pdd-builder", builder, g_object_unref);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_quit), NULL);

    return window;
}

GNUC_VISIBLE void on_training_section()
{
    gtk_widget_hide(main_window);
    pddby_t* pddby = g_object_get_data(G_OBJECT(main_window), "pdd-pddby-handle");
    GtkWidget *dialog = chooser_dialog_new_with_sections(pddby);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(dialog);
        pddby_section_t *section = pddby_section_find_by_id(pddby, chooser_dialog_get_id(dialog));
        GtkWidget *question_window = question_window_new_with_section(section, FALSE);
        gtk_widget_show(question_window);
    }
    else
    {
        gtk_widget_destroy(dialog);
        gtk_widget_show(main_window);
    }
}

static void on_topic(gboolean is_exam)
{
    gtk_widget_hide(main_window);
    pddby_t* pddby = g_object_get_data(G_OBJECT(main_window), "pdd-pddby-handle");
    GtkWidget *dialog = chooser_dialog_new_with_topics(pddby);

once_again:
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        pddby_topic_t *topic = pddby_topic_find_by_id(pddby, chooser_dialog_get_id(dialog));
        GtkWidget *ticket_dialog = ticket_dialog_new((pddby_topic_get_question_count(topic) + 9) / 10);
        if (gtk_dialog_run(GTK_DIALOG(ticket_dialog)) != GTK_RESPONSE_OK)
        {
            gtk_widget_destroy(ticket_dialog);
            goto once_again;
        }
        gtk_widget_destroy(ticket_dialog);
        gtk_widget_destroy(dialog);
        GtkWidget *question_window = question_window_new_with_topic(topic, ticket_dialog_get_number(ticket_dialog),
            is_exam);
        gtk_widget_show(question_window);
    }
    else
    {
        gtk_widget_destroy(dialog);
        gtk_widget_show(main_window);
    }
}

static void on_ticket(gboolean is_exam)
{
    gtk_widget_hide(main_window);
    pddby_t* pddby = g_object_get_data(G_OBJECT(main_window), "pdd-pddby-handle");
    GtkWidget *ticket_dialog = ticket_dialog_new(999);
    if (gtk_dialog_run(GTK_DIALOG(ticket_dialog)) == GTK_RESPONSE_OK)
    {
        gtk_widget_destroy(ticket_dialog);
        GtkWidget *question_window = question_window_new_with_ticket(pddby, ticket_dialog_get_number(ticket_dialog), is_exam);
        gtk_widget_destroy(ticket_dialog);
        gtk_widget_show(question_window);
    }
    else
    {
        gtk_widget_destroy(ticket_dialog);
        gtk_widget_show(main_window);
    }
}

static void on_random_ticket(gboolean is_exam)
{
    gtk_widget_hide(main_window);
    pddby_t* pddby = g_object_get_data(G_OBJECT(main_window), "pdd-pddby-handle");
    GtkWidget *question_window = question_window_new_with_random_ticket(pddby, is_exam);
    gtk_widget_show(question_window);
}

GNUC_VISIBLE void on_training_topic()
{
    on_topic(FALSE);
}

GNUC_VISIBLE void on_training_ticket()
{
    on_ticket(FALSE);
}

GNUC_VISIBLE void on_training_random_ticket()
{
    on_random_ticket(FALSE);
}

GNUC_VISIBLE void on_exam_topic()
{
    on_topic(TRUE);
}

GNUC_VISIBLE void on_exam_ticket()
{
    on_ticket(TRUE);
}

GNUC_VISIBLE void on_exam_random_ticket()
{
    on_random_ticket(TRUE);
}

GNUC_VISIBLE void on_quit()
{
    gtk_main_quit();
}

GNUC_VISIBLE void on_about()
{
    const gchar * const authors[] = { "Mike `mike.dld` Gelfand <mike.dld@gmail.com>", NULL };
    gtk_show_about_dialog(NULL,
        "program-name", "Учебная программа ПДД",
        "version", PDDBY_VERSION,
        "website", "http://mikedld.com/",
        "copyright", "Copyright © 2009-2011 Mike `mike.dld` Gelfand",
        "comments", "Империя наносит ответный удар\n(привет ЧПУП \"Новый поворот\")",
        "authors", authors,
        "license", "Данная программа распространяется свободно под лицензией GNU GPL v3.\n"
            "\n"
            "Запрещается использование программы в целях, нарушающих покой\n"
            "ЧПУП \"Новый поворот\". Для пользования данной программой, вы должны\n"
            "быть счастливым обладателем лицензионной копии компакт-диска\n"
            "вышеупомянутой компании. Распространение данных, полученных в\n"
            "результате работы каких-либо компонентов программы, а также любое\n"
            "их использование вне данной программы не является законным и автор\n"
            "не несёт за это никакой ответственности.",
        NULL);
}
