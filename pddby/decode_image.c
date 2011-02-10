#include "decode_image.h"

#include "callback.h"
#include "image.h"
#include "util/aux.h"
#include "util/delphi.h"
#include "util/string.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct bpftcam_context_s
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

typedef struct bpftcam_context_s bpftcam_context_t;

static uint32_t rol(uint32_t value, uint8_t shift)
{
    return (value << shift) | (value >> (32 - shift));
}

static uint32_t ror(uint32_t value, uint8_t shift)
{
    return (value >> shift) | (value << (32 - shift));
}

static void swap(uint32_t* left, uint32_t* right)
{
    uint32_t const temp = *left;
    *left = *right;
    *right = temp;
}

static pddby_image_t* decode_image_a8(char* basename, uint16_t magic, char* data, size_t data_size)
{
    // v9 image format

    struct __attribute__((packed)) data_header_s
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
    } *header = (struct data_header_s *)data;

    header->signature = 0x4d42; // 'BM'

    uint32_t seed = 0;
    for (size_t i = 0; basename[i]; i++)
    {
        if (isdigit(basename[i]))
        {
            seed = seed * 10 + (basename[i] - '0');
        }
    }

    set_randseed(seed + magic);

    for (size_t i = header->image_height; i > 0; i--)
    {
        char *scanline = &data[header->bitmap_offset + (i - 1) * ((header->image_width + 1) / 2)];
        for (size_t j = 0; j < (header->image_width + 1) / 2; j++)
        {
            assert(&scanline[j] < data + header->file_size);
            scanline[j] ^= delphi_random(255);
        }
    }

    return pddby_image_new(pddby_string_delimit(basename, ".", '\0'), data, data_size);
}

static pddby_image_t* decode_image_bpft(char* basename, uint16_t magic, char* data, size_t data_size)
{
    // v10 & v11 image format

    init_randseed_for_image(basename, magic);

    for (size_t i = 4; i < data_size; i++)
    {
        data[i] ^= delphi_random(255);
    }

    return pddby_image_new(pddby_string_delimit(basename, ".", '\0'), data + 4, data_size - 4);
}

static void decode_image_bpftcam_init(bpftcam_context_t* ctx, char const* basename, uint16_t magic)
{
    init_randseed_for_image(basename, magic);

    ctx->a[0] = get_randseed();
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
}

static uint8_t decode_image_bpftcam_next(bpftcam_context_t* ctx)
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

static pddby_image_t* decode_image_bpftcam(char* basename, uint16_t magic, char* data, size_t data_size)
{
    // v12 image format

    bpftcam_context_t context;
    decode_image_bpftcam_init(&context, basename, magic);

    for (size_t i = 7; i < data_size; i++)
    {
        data[i] ^= decode_image_bpftcam_next(&context);
    }

    return pddby_image_new(pddby_string_delimit(basename, ".", '\0'), data + 7, data_size - 7);
}

int decode_image(char const* path, uint16_t magic)
{
    char* data;
    size_t data_size;

    if (!pddby_aux_file_get_contents(path, &data, &data_size))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    pddby_image_t* image = 0;
    char* basename = pddby_aux_path_get_basename(path);

    if (!strncmp(data, "A8", 2))
    {
        image = decode_image_a8(basename, magic, data, data_size);
    }
    else if (!strncmp(data, "BPFTCAM", 7))
    {
        image = decode_image_bpftcam(basename, magic, data, data_size);
    }
    else if (!strncmp(data, "BPFT", 4))
    {
        image = decode_image_bpft(basename, magic, data, data_size);
    }

    free(data);
    free(basename);

    if (image)
    {
        int result = pddby_image_save(image);
        pddby_image_free(image);

        if (!result)
        {
            pddby_report_error("unable to save image");
        }
    }
    else
    {
        pddby_report_error("unknown image format\n");
    }

    return 1;
}
