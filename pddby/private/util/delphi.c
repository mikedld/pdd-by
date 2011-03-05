#include "delphi.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static uint64_t rand_seed = 0xccba8e81;

inline uint32_t delphi_random(uint32_t limit)
{
    rand_seed = (rand_seed * 0x08088405 + 1) & 0x0ffffffff;
    // gcc seem to complain about `>> 32` on i386 arch
    return (rand_seed * limit) / 0x100000000LL;
}

inline void set_randseed(uint64_t seed)
{
    rand_seed = seed;
}

inline uint64_t get_randseed()
{
    return rand_seed;
}
