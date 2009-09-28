#include "question_window.h"
#include "answer.h"
#include "main_window.h"
#include "question.h"

#include <gdk/gdkkeysyms.h>

extern GtkWidget *main_window;

enum QuestionState
{
	UnknownState = 0,
	CorrectState,
	IncorrectState,
	SkippedState
};

typedef struct statistics_s
{
	pdd_questions_t *questions;
	GArray *states;
	gboolean is_exam;
	gint index;
	gint try_count;
	gint correct_index;
} statistics_t;

gint questions_type = 0;

void statistics_free(statistics_t *statistics);

void fetch_next_question(statistics_t *statistics, GtkWindow *window);

void on_question_real_quit();
void on_question_quit();
void on_question_answer();
void on_question_skip();
void on_question_show_traffregs();
void on_question_show_comment();

GtkWidget *question_window_new(gchar *title, pdd_questions_t *quesions, gboolean is_exam)
{
    GError *err = NULL;
    GtkBuilder *builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "ui/question_window.ui", &err);
    if (err)
	{
	    g_error("%s\n", err->message);
	}

    gtk_builder_connect_signals(builder, NULL);
    GtkWidget *window = GTK_WIDGET(gtk_builder_get_object(builder, "question_window"));

	g_object_set_data_full(G_OBJECT(window), "pdd-builder", builder, g_object_unref);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(on_question_real_quit), NULL);

	gchar *window_title = g_strdup_printf("%s - Учебная программа ПДД", title);
	g_free(title);
	gtk_window_set_title(GTK_WINDOW(window), window_title);
	g_free(window_title);

	GtkAccelGroup *accel_group = gtk_accel_group_new();
	gtk_accel_group_connect(accel_group, GDK_Escape, 0, GTK_ACCEL_VISIBLE, g_cclosure_new(on_question_quit, window, NULL));
	gtk_accel_group_connect(accel_group, GDK_Return, 0, GTK_ACCEL_VISIBLE, g_cclosure_new(on_question_answer, window, NULL));
	gtk_accel_group_connect(accel_group, GDK_space, 0, GTK_ACCEL_VISIBLE, g_cclosure_new(on_question_skip, window, NULL));
	gtk_accel_group_connect(accel_group, GDK_F1, 0, GTK_ACCEL_VISIBLE, g_cclosure_new(on_question_show_traffregs, window, NULL));
	gtk_accel_group_connect(accel_group, GDK_F2, 0, GTK_ACCEL_VISIBLE, g_cclosure_new(on_question_show_comment, window, NULL));
	gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

	GtkLabel *hint_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_hint"));
	if (is_exam)
	{
		gtk_label_set_markup(hint_label, "<b>Ввод</b> - ответить, <b>Пробел</b> - пропустить");
	}
	else
	{
		gtk_label_set_markup(hint_label, "<b>Ввод</b> - ответить, <b>Пробел</b> - пропустить, <b>F1</b> - правила, <b>F2</b> - комментарий");
	}

	statistics_t *statistics = g_new(statistics_t, 1);
	statistics->questions = quesions;
	statistics->states = g_array_sized_new(FALSE, TRUE, sizeof(gint8), quesions->len);
	g_array_set_size(statistics->states, quesions->len);
	statistics->is_exam = is_exam;
	statistics->index = -1;
	g_object_set_data_full(G_OBJECT(window), "pdd-statistics", statistics, (GDestroyNotify)statistics_free);

	fetch_next_question(statistics, GTK_WINDOW(window));

	return window;
}

GtkWidget *question_window_new_with_section(pdd_section_t *section, gboolean is_exam)
{
	questions_type = 0;
	return question_window_new(g_strdup(section->title_prefix), question_find_by_section(section->id), is_exam);
}

GtkWidget *question_window_new_with_topic(pdd_topic_t *topic, gint ticket_number, gboolean is_exam)
{
	questions_type = 1;
	return question_window_new(g_strdup_printf("Раздел %d, билет %d", topic->number, ticket_number),
		question_find_by_topic(topic->id, ticket_number), is_exam);
}

GtkWidget *question_window_new_with_ticket(gint ticket_number, gboolean is_exam)
{
	questions_type = 2;
	return question_window_new(g_strdup_printf("Билет %d", ticket_number),
		question_find_by_ticket(ticket_number), is_exam);
}

GtkWidget *question_window_new_with_random_ticket(gboolean is_exam)
{
	questions_type = 3;
	return question_window_new(g_strdup("Случайный билет"), question_find_random(), is_exam);
}

void statistics_free(statistics_t *statistics)
{
	question_free_all(statistics->questions);
	g_array_free(statistics->states, TRUE);
}

void update_question(statistics_t *statistics, GtkWindow *window)
{
	GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(window), "pdd-builder"));

	pdd_question_t *question = g_ptr_array_index(statistics->questions, statistics->index);

	gsize i;
	gint count[4] = {0, 0, 0, 0};
	for (i = 0; i < statistics->states->len; i++)
	{
		count[g_array_index(statistics->states, gint8, i)]++;
	}

	gchar *progress_text = g_strdup_printf("Вопрос %d из %d", statistics->index + 1, statistics->questions->len);
	if (count[IncorrectState] || count[SkippedState])
	{
		gchar *new_progress_text;
		if (count[IncorrectState] && count[SkippedState])
		{
			new_progress_text = g_strdup_printf("%s (%d неверно, %d пропущено)", progress_text, count[IncorrectState], count[SkippedState]);
		}
		else if (count[IncorrectState])
		{
			new_progress_text = g_strdup_printf("%s (%d неверно)", progress_text, count[IncorrectState]);
		}
		else
		{
			new_progress_text = g_strdup_printf("%s (%d пропущено)", progress_text, count[SkippedState]);
		}
		g_free(progress_text);
		progress_text = new_progress_text;
	}
	GtkProgressBar *progressbar = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "pb_progress"));
	gtk_progress_bar_set_text(progressbar, progress_text);
	gtk_progress_bar_set_fraction(progressbar, (count[CorrectState] + count[IncorrectState]) * 1. / statistics->states->len);
	g_free(progress_text);

	gchar *question_text = g_strdup_printf("<big><b>%s</b></big>", question->text);
	GtkLabel *question_text_label = GTK_LABEL(gtk_builder_get_object(builder, "lbl_question"));
	gtk_label_set_markup(question_text_label, question_text);
	g_free(question_text);

	GtkVBox *answers_box = GTK_VBOX(gtk_builder_get_object(builder, "box_answers"));
	gtk_container_foreach(GTK_CONTAINER(answers_box), (GtkCallback)gtk_widget_destroy, NULL);
	pdd_answers_t *answers = answer_find_by_question(question->id);
	GtkWidget *answer_radio = NULL;
	for (i = 0; i < answers->len; i++)
	{
		pdd_answer_t *answer = g_ptr_array_index(answers, i);
		GtkWidget *radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(answer_radio), answer->text);
		gtk_widget_set_size_request(radio, 400, -1);
		gtk_label_set_line_wrap(GTK_LABEL(gtk_bin_get_child(GTK_BIN(radio))), TRUE);
		gtk_box_pack_start(GTK_BOX(answers_box), radio, FALSE, TRUE, 0);
		g_object_set_data(G_OBJECT(radio), "pdd-answer", (gpointer)(answer->is_correct ? radio : NULL));
		if (answer->is_correct)
		{
			statistics->correct_index = i;
		}
		if (answer_radio == NULL)
		{
			gtk_widget_grab_focus(radio);
		}
		answer_radio = radio;
	}
	answer_free_all(answers);
	gtk_widget_show_all(GTK_WIDGET(answers_box));

	GtkFrame *image_frame = GTK_FRAME(gtk_builder_get_object(builder, "frm_image"));
	GtkImage *question_image = GTK_IMAGE(gtk_builder_get_object(builder, "img_question"));
	if (question->image_id != 0)
	{
		gtk_widget_show_all(GTK_WIDGET(image_frame));
		GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
		pdd_image_t *image = image_find_by_id(question->image_id);
		GError *err = NULL;
		if (!gdk_pixbuf_loader_write(loader, image->data, image->data_length, &err))
		{
			g_error("%s\n", err->message);
		}
		if (!gdk_pixbuf_loader_close(loader, &err))
		{
			g_error("%s\n", err->message);
		}
		image_free(image);
		GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
		pixbuf = gdk_pixbuf_scale_simple(pixbuf, 350, 350 * gdk_pixbuf_get_height(pixbuf) / gdk_pixbuf_get_width(pixbuf), GDK_INTERP_BILINEAR);
		g_object_unref(loader);
		gtk_image_set_from_pixbuf(question_image, pixbuf);
		g_object_unref(pixbuf);
	}
	else
	{
		gtk_widget_hide_all(GTK_WIDGET(image_frame));
		gtk_image_set_from_pixbuf(question_image, NULL);
	}
}

void fetch_next_question(statistics_t *statistics, GtkWindow *window)
{
	gint old_index = statistics->index;
	statistics->index = (statistics->index + 1) % statistics->questions->len;
	statistics->try_count = 0;

	while (statistics->index != old_index &&
		g_array_index(statistics->states, gint8, statistics->index) != UnknownState &&
		g_array_index(statistics->states, gint8, statistics->index) != SkippedState)
	{
		statistics->index = (statistics->index + 1) % statistics->questions->len;
	}

	if (statistics->index == old_index)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(window), FALSE);

		gsize i;
		gint count[4] = {0, 0, 0, 0};
		for (i = 0; i < statistics->states->len; i++)
		{
			count[g_array_index(statistics->states, gint8, i)]++;
		}

		gchar *count_text = g_strdup_printf("Правильно: %d\nНеправильно: %d", count[CorrectState], count[IncorrectState]);
		GtkWidget *results_dialog = gtk_message_dialog_new_with_markup(window, GTK_DIALOG_MODAL,
			GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "<span size='large' weight='bold'>%s</span>",
			count[IncorrectState] > 1 ? "Тест не пройден" : "Тест пройден");
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(results_dialog), "%s", count_text);
		g_free(count_text);
		gtk_window_set_title(GTK_WINDOW(results_dialog), "Результат");
		gtk_dialog_run(GTK_DIALOG(results_dialog));
		gtk_widget_destroy(results_dialog);

		gtk_widget_destroy(GTK_WIDGET(window));
		return;
	}

	g_array_index(statistics->states, gint8, statistics->index) = UnknownState;
	update_question(statistics, window);
}

void on_question_real_quit(GtkWindow *window)
{
	statistics_t *statistics = g_object_get_data(G_OBJECT(window), "pdd-statistics");
	gboolean is_exam = statistics->is_exam;
	switch (questions_type)
	{
	case 0:
		on_training_section();
		break;
	case 1:
		if (is_exam)
		{
			on_exam_topic();
		}
		else
		{
			on_training_topic();
		}
		break;
	case 2:
		if (is_exam)
		{
			on_exam_ticket();
		}
		else
		{
			on_training_ticket();
		}
		break;
	case 3:
		gtk_widget_show(main_window);
		break;
	}
}

void on_question_quit(gpointer unused, GtkWindow *window)
{
	gtk_widget_destroy(GTK_WIDGET(window));
}

void on_question_answer(gpointer unused, GtkWindow *window)
{
	GtkBuilder *builder = GTK_BUILDER(g_object_get_data(G_OBJECT(window), "pdd-builder"));
	statistics_t *statistics = g_object_get_data(G_OBJECT(window), "pdd-statistics");
	GtkVBox *answers_box = GTK_VBOX(gtk_builder_get_object(builder, "box_answers"));
	GList *answer_radios = gtk_container_get_children(GTK_CONTAINER(answers_box));
	GtkRadioButton *answer_radio = NULL;
	GList *ar = answer_radios;
	do
	{
		answer_radio = (GtkRadioButton *)ar->data;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(answer_radio)))
		{
			break;
		}
	} while ((ar = g_list_next(ar)) != NULL);
	g_list_free(answer_radios);
	if (g_object_get_data(G_OBJECT(answer_radio), "pdd-answer") == answer_radio)
	{
		g_array_index(statistics->states, gint8, statistics->index) = CorrectState;
	}
	else
	{
		GtkLabel *label = GTK_LABEL(gtk_bin_get_child(GTK_BIN(answer_radio)));
		gchar *new_text = g_strdup_printf("<span underline='error' underline_color='red'>%s</span>", gtk_label_get_text(label));
		gtk_label_set_markup(label, new_text);
		g_free(new_text);
		statistics->try_count++;
		if (statistics->is_exam || statistics->try_count == 2)
		{
			g_array_index(statistics->states, gint8, statistics->index) = IncorrectState;
			if (!statistics->is_exam)
			{
				GtkWidget *error_dialog = gtk_message_dialog_new_with_markup(window, GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
					"<span size='large' weight='bold'>Правильный ответ: %d</span>", statistics->correct_index + 1);
				gchar *advice = ((pdd_question_t *)g_ptr_array_index(statistics->questions, statistics->index))->advice;
				if (advice)
				{
					gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), "%s", advice);
				}
				gtk_window_set_title(GTK_WINDOW(error_dialog), "Неправильный ответ");
				gtk_dialog_run(GTK_DIALOG(error_dialog));
				gtk_widget_destroy(error_dialog);
			}
		}
	}
	if (g_array_index(statistics->states, gint8, statistics->index) != UnknownState)
	{
		fetch_next_question(statistics, window);
	}
}

void on_question_skip(gpointer unused, GtkWindow *window)
{
	statistics_t *statistics = g_object_get_data(G_OBJECT(window), "pdd-statistics");
	g_array_index(statistics->states, gint8, statistics->index) = SkippedState;
	fetch_next_question(statistics, window);
}

void on_question_show_traffregs(gpointer unused, GtkWindow *window)
{
	statistics_t *statistics = g_object_get_data(G_OBJECT(window), "pdd-statistics");
	if (statistics->is_exam)
	{
		return;
	}
	g_print("on_question_show_traffregs (%p)\n", window);
}

void on_question_show_comment(gpointer unused, GtkWindow *window)
{
	statistics_t *statistics = g_object_get_data(G_OBJECT(window), "pdd-statistics");
	if (statistics->is_exam)
	{
		return;
	}
	g_print("on_question_show_comment (%p)\n", window);
}
