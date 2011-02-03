#ifndef TRAFFREG_H
#define TRAFFREG_H

#include "image.h"

#include <glib.h>

typedef struct pdd_traffreg_s
{
    gint64 id;
    gint32 number;
    gchar *text;
} pdd_traffreg_t;

typedef GPtrArray pdd_traffregs_t;

GHashTable *get_traffregs_images();

pdd_traffreg_t *traffreg_new(gint32 number, const gchar *text);
void traffreg_free(pdd_traffreg_t *traffreg);

gboolean traffreg_save(pdd_traffreg_t *traffreg);

gboolean traffreg_set_images(pdd_traffreg_t *traffreg, pdd_images_t *images);

pdd_traffreg_t *traffreg_find_by_id(gint64 id);
pdd_traffreg_t *traffreg_find_by_number(gint32 number);

pdd_traffregs_t *traffreg_find_by_question(gint64 question_id);
pdd_traffregs_t *traffreg_copy_all(pdd_traffregs_t *traffregs);
void traffreg_free_all(pdd_traffregs_t *traffregs);

#endif // TRAFFREG_H
