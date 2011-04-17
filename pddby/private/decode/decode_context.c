#include "decode_context.h"

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

static char* pddby_decode_string(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number);
static char* pddby_decode_string_v12(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number);
static char* pddby_decode_string_v13(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number);

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

    context->data_magic = 0;
    pddby_delphi_set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    for (int i = 0; i < 255; i++)
    {
        context->data_magic ^= buffer[pddby_delphi_random(16 * 1024)];
    }
    context->data_magic = context->data_magic * buffer[16 * 1024] + 0x1998;

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

    context->data_magic = 0x2008;
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
                uint16_t old_magic = context->data_magic;
                context->data_magic >>= 1;
                if ((ch ^ old_magic) & 1)
                {
                    // TODO: magic number?
                    context->data_magic ^= 0x0a001;
                }
                ch >>= 1;
            }
        }
    }

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
        context->image_magic = context->data_magic;
        context->decode_string = pddby_decode_string;
        break;
    case 2008: // v10 & v11
    case 2009:
        result = pddby_decode_init_magic_2008(context);
        context->image_magic = context->data_magic;
        context->decode_string = pddby_decode_string;
        break;
    default:   // v12
        {
            struct checksum_magic
            {
                char const* checksum;
                uint16_t data_magic;
                uint16_t image_magic; // == data_magic ^ 0x1a80
                pddby_decode_string_func_t decode_string;
            };
            struct checksum_magic const s_checksums[] =
            {
                // v12
                {"2d8a027c323c8a8688c42fe5ccd57c5d", 0x1e35, 0x04b5, pddby_decode_string_v12},
                {"53c99a5ef1df3070ad487d4aff4fc400", 0xaeb0, 0xb430, pddby_decode_string_v12},
                {"fa3f431b556b9e2529a79eb649531af6", 0x4184, 0x5b04, pddby_decode_string_v12},
                // v13
                {"7444b8c559cf5a003e1058ece7b267dc", 0x3492, 0x2e12, pddby_decode_string_v13}
            };

            char* pdd32_path = pddby_aux_build_filename_ci(context->pddby, context->root_path, "pdd32.exe", 0);
            if (!pdd32_path)
            {
                goto error;
            }
            char* checksum = pddby_aux_file_get_checksum(context->pddby, pdd32_path);
            free(pdd32_path);
            if (!checksum)
            {
                goto error;
            }

            for (size_t i = 0; i < sizeof(s_checksums) / sizeof(*s_checksums); i++)
            {
                if (!strcmp(s_checksums[i].checksum, checksum))
                {
                    context->data_magic = s_checksums[i].data_magic;
                    context->image_magic = s_checksums[i].image_magic;
                    context->decode_string = s_checksums[i].decode_string;
                    result = 1;
                    break;
                }
            }

            free(checksum);
        }
    }
    if (!result)
    {
        goto error;
    }

    pddby_report(context->pddby, pddby_message_type_log, "magic number: 0x%04x", context->data_magic);
    return 1;

error:
    pddby_report(context->pddby, pddby_message_type_error, "unable to initialize magic number");
    return 0;
}

static char* pddby_decode_string(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number)
{
    char* str;
    if (!pddby_aux_file_get_contents(context->pddby, path, &str, str_size))
    {
        pddby_report(context->pddby, pddby_message_type_error, "unable to decode string");
        return NULL;
    }

    for (size_t i = 0; i < *str_size; i++)
    {
        // TODO: magic numbers?
        str[i] ^= (context->data_magic & 0x0ff) ^ topic_number ^ (i & 1 ? 0x30 : 0x16) ^ ((i + 1) % 255);
    }

    return str;
}

static char* pddby_decode_string_v12(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number)
{
    char* str;
    if (!pddby_aux_file_get_contents(context->pddby, path, &str, str_size))
    {
        pddby_report(context->pddby, pddby_message_type_error, "unable to decode string");
        return NULL;
    }

    for (size_t i = 0; i < *str_size; i++)
    {
        str[i] ^= (context->data_magic >> 8) ^ (i & 1 ? topic_number : 0) ^ (i & 1 ? 0x80 : 0xaa) ^ ((i + 1) % 255);
    }

    return str;
}

static char* pddby_decode_string_v13(pddby_decode_context_t* context, char const* path, size_t* str_size, int8_t topic_number)
{
    char* str;
    if (!pddby_aux_file_get_contents(context->pddby, path, &str, str_size))
    {
        pddby_report(context->pddby, pddby_message_type_error, "unable to decode string");
        return NULL;
    }

    for (size_t i = 0; i < *str_size; i++)
    {
        str[i] ^= (context->data_magic >> 8) ^ (i & 1 ? topic_number : 0) ^ (i & 1 ? 0x13 : 0x11) ^ ((i + 1) % 255);
    }

    return str;
}
