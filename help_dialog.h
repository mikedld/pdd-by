#ifndef HELP_DIALOG_H
#define HELP_DIALOG_H

#include "question.h"

#include <gtk/gtk.h>

GtkWidget *help_dialog_new_with_comment(pdd_question_t *question);
GtkWidget *help_dialog_new_with_traffregs(pdd_question_t *question);

#endif // HELP_DIALOG_H
