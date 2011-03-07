#ifndef PDDBY_GTK_PLATFORM_H
#define PDDBY_GTK_PLATFORM_H

#include "config.h"

#if defined(__GNUC__) && __GNUC__ >= 4
#define GNUC_VISIBLE __attribute__((visibility("default")))
#else
#define GNUC_VISIBLE
#endif

#endif // PDDBY_GTK_PLATFORM_H
