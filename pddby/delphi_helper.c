#include "delphi_helper.h"

guint64 rand_seed = 0xccba8e81;

inline guint64 delphi_random(guint32 limit)
{
    rand_seed = (rand_seed * 0x08088405 + 1) & 0x0ffffffff;
    // gcc seem to complain about `>> 32` on i386 arch
    return (rand_seed * limit) / 0x100000000LL;
}

inline void set_randseed(guint64 seed)
{
    rand_seed = seed;
}

inline guint64 get_randseed()
{
    return rand_seed;
}

void init_randseed_for_image(const gchar *name, guint16 magic)
{
    gchar *name_up = g_utf8_strup(name, -1);
    gsize i, j;

    rand_seed = magic;
    for (i = 0; name_up[i]; i++)
    {
        guchar ch = name_up[i];
        for (j = 0; j < 8; j++)
        {
            guint64 old_seed = rand_seed;
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

    g_free(name_up);
}
