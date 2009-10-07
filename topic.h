#ifndef TOPIC_H
#define TOPIC_H

#include <glib.h>

typedef struct pdd_topic_s
{
	gint64 id;
	gint number;
	gchar *title;
} pdd_topic_t;

typedef GPtrArray pdd_topics_t;

pdd_topic_t *topic_new(gint number, const gchar *title);
void topic_free(pdd_topic_t *topic);

gboolean topic_save(pdd_topic_t *topic);

pdd_topic_t *topic_find_by_id(gint64 id);
pdd_topic_t *topic_find_by_number(gint number);

pdd_topics_t *topic_find_all();
void topic_free_all(pdd_topics_t *topics);

gint32 topic_get_question_count(pdd_topic_t *topic);

#endif // TOPIC_H
