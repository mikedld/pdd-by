#ifndef CONFIG_H
#define CONFIG_H

#ifndef PDD_SHARE_DIR
#define PDD_SHARE_DIR "."
#endif

#if __GNUC__ >= 4
#define GNUC_VISIBLE __attribute__((visibility("default")))
#else
#define GNUC_VISIBLE
#endif

extern gboolean use_cache;

typedef struct id_pointer_s
{
	gint64 id;
	gpointer ptr;
} id_pointer_t;

#endif // CONFIG_H
