#ifndef QUESTION_H
#define QUESTION_H

#include "section.h"
#include "traffreg.h"

#include <glib.h>

typedef struct pdd_question_s
{
	gint64 id;
	gint64 topic_id;
	gchar *text;
	gint64 image_id;
	gchar *advice;
	gint64 comment_id;
} pdd_question_t;

typedef GPtrArray pdd_questions_t;

pdd_question_t *question_new(gint64 topic_id, const gchar *text, gint64 image_id, const gchar *advice, gint64 comment_id);
void question_free(pdd_question_t *question);

gboolean question_save(pdd_question_t *question);

gboolean question_set_sections(pdd_question_t *question, pdd_sections_t *sections);
gboolean question_set_traffregs(pdd_question_t *question, pdd_traffregs_t *traffregs);

pdd_question_t *question_find_by_id(gint64 id);

pdd_questions_t *question_find_by_section(gint64 section_id);
pdd_questions_t *question_find_by_topic(gint64 topic_id, gint ticket_number);
pdd_questions_t *question_find_by_ticket(gint ticket_number);
pdd_questions_t *question_find_random();
void question_free_all(pdd_questions_t *questions);

#endif // QUESTION_H
