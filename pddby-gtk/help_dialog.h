#ifndef PDDBY_GTK_HELP_DIALOG_H
#define PDDBY_GTK_HELP_DIALOG_H

#include "pddby/question.h"

#include <gtk/gtk.h>

GtkWidget *help_dialog_new_with_comment(const pddby_question_t *question);
GtkWidget *help_dialog_new_with_traffregs(const pddby_question_t *question);

#endif // PDDBY_GTK_HELP_DIALOG_H
