#ifndef COMMENT_H
#define COMMENT_H

#include <glib.h>

typedef struct pdd_comment_s
{
    gint64 id;
    gint32 number;
    gchar *text;
} pdd_comment_t;

typedef GPtrArray pdd_comments_t;

pdd_comment_t *comment_new(gint32 number, const gchar *text);
void comment_free(pdd_comment_t *comment);

gboolean comment_save(pdd_comment_t *comment);

pdd_comment_t *comment_find_by_id(gint64 id);
pdd_comment_t *comment_find_by_number(gint32 number);

#endif // COMMENT_H
