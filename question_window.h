#ifndef CHOOSERWINDOW_H
#define CHOOSERWINDOW_H

#include "section.h"
#include "topic.h"

#include <gtk/gtk.h>

GtkWidget *question_window_new_with_section(pdd_section_t *section, gboolean is_exam);
GtkWidget *question_window_new_with_topic(pdd_topic_t *topic, gint ticket_number, gboolean is_exam);
GtkWidget *question_window_new_with_ticket(gint ticket_number, gboolean is_exam);
GtkWidget *question_window_new_with_random_ticket(gboolean is_exam);

#endif // CHOOSERWINDOW_H
