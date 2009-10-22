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

#ifndef sqlite3_prepare_v2
#define sqlite3_prepare_v2 sqlite3_prepare
#endif

#endif // CONFIG_H
