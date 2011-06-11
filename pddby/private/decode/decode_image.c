#include "decode_image.h"

#include "rolling_stones.h"

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

static pddby_image_t* pddby_decode_image_bpftcam(pddby_t* pddby, char* basename, uint16_t magic, char* data, size_t data_size)
{
    // v12/v13 image format

    if (!pddby_init_randseed_for_image(basename, magic))
    {
        return NULL;
    }

    pddby_rolling_rocks_t* rocks = pddby_rolling_rocks_new(pddby_delphi_get_randseed());
    if (!rocks)
    {
        return NULL;
    }

    for (size_t i = 7; i < data_size; i++)
    {
        data[i] ^= pddby_rolling_rocks_next(rocks);
    }

    pddby_rolling_rocks_free(rocks);

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
