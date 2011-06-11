#include "decode_context.h"

#include "decode_questions.h"
#include "rolling_stones.h"

#include "private/util/aux.h"
#include "private/util/delphi.h"
#include "private/util/report.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static int pddby_decode_init_magic(pddby_decode_context_t* context);

static char* pddby_decode_string(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file);
static char* pddby_decode_string_v12(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file);
static char* pddby_decode_string_v13(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file);
static char* pddby_decode_string_v13_v2(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file);
static char* pddby_decode_question_string_v13(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file);

pddby_decode_context_t* pddby_decode_context_new(pddby_t* pddby, char const* root_path)
{
    pddby_decode_context_t* context = calloc(1, sizeof(pddby_decode_context_t));
    if (!context)
    {
        goto error;
    }

    context->iconv = pddby_iconv_new(pddby, "cp1251", "utf-8");
    if (!context->iconv)
    {
        goto error;
    }

    context->root_path = root_path;
    context->pddby = pddby;

    if (!pddby_decode_init_magic(context))
    {
        goto error;
    }

    return context;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create decode context");

    if (context)
    {
        pddby_decode_context_free(context);
    }

    return NULL;
}

void pddby_decode_context_free(pddby_decode_context_t* context)
{
    assert(context);

    if (context->iconv)
    {
        pddby_iconv_free(context->iconv);
    }
    free(context);
}

static int pddby_decode_init_magic_2006(pddby_decode_context_t* context)
{
    FILE* f = NULL;
    uint8_t* buffer = NULL;

    char* pdd32_path = pddby_aux_build_filename_ci(context->pddby, context->root_path, "pdd32.exe", NULL);
    if (!pdd32_path)
    {
        goto error;
    }

    f = fopen(pdd32_path, "rb");
    free(pdd32_path);
    if (!f)
    {
        goto error;
    }

    buffer = malloc(16 * 1024 + 1);
    if (!buffer)
    {
        goto error;
    }
    if (fseek(f, 16 * 1024 + 1, SEEK_SET) == -1)
    {
        goto error;
    }
    if (fread(buffer, 16 * 1024 + 1, 1, f) != 1)
    {
        goto error;
    }

    context->magic = 0;
    pddby_delphi_set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    for (int i = 0; i < 255; i++)
    {
        context->magic ^= buffer[pddby_delphi_random(16 * 1024)];
    }
    context->magic = context->magic * buffer[16 * 1024] + 0x1998;

    context->simple_table_magic = context->magic;
    context->image_magic = context->magic;
    context->part_magic = context->magic;
    context->table_magic = context->magic;
    context->decode_string = pddby_decode_string;
    context->decode_question_string = pddby_decode_string;
    context->decode_topic_questions_table = pddby_decode_topic_questions_table;

    free(buffer);
    fclose(f);
    return 1;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number (2006)");

    if (buffer)
    {
        free(buffer);
    }
    if (f)
    {
        fclose(f);
    }

    return 0;
}

static int pddby_decode_init_magic_2008(pddby_decode_context_t* context)
{
    FILE* f = NULL;
    uint8_t* buffer = NULL;

    char* pdd32_path = pddby_aux_build_filename_ci(context->pddby, context->root_path, "pdd32.exe", 0);
    if (!pdd32_path)
    {
        goto error;
    }

    f = fopen(pdd32_path, "rb");
    free(pdd32_path);
    if (!f)
    {
        goto error;
    }

    buffer = malloc(32 * 1024);
    if (!buffer)
    {
        goto error;
    }
    if (fseek(f, 32 * 1024, SEEK_SET) == -1)
    {
        goto error;
    }
    if (fread(buffer, 32 * 1024, 1, f) != 1)
    {
        goto error;
    }

    context->magic = 0x2008;
    pddby_delphi_set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    while (!feof(f))
    {
        int length = fread(buffer, 1, 32 * 1024, f);
        if (length <= 0)
        {
            if (feof(f))
            {
                break;
            }
            goto error;
        }
        for (int i = 0; i < 256; i++)
        {
            uint8_t ch = buffer[pddby_delphi_random(length)];
            for (int j = 0; j < 8; j++)
            {
                uint16_t old_magic = context->magic;
                context->magic >>= 1;
                if ((ch ^ old_magic) & 1)
                {
                    // TODO: magic number?
                    context->magic ^= 0x0a001;
                }
                ch >>= 1;
            }
        }
    }

    context->simple_table_magic = context->magic;
    context->image_magic = context->magic;
    context->part_magic = context->magic;
    context->table_magic = context->magic;
    context->decode_string = pddby_decode_string;
    context->decode_question_string = pddby_decode_string;
    context->decode_topic_questions_table = pddby_decode_topic_questions_table;

    free(buffer);
    fclose(f);
    return 1;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number (2008)");

    if (buffer)
    {
        free(buffer);
    }
    if (f)
    {
        fclose(f);
    }

    return 0;
}

static int pddby_decode_init_magic_2010(pddby_decode_context_t* context)
{
    struct checksum_magic
    {
        char const* checksum;
        uint16_t magic;
        uint16_t image_magic;
        pddby_decode_string_func_t decode_string;
    };
    static struct checksum_magic const s_checksums[] =
    {
        // v12
        {"2d8a027c323c8a8688c42fe5ccd57c5d", 0x1e35, 0x1e35 ^ 0x1a80, pddby_decode_string_v12},
        {"53c99a5ef1df3070ad487d4aff4fc400", 0xaeb0, 0xaeb0 ^ 0x1a80, pddby_decode_string_v12},
        {"fa3f431b556b9e2529a79eb649531af6", 0x4184, 0x4184 ^ 0x1a80, pddby_decode_string_v12},
        // v13
        {"7444b8c559cf5a003e1058ece7b267dc", 0x3492, 0x3492 ^ 0x1a80, pddby_decode_string_v13}
    };

    char* checksum = NULL;

    char* pdd32_path = pddby_aux_build_filename_ci(context->pddby, context->root_path, "pdd32.exe", 0);
    if (!pdd32_path)
    {
        goto error;
    }

    checksum = pddby_aux_file_get_checksum(context->pddby, pdd32_path);
    free(pdd32_path);
    if (!checksum)
    {
        goto error;
    }

    int result = 0;

    for (size_t i = 0; i < sizeof(s_checksums) / sizeof(*s_checksums); i++)
    {
        if (!strcmp(s_checksums[i].checksum, checksum))
        {
            context->magic = s_checksums[i].magic;
            context->simple_table_magic = s_checksums[i].magic;
            context->image_magic = s_checksums[i].image_magic;
            context->part_magic = s_checksums[i].magic;
            context->table_magic = s_checksums[i].magic;
            context->decode_string = s_checksums[i].decode_string;
            context->decode_question_string = s_checksums[i].decode_string;
            context->decode_topic_questions_table = pddby_decode_topic_questions_table;
            result = 1;
            break;
        }
    }

    free(checksum);
    return result;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number (2010)");

    if (checksum)
    {
        free(checksum);
    }

    return 0;
}

static int pddby_decode_init_magic_2011(pddby_decode_context_t* context)
{
    struct checksum_magic
    {
        char const* checksum;
        uint16_t magic;
        uint16_t simple_table_magic;
        uint16_t image_magic;
        uint32_t part_magic;
        uint32_t table_magic;
        pddby_decode_string_func_t decode_string;
        pddby_decode_string_func_t decode_question_string;
        pddby_decode_topic_questions_table_func_t decode_topic_questions_table;
    };
    static struct checksum_magic const s_checksums[] =
    {
        // v13
        {"9d0f0e9c9af0919c34675fe7d2229f49", 0x9af8, 0x9af8 ^ 0x1104, 0x9af8, 0x3b9e7933, 0x3b9e7933 ^ 0x9802,
            pddby_decode_string_v13_v2, pddby_decode_question_string_v13, pddby_decode_topic_questions_table_v13}
    };

    char* checksum = NULL;

    char* pdd32_path = pddby_aux_build_filename_ci(context->pddby, context->root_path, "pdd32.exe", 0);
    if (!pdd32_path)
    {
        goto error;
    }

    checksum = pddby_aux_file_get_checksum(context->pddby, pdd32_path);
    free(pdd32_path);
    if (!checksum)
    {
        goto error;
    }

    int result = 0;

    for (size_t i = 0; i < sizeof(s_checksums) / sizeof(*s_checksums); i++)
    {
        if (!strcmp(s_checksums[i].checksum, checksum))
        {
            context->magic = s_checksums[i].magic;
            context->simple_table_magic = s_checksums[i].simple_table_magic;
            context->image_magic = s_checksums[i].image_magic;
            context->part_magic = s_checksums[i].part_magic;
            context->table_magic = s_checksums[i].table_magic;
            context->decode_string = s_checksums[i].decode_string;
            context->decode_question_string = s_checksums[i].decode_question_string;
            context->decode_topic_questions_table = s_checksums[i].decode_topic_questions_table;
            result = 1;
            break;
        }
    }

    free(checksum);
    return result;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number (2011)");

    if (checksum)
    {
        free(checksum);
    }

    return 0;
}

static int pddby_decode_init_magic(pddby_decode_context_t* context)
{
    struct stat root_stat;
    if (stat(context->root_path, &root_stat) == -1)
    {
        goto error;
    }

    struct tm root_tm = *gmtime(&root_stat.st_mtime);

    int result = 0;
    // TODO: better checks
    switch (1900 + root_tm.tm_year)
    {
    case 2006: // v9
    case 2007:
        result = pddby_decode_init_magic_2006(context);
        break;
    case 2008: // v10 & v11
    case 2009:
        result = pddby_decode_init_magic_2008(context);
        break;
    default:   // v12 & v13
        result = pddby_decode_init_magic_2010(context);
        if (!result)
        {
            result = pddby_decode_init_magic_2011(context);
        }
        break;
    }
    if (!result)
    {
        goto error;
    }

    pddby_report(context->pddby, pddby_message_type_log, "magic numbers: 0x%04x / 0x%04x / 0x%04x / 0x%08x / 0x%08x",
        context->magic, context->simple_table_magic, context->image_magic, context->part_magic, context->table_magic);
    return 1;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number");
    return 0;
}

static char* pddby_decode_string(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file)
{
    char* result = malloc(*buffer_size);
    if (!result)
    {
        return 0;
    }

    for (size_t i = 0, j = pos_in_file; i < *buffer_size; i++, j++)
    {
        // TODO: magic numbers?
        result[i] = buffer[i] ^ (context->magic & 0x0ff) ^ topic_number ^ (j & 1 ? 0x30 : 0x16) ^ ((j + 1) % 255);
    }

    return result;
}

static char* pddby_decode_string_v12(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file)
{
    char* result = malloc(*buffer_size);
    if (!result)
    {
        return 0;
    }

    for (size_t i = 0, j = pos_in_file; i < *buffer_size; i++, j++)
    {
        result[i] = buffer[i] ^ (context->magic >> 8) ^ (j & 1 ? topic_number : 0) ^ (j & 1 ? 0x80 : 0xaa) ^ ((j + 1) % 255);
    }

    return result;
}

static char* pddby_decode_string_v13(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file)
{
    char* result = malloc(*buffer_size);
    if (!result)
    {
        return 0;
    }

    for (size_t i = 0, j = pos_in_file; i < *buffer_size; i++, j++)
    {
        result[i] = buffer[i] ^ (context->magic >> 8) ^ (j & 1 ? topic_number : 0) ^ (j & 1 ? 0x13 : 0x11) ^ ((j + 1) % 255);
    }

    return result;
}


static char* pddby_decode_string_v13_v2(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file)
{
    char* result = malloc(*buffer_size);
    if (!result)
    {
        return 0;
    }

    pddby_rolling_rocks_t* rocks = pddby_rolling_rocks_new(pos_in_file);
    if (!rocks)
    {
        free(result);
        return 0;
    }

    for (size_t i = 0, j = pos_in_file; i < *buffer_size; i++, j++)
    {
        result[i] = buffer[i] ^ context->magic ^ pddby_rolling_rocks_next(rocks) ^ (j & 1 ? topic_number : 0);
    }

    pddby_rolling_rocks_free(rocks);

    return result;
}

static char* pddby_decode_question_string_v13(pddby_decode_context_t* context, char const* buffer, size_t* buffer_size, int8_t topic_number, off_t pos_in_file)
{
    char* result = malloc(*buffer_size);
    if (!result)
    {
        return 0;
    }

    uint32_t seed[3] = { pos_in_file, topic_number, context->part_magic };
    pddby_rolling_stones_t* stones = pddby_rolling_stones_new((uint8_t*)seed, sizeof(seed));
    if (!stones)
    {
        free(result);
        return 0;
    }

    for (size_t i = 0, j = pos_in_file; i < *buffer_size; i++, j++)
    {
        result[i] = buffer[i] ^ pddby_rolling_stones_next(stones, j);
    }

    pddby_rolling_stones_free(stones);

    return result;
}
