#include "help_dialog.h"
#include "config.h"
#include "pddby/comment.h"
#include "pddby/image.h"
#include "pddby/traffreg.h"
#include "settings.h"

static GtkWidget *help_dialog_new(GtkBuilder **builder)
{
    GError *err = NULL;
    *builder = gtk_builder_new();
    gchar *ui_filename = g_build_filename(get_share_dir(), "gtk", "help_dialog.ui", NULL);
    gtk_builder_add_from_file(*builder, ui_filename, &err);
    g_free(ui_filename);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    gtk_builder_connect_signals(*builder, NULL);
    GtkWidget *dialog = GTK_WIDGET(gtk_builder_get_object(*builder, "help_dialog"));

    g_object_set_data_full(G_OBJECT(dialog), "pdd-builder", *builder, g_object_unref);

    return dialog;
}

static void add_images_to_box(GtkWidget *box, const pddby_images_t *images)
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    for (gsize i = 0, size = pddby_array_size(images); i < size; i++)
    {
        const pddby_image_t *image = pddby_array_index(images, i);
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
        GError *err = NULL;
        if (!gdk_pixbuf_loader_write(loader, image->data, image->data_length, &err))
        {
            g_error("%s\n", err->message);
        }
        if (!gdk_pixbuf_loader_close(loader, &err))
        {
            g_error("%s\n", err->message);
        }
        GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
        GtkWidget *box_image = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref(loader);
        gtk_box_pack_start(GTK_BOX(hbox), box_image, FALSE, FALSE, 0);
    }
    GtkWidget *box_alignment = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
    //gtk_widget_set_size_request(box_alignment, 650, -1);
    gtk_container_add(GTK_CONTAINER(box_alignment), hbox);
    gtk_box_pack_start(GTK_BOX(box), box_alignment, FALSE, TRUE, 0);
}

static void add_text_to_box(GtkWidget *box, const gchar *text)
{
    GtkWidget *box_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(box_label), text);
    gtk_label_set_line_wrap(GTK_LABEL(box_label), TRUE);
    gtk_widget_set_size_request(box_label, 650, -1);
    gtk_misc_set_padding(GTK_MISC(box_label), 10, 5);
    GtkWidget *box_alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
    gtk_container_add(GTK_CONTAINER(box_alignment), box_label);
    gtk_box_pack_start(GTK_BOX(box), box_alignment, FALSE, TRUE, 0);
}

GtkWidget *help_dialog_new_with_comment(const pddby_question_t *question)
{
    GtkBuilder *builder;
    GtkWidget *dialog = help_dialog_new(&builder);

    pddby_comment_t *comment = pddby_comment_find_by_id(question->pddby, question->comment_id);
    if (!comment)
    {
        return NULL;
    }

    GtkWidget *help_box = GTK_WIDGET(gtk_builder_get_object(builder, "box_help"));
    add_text_to_box(help_box, comment->text);
    gtk_widget_show_all(help_box);
    pddby_comment_free(comment);

    return dialog;
}

GtkWidget *help_dialog_new_with_traffregs(const pddby_question_t *question)
{
    GtkBuilder *builder;
    GtkWidget *dialog = help_dialog_new(&builder);

    pddby_traffregs_t *traffregs = pddby_traffregs_find_by_question(question->pddby, question->id);
    if (!traffregs || !pddby_array_size(traffregs))
    {
        return NULL;
    }

    GtkWidget *help_box = GTK_WIDGET(gtk_builder_get_object(builder, "box_help"));
    for (gsize i = 0, size = pddby_array_size(traffregs); i < size; i++)
    {
        pddby_traffreg_t *traffreg = pddby_array_index(traffregs, i);
        pddby_images_t *images = pddby_images_find_by_traffreg(traffreg->pddby, traffreg->id);
        add_images_to_box(help_box, images);
        add_text_to_box(help_box, traffreg->text);
        pddby_images_free(images);
    }
    gtk_widget_show_all(help_box);
    pddby_traffregs_free(traffregs);

    return dialog;
}
