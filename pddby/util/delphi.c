#include "delphi.h"
#include "string.h"

#include <stddef.h>
#include <stdlib.h>

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

void init_randseed_for_image(char const* name, uint16_t magic)
{
    char* name_up = pddby_string_upcase(name);

    rand_seed = magic;
    for (size_t i = 0; name_up[i]; i++)
    {
        uint8_t ch = name_up[i];
        for (size_t j = 0; j < 8; j++)
        {
            uint64_t const old_seed = rand_seed;
            rand_seed >>= 1;
            if ((ch ^ old_seed) & 1)
            {
                // TODO: magic number?
                rand_seed ^= 0x0a001;
            }
            rand_seed &= 0x0ffff;
            ch >>= 1;
        }
    }

    free(name_up);
}
