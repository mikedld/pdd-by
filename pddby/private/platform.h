#ifndef PDDBY_PRIVATE_PLATFORM_H
#define PDDBY_PRIVATE_PLATFORM_H

#include "config.h"

#ifndef sqlite3_prepare_v2
#define sqlite3_prepare_v2 sqlite3_prepare
#endif

#ifdef PDDBY_BIG_ENDIAN
// generic, could be better
#define PDDBY_INT32_FROM_LE(x) (((x) << 24) | (((x) << 8) & 0x0ff0000) | (((x) >> 8) & 0x0ff00) | (((x) >> 24) & 0x0ff))
#else
#define PDDBY_INT32_FROM_LE(x) (x)
#endif

#endif // PDDBY_PRIVATE_PLATFORM_H
