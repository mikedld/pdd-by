#include "decode_image.h"

#include "image.h"
#include "private/util/aux.h"
#include "private/util/delphi.h"
#include "private/util/report.h"
#include "private/util/string.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct bpftcam_context
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

typedef struct bpftcam_context bpftcam_context_t;

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

static int pddby_init_randseed_for_image(char const* name, uint16_t magic)
{
    assert(name);

    uint16_t rand_seed = magic;
    for (size_t i = 0; name[i]; i++)
    {
        uint8_t ch = toupper(name[i]);
        for (size_t j = 0; j < 8; j++)
        {
            uint64_t const old_seed = rand_seed;
            rand_seed >>= 1;
            if ((ch ^ old_seed) & 1)
            {
                // TODO: magic number?
                rand_seed ^= 0x0a001;
            }
            ch >>= 1;
        }
    }

    pddby_delphi_set_randseed(rand_seed);
    return 1;
}

static pddby_image_t* pddby_decode_image_a8(pddby_t* pddby, char* basename, uint16_t magic, char* data,
    size_t data_size)
{
    // v9 image format

    struct __attribute__((packed)) data_header
    {
        // file header
        uint16_t signature;
        uint32_t file_size;
        uint16_t reserved[2];
        uint32_t bitmap_offset;
        // information header
        uint32_t header_size;
        uint32_t image_width;
        uint32_t image_height;
        uint16_t planes;
        uint16_t bpp;
        // ... not important
    } *header = (struct data_header *)data;

    header->signature = 0x4d42; // 'BM'

    uint32_t seed = 0;
    for (size_t i = 0; basename[i]; i++)
    {
        if (isdigit(basename[i]))
        {
            seed = seed * 10 + (basename[i] - '0');
        }
    }

    pddby_delphi_set_randseed(seed + magic);

    for (size_t i = header->image_height; i > 0; i--)
    {
        char *scanline = &data[header->bitmap_offset + (i - 1) * ((header->image_width + 1) / 2)];
        for (size_t j = 0; j < (header->image_width + 1) / 2; j++)
        {
            assert(&scanline[j] < data + header->file_size);
            scanline[j] ^= pddby_delphi_random(255);
        }
    }

    return pddby_image_new(pddby, pddby_string_delimit(basename, ".", '\0'), data, data_size);
}

static pddby_image_t* pddby_decode_image_bpft(pddby_t* pddby, char* basename, uint16_t magic, char* data,
    size_t data_size)
{
    // v10 & v11 image format

    if (!pddby_init_randseed_for_image(basename, magic))
    {
        return NULL;
    }

    for (size_t i = 4; i < data_size; i++)
    {
        data[i] ^= pddby_delphi_random(255);
    }

    return pddby_image_new(pddby, pddby_string_delimit(basename, ".", '\0'), data + 4, data_size - 4);
}

static int pddby_decode_image_bpftcam_init(bpftcam_context_t* ctx, char const* basename, uint16_t magic)
{
    if (!pddby_init_randseed_for_image(basename, magic))
    {
        return 0;
    }

    ctx->a[0] = pddby_delphi_get_randseed();
    // initializes both `a` and `x`
    for (size_t i = 1; i < 12; i++)
    {
        ctx->a[i] = ctx->a[i - 1] * 69069 + 1;
    }

    ctx->c1 = 0;
    ctx->c2 = 0;
    ctx->c3 = 0;
    for (size_t i = 0; i < 4; i++)
    {
        ctx->i1[i] = ror((ctx->a[i] >> 5) ^ (ctx->a[i] * 17), i + 1) % 3;
        ctx->c1 = ctx->a[i] + ((ctx->c1 * (ctx->c1 + 1)) >> 1);
    }
    for (size_t i = 4; i < 8; i++)
    {
        ctx->i2[7 - i] = rol((ctx->a[i] >> 5) ^ (ctx->a[i] * 17), i + 1) % 3;
        ctx->c2 = ctx->a[i] + ((ctx->c2 * (ctx->c2 + 1)) >> 1);
    }
    for (size_t i = 0; i < 8; i++)
    {
        ctx->r[i] = (rol((i + 1) ^ ctx->a[i], 8 - i) % 0x1f) + 1;
        ctx->c3 += (i + 1) * ctx->a[i];
    }
    ctx->c1 = (ctx->c1 ^ (ctx->c1 >> 16)) & 3;
    ctx->c2 = (ctx->c2 ^ (ctx->c2 >> 16)) & 3;
    ctx->c3 = (ctx->c3 ^ (ctx->c3 >> 16)) & 3;

    return 1;
}

static uint8_t pddby_decode_image_bpftcam_next(bpftcam_context_t* ctx)
{
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

static pddby_image_t* pddby_decode_image_bpftcam(pddby_t* pddby, char* basename, uint16_t magic, char* data, size_t data_size)
{
    // v12 image format

    bpftcam_context_t context;
    if (!pddby_decode_image_bpftcam_init(&context, basename, magic))
    {
        return NULL;
    }

    for (size_t i = 7; i < data_size; i++)
    {
        data[i] ^= pddby_decode_image_bpftcam_next(&context);
    }

    return pddby_image_new(pddby, pddby_string_delimit(basename, ".", '\0'), data + 7, data_size - 7);
}

int pddby_decode_image(pddby_t* pddby, char const* path, uint16_t magic)
{
    char* data = NULL;
    size_t data_size;
    pddby_image_t* image = NULL;
    char* basename = NULL;

    if (!pddby_aux_file_get_contents(pddby, path, &data, &data_size))
    {
        goto error;
    }

    basename = pddby_aux_path_get_basename(pddby, path);
    if (!basename)
    {
        goto error;
    }

    if (!strncmp(data, "A8", 2))
    {
        image = pddby_decode_image_a8(pddby, basename, magic, data, data_size);
    }
    else if (!strncmp(data, "BPFTCAM", 7))
    {
        image = pddby_decode_image_bpftcam(pddby, basename, magic, data, data_size);
    }
    else if (!strncmp(data, "BPFT", 4))
    {
        image = pddby_decode_image_bpft(pddby, basename, magic, data, data_size);
    }
    else
    {
        pddby_report(pddby, pddby_message_type_error, "unknown image format: %s", path);
        goto error;
    }

    free(data);
    data = NULL;
    free(basename);
    basename = NULL;

    if (!image)
    {
        goto error;
    }

    if (!pddby_image_save(image))
    {
        goto error;
    }

    pddby_image_free(image);
    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode image: %s", path);

    if (data)
    {
        free(data);
    }
    if (basename)
    {
        free(basename);
    }
    if (image)
    {
        pddby_image_free(image);
    }

    return 0;
}
