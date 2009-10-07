#ifndef IMAGE_H
#define IMAGE_H

#include <glib.h>

typedef struct pdd_image_s
{
	gint64 id;
	gchar *name;
	gpointer data;
	gsize data_length;
} pdd_image_t;

typedef GPtrArray pdd_images_t;

pdd_image_t *image_new(const gchar *name, gconstpointer data, gsize data_length);
void image_free(pdd_image_t *image);

gboolean image_save(pdd_image_t *image);

pdd_image_t *image_find_by_id(gint64 id);
pdd_image_t *image_find_by_name(const gchar *name);

pdd_images_t *image_copy_all(pdd_images_t *images);
void image_free_all(pdd_images_t *images);

#endif // IMAGE_H
