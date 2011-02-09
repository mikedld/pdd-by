#ifndef CHOOSERWINDOW_H
#define CHOOSERWINDOW_H

#include "pddby/section.h"
#include "pddby/topic.h"

#include <gtk/gtk.h>

GtkWidget *question_window_new_with_section(pddby_section_t *section, gboolean is_exam);
GtkWidget *question_window_new_with_topic(pddby_topic_t *topic, gint ticket_number, gboolean is_exam);
GtkWidget *question_window_new_with_ticket(gint ticket_number, gboolean is_exam);
GtkWidget *question_window_new_with_random_ticket(gboolean is_exam);

#endif // CHOOSERWINDOW_H
