#include "rolling_stones.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_rolling_rocks
{
    uint32_t c1;
    uint32_t c2;
    uint32_t c3;
    uint32_t i1[4];
    uint32_t i2[4];
    uint32_t r[8];
    uint32_t a[8];
    uint32_t x[4];
};

static inline uint32_t rol(uint32_t value, uint8_t shift)
{
    return (value << shift) | (value >> (32 - shift));
}

static inline uint32_t ror(uint32_t value, uint8_t shift)
{
    return (value >> shift) | (value << (32 - shift));
}

static inline void swap(uint32_t* left, uint32_t* right)
{
    uint32_t const temp = *left;
    *left = *right;
    *right = temp;
}

struct pddby_rolling_stones
{
    int32_t seed;
    int8_t magic[4096];
};

static uint8_t pddby_rolling_stones_next_magic(pddby_rolling_stones_t* stones)
{
    uint8_t buf[4];
    for (size_t i = 0; i < sizeof(buf); i++)
    {
        stones->seed = stones->seed * 0x08088405 + 1;
        buf[i] = stones->seed >> 24;
    }
    return buf[0];
}

pddby_rolling_stones_t* pddby_rolling_stones_new(uint8_t* seed, size_t seed_size)
{
    // seed == { part_something, part_number, magic }

    pddby_rolling_stones_t* stones = calloc(1, sizeof(pddby_rolling_stones_t));
    if (!stones)
    {
        return NULL;
    }

    while (seed_size)
    {
        stones->seed = seed[--seed_size] ^ rol(stones->seed, 8);
    }

    for (size_t i = 0; i < sizeof(stones->magic); i++)
    {
        stones->magic[i] = pddby_rolling_stones_next_magic(stones);
    }

    return stones;
}

uint8_t pddby_rolling_stones_next(pddby_rolling_stones_t* ctx, off_t pos)
{
    assert(ctx);

    return ctx->magic[pos & 0x0fff];
}

void pddby_rolling_stones_free(pddby_rolling_stones_t* ctx)
{
    assert(ctx);

    free(ctx);
}

pddby_rolling_rocks_t* pddby_rolling_rocks_new(uint32_t seed)
{
    pddby_rolling_rocks_t* rocks = malloc(sizeof(pddby_rolling_rocks_t));
    if (!rocks)
    {
        return NULL;
    }

    rocks->a[0] = seed;
    for (size_t i = 1; i < 12; i++)
    {
        uint32_t const* prev = i < 9 ? &rocks->a[i - 1] : &rocks->x[i - 9];
        uint32_t* curr = i < 8 ? &rocks->a[i] : &rocks->x[i - 8];
        *curr = *prev * 69069 + 1;
    }

    rocks->c1 = 0;
    rocks->c2 = 0;
    rocks->c3 = 0;
    for (size_t i = 0; i < 4; i++)
    {
        rocks->i1[i] = ror((rocks->a[i] >> 5) ^ (rocks->a[i] * 17), i + 1) % 3;
        rocks->c1 = rocks->a[i] + ((rocks->c1 * (rocks->c1 + 1)) >> 1);
    }
    for (size_t i = 4; i < 8; i++)
    {
        rocks->i2[7 - i] = rol((rocks->a[i] >> 5) ^ (rocks->a[i] * 17), i + 1) % 3;
        rocks->c2 = rocks->a[i] + ((rocks->c2 * (rocks->c2 + 1)) >> 1);
    }
    for (size_t i = 0; i < 8; i++)
    {
        rocks->r[i] = (rol((i + 1) ^ rocks->a[i], 8 - i) % 0x1f) + 1;
        rocks->c3 += (i + 1) * rocks->a[i];
    }
    rocks->c1 = (rocks->c1 ^ (rocks->c1 >> 16)) & 3;
    rocks->c2 = (rocks->c2 ^ (rocks->c2 >> 16)) & 3;
    rocks->c3 = (rocks->c3 ^ (rocks->c3 >> 16)) & 3;

    return rocks;
}

uint8_t pddby_rolling_rocks_next(pddby_rolling_rocks_t* ctx)
{
    assert(ctx);

    for (size_t i = 0; i < 4; i++)
    {
        uint32_t j = ctx->i1[i];
        uint32_t k = ctx->i2[i];
        for (size_t l = 0; l < 4; l++)
        {
            if (j == ctx->c1)
            {
                ctx->x[0] += ctx->a[j] + ctx->x[3] + ror((ctx->x[1] >> 9) ^ (ctx->x[1] * 65), ctx->r[k + j]);
            }
            else
            {
                ctx->x[0] += ctx->x[3] + ror(ctx->a[j] + ((ctx->x[1] >> 9) ^ (ctx->x[1] * 65)), ctx->r[k + j]);
            }
            if (k == ctx->c2)
            {
                ctx->x[1] += ctx->a[j + 1] + ctx->x[2] + ror((ctx->x[0] >> 9) ^ (ctx->x[0] * 65), ctx->r[k + j + 1]);
            }
            else
            {
                ctx->x[1] += ctx->x[2] + ror(ctx->a[j + 1] + ((ctx->x[0] >> 9) ^ (ctx->x[0] * 65)), ctx->r[k + j + 1]);
            }
            if (l == ctx->c3)
            {
                ctx->x[2] += ctx->a[k + 4] + ((ctx->x[0] >> 5) ^ (ctx->x[0] * 17));
                ctx->x[3] += ctx->a[k + 5] + ((ctx->x[1] >> 5) ^ (ctx->x[1] * 17));
            }
            else
            {
                ctx->x[2] ^= ctx->a[k + 4] + ((ctx->x[0] >> 5) ^ (ctx->x[0] * 17));
                ctx->x[3] ^= ctx->a[k + 5] + ((ctx->x[1] >> 5) ^ (ctx->x[1] * 17));
            }
            swap(&ctx->x[0], &ctx->x[2]);
            swap(&ctx->x[1], &ctx->x[3]);
            j = (j + 1) % 3;
            k = (k + 1) % 3;
        }
    }

    return *ctx->x;
}

void pddby_rolling_rocks_free(pddby_rolling_rocks_t* ctx)
{
    assert(ctx);

    free(ctx);
}
