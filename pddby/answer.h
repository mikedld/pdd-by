#ifndef ANSWER_H
#define ANSWER_H

#include "comment.h"
#include "image.h"
#include "section.h"

#include <glib.h>

typedef struct pdd_answer_s
{
    gint64 id;
    gint64 question_id;
    gchar *text;
    gboolean is_correct;
} pdd_answer_t;

typedef GPtrArray pdd_answers_t;

pdd_answer_t *answer_new(gint64 question_id, const gchar *text, gboolean is_correct);
void answer_free(pdd_answer_t *answer);

gboolean answer_save(pdd_answer_t *answer);

pdd_answer_t *answer_find_by_id(gint64 id);

pdd_answers_t *answer_find_by_question(gint64 question_id);
void answer_free_all(pdd_answers_t *answers);

#endif // ANSWER_H
