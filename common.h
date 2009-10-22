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

#endif // CONFIG_H
