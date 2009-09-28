#ifndef SECTION_H
#define SECTION_H

#include <gtk/gtk.h>

typedef struct pdd_section_s
{
	gint64 id;
	gchar *name;
	gchar *title_prefix;
	gchar *title;
} pdd_section_t;

typedef GPtrArray pdd_sections_t;

pdd_section_t *section_new(const gchar *name, const gchar *title_prefix, const gchar *title);
void section_free(pdd_section_t *section);

gboolean section_save(pdd_section_t *section);

pdd_section_t *section_find_by_id(gint64 id);
pdd_section_t *section_find_by_name(const gchar *name);

pdd_sections_t *section_find_all();
void section_free_all(pdd_sections_t *sections);

gint32 section_get_question_count(pdd_section_t *section);

#endif // SECTION_H
