#ifndef CONFIG_H
#define CONFIG_H

#ifndef PDD_SHARE_DIR
#define PDD_SHARE_DIR "."
#endif

extern gboolean use_cache;

typedef struct id_pointer_s
{
	gint64 id;
	gpointer ptr;
} id_pointer_t;

#endif // CONFIG_H
