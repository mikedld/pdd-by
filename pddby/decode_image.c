#include "decode_image.h"
#include "delphi_helper.h"
#include "image.h"

#include <ctype.h>
#include <string.h>

typedef struct bpftcam_context_s
{
    guint32 c1;
    guint32 c2;
    guint32 c3;
    guint32 i1[4];
    guint32 i2[4];
    guint32 r[8];
    guint32 a[8];
    guint32 x[4];
} bpftcam_context_t;

static guint32 rol(guint32 value, guint8 shift)
{
    return (value << shift) | (value >> (32 - shift));
}

static guint32 ror(guint32 value, guint8 shift)
{
    return (value >> shift) | (value << (32 - shift));
}

static void swap(guint32* left, guint32* right)
{
    guint32 const temp = *left;
    *left = *right;
    *right = temp;
}

static pdd_image_t* decode_image_a8(gchar* basename, guint16 magic, gchar* data, gsize data_size)
{
    // v9 image format

    struct __attribute__((packed)) data_header_s
    {
        // file header
        guint16 signature;
        guint32 file_size;
        guint16 reserved[2];
        guint32 bitmap_offset;
        // information header
        guint32 header_size;
        guint32 image_width;
        guint32 image_height;
        guint16 planes;
        guint16 bpp;
        // ... not important
    } *header = (struct data_header_s *)data;

    header->signature = 0x4d42; // 'BM'

    guint32 seed = 0;
    for (gsize i = 0; basename[i]; i++)
    {
        if (isdigit(basename[i]))
        {
            seed = seed * 10 + (basename[i] - '0');
        }
    }

    set_randseed(seed + magic);

    for (gsize i = header->image_height; i > 0; i--)
    {
        gchar *scanline = &data[header->bitmap_offset + (i - 1) * ((header->image_width + 1) / 2)];
        for (gsize j = 0; j < (header->image_width + 1) / 2; j++)
        {
            g_assert(&scanline[j] < data + header->file_size);
            scanline[j] ^= delphi_random(255);
        }
    }

    return image_new(g_strdelimit(basename, ".", '\0'), data, data_size);
}

static pdd_image_t* decode_image_bpft(gchar* basename, guint16 magic, gchar* data, gsize data_size)
{
    // v10 & v11 image format

    init_randseed_for_image(basename, magic);

    for (gsize i = 4; i < data_size; i++)
    {
        data[i] ^= delphi_random(255);
    }

    return image_new(g_strdelimit(basename, ".", '\0'), data + 4, data_size - 4);
}

static void decode_image_bpftcam_init(bpftcam_context_t* ctx, gchar const* basename, guint16 magic)
{
    init_randseed_for_image(basename, magic);

    ctx->a[0] = get_randseed();
    // initializes both `a` and `x`
    for (gsize i = 1; i < 12; i++)
    {
        ctx->a[i] = ctx->a[i - 1] * 69069 + 1;
    }

    ctx->c1 = 0;
    ctx->c2 = 0;
    ctx->c3 = 0;
    for (gsize i = 0; i < 4; i++)
    {
        ctx->i1[i] = ror((ctx->a[i] >> 5) ^ (ctx->a[i] * 17), i + 1) % 3;
        ctx->c1 = ctx->a[i] + ((ctx->c1 * (ctx->c1 + 1)) >> 1);
    }
    for (gsize i = 4; i < 8; i++)
    {
        ctx->i2[7 - i] = rol((ctx->a[i] >> 5) ^ (ctx->a[i] * 17), i + 1) % 3;
        ctx->c2 = ctx->a[i] + ((ctx->c2 * (ctx->c2 + 1)) >> 1);
    }
    for (gsize i = 0; i < 8; i++)
    {
        ctx->r[i] = (rol((i + 1) ^ ctx->a[i], 8 - i) % 0x1f) + 1;
        ctx->c3 += (i + 1) * ctx->a[i];
    }
    ctx->c1 = (ctx->c1 ^ (ctx->c1 >> 16)) & 3;
    ctx->c2 = (ctx->c2 ^ (ctx->c2 >> 16)) & 3;
    ctx->c3 = (ctx->c3 ^ (ctx->c3 >> 16)) & 3;
}

guint8 decode_image_bpftcam_next(bpftcam_context_t* ctx)
{
    for (gsize i = 0; i < 4; i++)
    {
        guint32 j = ctx->i1[i];
        guint32 k = ctx->i2[i];
        for (gsize l = 0; l < 4; l++)
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

static pdd_image_t* decode_image_bpftcam(gchar* basename, guint16 magic, gchar* data, gsize data_size)
{
    // v12 image format

    bpftcam_context_t context;
    decode_image_bpftcam_init(&context, basename, magic);

    for (gsize i = 7; i < data_size; i++)
    {
        data[i] ^= decode_image_bpftcam_next(&context);
    }

    return image_new(g_strdelimit(basename, ".", '\0'), data + 7, data_size - 7);
}

gboolean decode_image(const gchar *path, guint16 magic)
{
    GError *err;
    gchar *data;
    gsize data_size;

    if (!g_file_get_contents(path, &data, &data_size, &err))
    {
        g_error("%s\n", err->message);
    }

    pdd_image_t *image = NULL;
    gchar *basename = g_path_get_basename(path);

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

    g_free(data);
    g_free(basename);

    if (image)
    {
        gboolean result = image_save(image);
        image_free(image);

        if (!result)
        {
            g_error("unable to save image");
        }
    }
    else
    {
        g_error("unknown image format\n");
    }

    return TRUE;
}
