#ifndef PDDBY_GTK_MAIN_WINDOW_H
#define PDDBY_GTK_MAIN_WINDOW_H

#include "pddby/pddby.h"

#include <gtk/gtk.h>

GtkWidget *main_window_new(pddby_t* pddby);

void on_training_section();
void on_training_topic();
void on_training_ticket();
void on_training_random_ticket();
void on_exam_topic();
void on_exam_ticket();
void on_exam_random_ticket();

#endif // PDDBY_GTK_MAIN_WINDOW_H
