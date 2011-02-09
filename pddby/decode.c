#include "decode.h"
#include "answer.h"
#include "callback.h"
#include "comment.h"
#include "config.h"
#include "database.h"
#include "decode.h"
#include "image.h"
#include "question.h"
#include "section.h"
#include "topic.h"
#include "traffreg.h"
#include "util/aux.h"
#include "util/delphi.h"
#include "util/regex.h"
#include "util/settings.h"

#include <ctype.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef struct topic_question_s
{
    int8_t topic_number;
    int32_t question_offset;
} __attribute__((__packed__)) topic_question_t;

typedef char* (*decode_string_func_t)(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number);

typedef struct decode_context_s
{
    char const* root_path;
    uint16_t data_magic;
    uint16_t image_magic;
    decode_string_func_t decode_string;
} decode_context_t;

typedef int (*decode_stage_t) (decode_context_t* context);

static int init_magic(decode_context_t* context);
static int decode_images(decode_context_t* context);
static int decode_comments(decode_context_t* context);
static int decode_traffregs(decode_context_t* context);
static int decode_questions(decode_context_t* context);

static char* decode_string(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number);
static char* decode_string_v12(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number);

static const decode_stage_t decode_stages[] =
{
    &init_magic,
    &decode_images,
    &decode_comments,
    &decode_traffregs,
    &decode_questions,
    0
};

int decode(char const* root_path)
{
    decode_context_t context;
    context.root_path = root_path;

    const decode_stage_t *stage;

    for (stage = decode_stages; *stage; stage++)
    {
        if (!(*stage)(&context))
        {
            return 0;
        }
    }

    return 1;
}

static char* find_file_ci(char const* path, char const* fname)
{
    DIR* dir = opendir(path);
    if (!dir)
    {
        pddby_report_error("");
    }
    char* file_path = 0;
    struct dirent* ent;
    while ((ent = readdir(dir)))
    {
        if (!strcasecmp(ent->d_name, fname))
        {
            file_path = pddby_aux_build_filename(path, ent->d_name, 0);
            break;
        }
    }
    closedir(dir);
    return file_path;
}

static char* make_path(char const* root_path, ...)
{
    char* result = strdup(root_path);
    va_list list;
    va_start(list, root_path);
    char const* path_part;
    while ((path_part = va_arg(list, char const*)))
    {
        char* path = find_file_ci(result, path_part);
        free(result);
        result = path;
        if (!result)
        {
            break;
        }
    }
    va_end(list);
    printf("make_path: %s\n", result);
    return result;
}

static int init_magic_2006(decode_context_t* context)
{
    char* pdd32_path = make_path(context->root_path, "pdd32.exe", 0);
    FILE* f = fopen(pdd32_path, "rb");
    free(pdd32_path);
    if (!f)
    {
        pddby_report_error("pdd32.exe not found");
    }

    uint8_t* buffer = malloc(16 * 1024 + 1);
    fseek(f, 16 * 1024 + 1, SEEK_SET);
    if (fread(buffer, 16 * 1024 + 1, 1, f) != 1)
    {
        pddby_report_error("pdd32.exe invalid");
    }

    context->data_magic = 0;
    set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    for (int i = 0; i < 255; i++)
    {
        context->data_magic ^= buffer[delphi_random(16 * 1024)];
    }
    context->data_magic = context->data_magic * buffer[16 * 1024] + 0x1998;

    free(buffer);
    fclose(f);
    return 1;
}

static int init_magic_2008(decode_context_t* context)
{
    char* pdd32_path = make_path(context->root_path, "pdd32.exe", 0);
    FILE* f = fopen(pdd32_path, "rb");
    free(pdd32_path);
    if (!f)
    {
        pddby_report_error("pdd32.exe not found");
    }

    uint8_t* buffer = malloc(32 * 1024);
    fseek(f, 32 * 1024, SEEK_SET);
    if (fread(buffer, 32 * 1024, 1, f) != 1)
    {
        pddby_report_error("pdd32.exe invalid");
    }

    context->data_magic = 0x2008;
    set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    while (!feof(f))
    {
        int length = fread(buffer, 1, 32 * 1024, f);
        if (length <= 0)
        {
            break;
        }
        for (int i = 0; i < 256; i++)
        {
            uint8_t ch = buffer[delphi_random(length)];
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
}

static char* get_file_checksum(char const* file_path)
{
    FILE* f = fopen(file_path, "rb");
    if (!f)
    {
        pddby_report_error("unable to open file: %s", file_path);
    }

    MD5_CTX md5ctx;
    MD5_Init(&md5ctx);

    uint8_t* buffer = malloc(32 * 1024);

    do
    {
        size_t bytes_read = fread(buffer, 1, 32 * 1024, f);
        if (!bytes_read)
        {
            if (feof(f))
            {
                break;
            }
            pddby_report_error("unable to read file: %s", file_path);
        }
        MD5_Update(&md5ctx, buffer, bytes_read);
    }
    while (!feof(f));

    free(buffer);
    fclose(f);

    uint8_t md5sum[MD5_DIGEST_LENGTH];
    MD5_Final(md5sum, &md5ctx);

    char* result = malloc(MD5_DIGEST_LENGTH * 2 + 1);
    for (size_t i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(result + i * 2, "%02x", md5sum[i]);
    }

    return result;
}

static int init_magic(decode_context_t* context)
{
    struct stat root_stat;
    if (stat(context->root_path, &root_stat) == -1)
    {
        pddby_report_error("unable to stat root directory");
    }

    struct tm root_tm = *gmtime(&root_stat.st_mtime);

    int result = 0;
    // TODO: better checks
    switch (1900 + root_tm.tm_year)
    {
    case 2006: // v9
    case 2007:
        result = init_magic_2006(context);
        context->image_magic = context->data_magic;
        context->decode_string = decode_string;
        break;
    case 2008: // v10 & v11
    case 2009:
        result = init_magic_2008(context);
        context->image_magic = context->data_magic;
        context->decode_string = decode_string;
        break;
    default:   // v12
        {
            struct checksum_magic_s
            {
                char const* checksum;
                uint16_t data_magic;
                uint16_t image_magic; // == data_magic ^ 0x1a80
                decode_string_func_t decode_string;
            };
            struct checksum_magic_s const s_checksums[] =
            {
                // v12
                {"2d8a027c323c8a8688c42fe5ccd57c5d", 0x1e35, 0x04b5, decode_string_v12},
                {"fa3f431b556b9e2529a79eb649531af6", 0x4184, 0x5b04, decode_string_v12},
                // v13 (?)
                {"7444b8c559cf5a003e1058ece7b267dc", 0x3492, 0x2e12, decode_string_v12}
            };

            char* pdd32_path = make_path(context->root_path, "pdd32.exe", 0);
            char* checksum = get_file_checksum(pdd32_path);
            free(pdd32_path);

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
        pddby_report_error("unable to calculate magic number");
    }

    printf("magic: 0x%04x\n", context->data_magic);
    return 1;
}

static int decode_images(decode_context_t* context)
{
    char* raw_image_dirs = pddby_settings_get("image_dirs");
    char** image_dirs = g_strsplit(raw_image_dirs, ":", 0);
    free(raw_image_dirs);

    int result = 1;
    pddby_database_tx_begin();
    char** dir_name = image_dirs;
    while (*dir_name)
    {
        char* images_path = make_path(context->root_path, *dir_name, 0);
        DIR* dir = opendir(images_path);
        if (!dir)
        {
            pddby_report_error("");
        }

        struct dirent* ent;
        while ((ent = readdir(dir)))
        {
            char* image_path = pddby_aux_build_filename(images_path, ent->d_name, 0);
            result = decode_image(image_path, context->image_magic);
            free(image_path);
            if (!result)
            {
                break;
            }
        }

        closedir(dir);
        free(images_path);

        if (!result)
        {
            break;
        }

        dir_name++;
    }
    pddby_database_tx_commit();
    g_strfreev(image_dirs);
    return result;
}

static int32_t* decode_table(uint16_t magic, char const* path, size_t* table_size)
{
    //GError *err = NULL;
    char* t;
    int32_t* table;
    if (!pddby_aux_file_get_contents(path, &t, table_size))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    table = (int32_t *)t;

    if (*table_size % sizeof(int32_t))
    {
        pddby_report_error("invalid file size: %s (should be multiple of %ld)\n", path, sizeof(int32_t));
    }

    *table_size /= sizeof(int32_t);

    for (size_t i = 0; i < *table_size; i++)
    {
        table[i] = PDDBY_INT32_FROM_LE(table[i]);
        if (table[i] != -1)
        {
            table[i] ^= magic;
            // delphi has file offsets starting from 1, we need 0
            table[i]--;
        }
    }

    return table;
}

static topic_question_t* decode_topic_questions_table(uint16_t magic, char const* path, size_t* table_size)
{
    //GError *err = NULL;
    char* t;
    topic_question_t* table;
    if (!pddby_aux_file_get_contents(path, &t, table_size))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    table = (topic_question_t*)t;

    if (*table_size % sizeof(topic_question_t))
    {
        pddby_report_error("invalid file size: %s (should be multiple of %ld)\n", path, sizeof(topic_question_t));
    }

    *table_size /= sizeof(topic_question_t);

    for (size_t i = 0; i < *table_size; i++)
    {
        table[i].question_offset = PDDBY_INT32_FROM_LE(table[i].question_offset) ^ magic;
        // delphi has file offsets starting from 1, we need 0
        // have to subtract another 1 from it (tell me why it points to 'R', not '[')
        table[i].question_offset -= 2;
    }

    return table;
}

static char* decode_string(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number)
{
    //GError *err = NULL;
    char* str;
    if (!pddby_aux_file_get_contents(path, &str, str_size))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    for (size_t i = 0; i < *str_size; i++)
    {
        // TODO: magic numbers?
        str[i] ^= (magic & 0x0ff) ^ topic_number ^ (i & 1 ? 0x30 : 0x16) ^ ((i + 1) % 255);
    }

    return str;
}

static char* decode_string_v12(uint16_t magic, char const* path, size_t* str_size, int8_t topic_number)
{
    //GError *err = NULL;
    char* str;
    if (!pddby_aux_file_get_contents(path, &str, str_size))
    {
        pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    for (size_t i = 0; i < *str_size; i++)
    {
        str[i] ^= (magic >> 8) ^ (i & 1 ? topic_number : 0) ^ (i & 1 ? 0x80 : 0xaa) ^ ((i + 1) % 255);
    }

    return str;
}

typedef void* (*object_new_t)(int32_t, char const*);
typedef int (*object_save_t)(void*);
typedef void (*object_free_t)(void*);
typedef void (*object_set_images_t)(void*, pddby_images_t* images);

static int decode_simple_data(decode_context_t* context, char const* dat_path, char const* dbt_path,
    object_new_t object_new, object_save_t object_save, object_free_t object_free,
    object_set_images_t object_set_images)
{
    size_t table_size;
    int32_t* table = decode_table(context->data_magic, dat_path, &table_size);

    size_t str_size;
    char* str = context->decode_string(context->data_magic, dbt_path, &str_size, 0);

    pddby_regex_t* simple_data_regex = pddby_regex_new("^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$",
        PDDBY_REGEX_DOTALL);

    struct markup_regex_s
    {
        char const* regex_text;
        char const* replacement_text;
        int regex_options;
        pddby_regex_t* regex;
    };
    struct markup_regex_s markup_regexes[] =
    {
        {
            "@(.+?)@",
            "<span underline='single' underline_color='#ff0000'>\\1</span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "^~\\s*.+?$\\s*",
            "",
            PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "^\\^R\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#cc0000'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "^\\^G\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#00cc00'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "^\\^B\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#0000cc'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "\\^R(.+?)\\^K",
            "<span color='#cc0000'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "\\^G(.+?)\\^K",
            "<span color='#00cc00'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "\\^B(.+?)\\^K",
            "<span color='#0000cc'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "-\\s*$\\s*",
            "",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "([^.> \t])\\s*$\\s*",
            "\\1 ",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY,
            0
        },
        {
            "[ \t]{2,}",
            " ",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY,
            0
        }
    };

    for (size_t i = 0; i < sizeof(markup_regexes) / sizeof(*markup_regexes); i++)
    {
        markup_regexes[i].regex = pddby_regex_new(markup_regexes[i].regex_text, markup_regexes[i].regex_options);
    }

    int result = 1;
    pddby_database_tx_begin();
    for (size_t i = 0; i < table_size; i++)
    {
        void* object = NULL;
        pddby_images_t* object_images = pddby_array_new(0);
        if (table[i] == -1)
        {
            object = object_new(table[i], NULL);
        }
        else
        {
            int32_t next_offset = str_size;
            for (size_t j = 0; j < table_size; j++)
            {
                if (table[j] > table[i] && table[j] < next_offset)
                {
                    next_offset = table[j];
                }
            }

            char* data = g_convert(str + table[i], next_offset - table[i], "utf-8", "cp1251", NULL, NULL);
            if (!data)
            {
                pddby_report_error("");
                //pddby_report_error("%s\n", err->message);
            }
            //GMatchInfo *match_info;
            pddby_regex_match_t* match;
            //if (!g_regex_match(simple_data_regex, data, 0, &match_info))
            if (!pddby_regex_match(simple_data_regex, data, &match))
            {
                pddby_report_error("unable to match simple data\n");
            }
            // FIXME: memleak
            char* number = pddby_string_chomp(pddby_regex_match_fetch(match, 1));
            char* images = pddby_string_chomp(pddby_regex_match_fetch(match, 2));
            char* text = pddby_string_chomp(pddby_regex_match_fetch(match, 3));
            if (*images)
            {
                char** image_names = g_strsplit(images, "\n", 0);
                char** in = image_names;
                while (*in)
                {
                    pddby_image_t* image = pddby_image_find_by_name(pddby_string_chomp(*in + 1));
                    if (!image)
                    {
                        pddby_report_error("unable to find image with name %s\n", *in + 1);
                    }
                    pddby_array_add(object_images, image);
                    in++;
                }
                g_strfreev(image_names);
            }
            for (size_t j = 0; j < sizeof(markup_regexes) / sizeof(*markup_regexes); j++)
            {
                char* new_text = pddby_regex_replace(markup_regexes[j].regex, text, markup_regexes[j].replacement_text);
                free(text);
                text = new_text;
            }
            object = object_new(atoi(number), pddby_string_chomp(text));
            free(text);
            free(images);
            free(number);
            pddby_regex_match_free(match);
            free(data);
        }
        result = object_save(object);
        if (object_set_images)
        {
            object_set_images(object, object_images);
        }
        pddby_image_free_all(object_images);
        if (!result)
        {
            pddby_report_error("unable to save object\n");
        }
        object_free(object);
        printf(".");
    }
    pddby_database_tx_commit();

    printf("\n");

    for (size_t i = 0; i < sizeof(markup_regexes) / sizeof(*markup_regexes); i++)
    {
        pddby_regex_free(markup_regexes[i].regex);
    }

    pddby_regex_free(simple_data_regex);
    free(str);
    free(table);

    return result;
}

static int decode_comments(decode_context_t* context)
{
    char* comments_dat_path = make_path(context->root_path, "tickets", "comments", "comments.dat", 0);
    char* comments_dbt_path = make_path(context->root_path, "tickets", "comments", "comments.dbt", 0);

    int result = decode_simple_data(context, comments_dat_path, comments_dbt_path,
        (object_new_t)pddby_comment_new, (object_save_t)pddby_comment_save, (object_free_t)pddby_comment_free, NULL);

    if (!result)
    {
        pddby_report_error("unable to decode comments\n");
    }

    free(comments_dat_path);
    free(comments_dbt_path);

    return result;
}

static int decode_traffregs(decode_context_t* context)
{
    char* traffreg_dat_path = make_path(context->root_path, "tickets", "traffreg", "traffreg.dat", NULL);
    char* traffreg_dbt_path = make_path(context->root_path, "tickets", "traffreg", "traffreg.dbt", NULL);

    int result = decode_simple_data(context, traffreg_dat_path, traffreg_dbt_path,
        (object_new_t)pddby_traffreg_new, (object_save_t)pddby_traffreg_save, (object_free_t)pddby_traffreg_free,
        (object_set_images_t)pddby_traffreg_set_images);

    if (!result)
    {
        pddby_report_error("unable to decode traffregs");
    }

    free(traffreg_dat_path);
    free(traffreg_dbt_path);

    return result;
}

static int decode_questions_data(decode_context_t* context, char const* dbt_path, int8_t topic_number,
    topic_question_t* sections_data, size_t sections_data_size)
{
    topic_question_t* table = sections_data;
    while (table->topic_number != topic_number)
    {
        table++;
        sections_data_size--;
    }
    size_t table_size = 0;
    while (table_size < sections_data_size && table[table_size].topic_number == topic_number)
    {
        table_size++;
    }

    size_t str_size;
    char* str = context->decode_string(context->data_magic, dbt_path, &str_size, topic_number);

    pddby_regex_t* question_data_regex = pddby_regex_new("\\s*\\[|\\]\\s*", PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* answers_regex = pddby_regex_new("^($^$)?\\d\\.?\\s+", PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* word_break_regex = pddby_regex_new("(?<!\\s)-\\s+", PDDBY_REGEX_NEWLINE_ANY);
    pddby_regex_t* spaces_regex = pddby_regex_new("\\s{2,}", PDDBY_REGEX_NEWLINE_ANY);

    int result = 1;
    pddby_database_tx_begin();
    for (size_t i = 0; i < table_size; i++)
    {
        int32_t next_offset = str_size;
        for (size_t j = 0; j < table_size; j++)
        {
            if (table[j].question_offset > table[i].question_offset && table[j].question_offset < next_offset)
            {
                next_offset = table[j].question_offset;
            }
        }

        char* text = g_convert(str + table[i].question_offset, next_offset - table[i].question_offset, "utf-8",
            "cp1251", NULL, NULL);
        if (!text)
        {
            pddby_report_error("");
            //pddby_report_error("%s\n", err->message);
        }
        char** parts = pddby_regex_split(question_data_regex, text);
        free(text);
        char** p = parts + 1;
        pddby_question_t* question = pddby_question_new(topic_number, NULL, 0, NULL, 0);
        pddby_traffregs_t* question_traffregs = pddby_array_new(0);
        pddby_sections_t* question_sections = pddby_array_new(0);
        pddby_answers_t* question_answers = pddby_array_new(0);
        size_t answer_number = 0;
        while (*p)
        {
            switch ((*p)[0])
            {
            case 'R':
                p++;
                {
                    char** section_names = g_strsplit(*p, " ", 0);
                    char** sn = section_names;
                    while (*sn)
                    {
                        pddby_section_t *section = pddby_section_find_by_name(*sn);
                        if (!section)
                        {
                            pddby_report_error("unable to find section with name %s\n", *sn);
                        }
                        pddby_array_add(question_sections, section);
                        sn++;
                    }
                    g_strfreev(section_names);
                }
                break;
            case 'G':
                p++;
                {
                    pddby_image_t *image = pddby_image_find_by_name(*p);
                    if (!image)
                    {
                        pddby_report_error("unable to find image with name %s\n", *p);
                    }
                    question->image_id = image->id;
                    pddby_image_free(image);
                }
                break;
            case 'Q':
                p++;
                {
                    char* question_text = pddby_regex_replace_literal(word_break_regex, *p, "");
                    question->text = pddby_regex_replace_literal(spaces_regex, question_text, " ");
                    free(question_text);
                    pddby_string_chomp(question->text);
                }
                break;
            case 'W':
            case 'V':
                p++;
                {
                    char** answers = pddby_regex_split(answers_regex, *p);
                    char** a = answers + 1;
                    while (*a)
                    {
                        char* answer_text = pddby_regex_replace_literal(word_break_regex, *a, "");
                        // FIXME: memleak
                        pddby_answer_t *answer = pddby_answer_new(0, pddby_regex_replace_literal(spaces_regex, answer_text, " "), 0);
                        free(answer_text);
                        pddby_string_chomp(answer->text);
                        pddby_array_add(question_answers, answer);
                        a++;
                    }
                    g_strfreev(answers);
                }
                break;
            case 'A':
                p++;
                {
                    answer_number = atoi(*p) - 1;
                }
                break;
            case 'T':
                p++;
                {
                    char* advice_text = pddby_regex_replace_literal(word_break_regex, *p, "");
                    question->advice = pddby_regex_replace_literal(spaces_regex, advice_text, " ");
                    free(advice_text);
                    pddby_string_chomp(question->advice);
                }
                break;
            case 'L':
                p++;
                {
                    char** traffreg_numbers = g_strsplit(*p, " ", 0);
                    char** trn = traffreg_numbers;
                    while (*trn)
                    {
                        pddby_traffreg_t *traffreg = pddby_traffreg_find_by_number(atoi(*trn));
                        if (!traffreg)
                        {
                            pddby_report_error("unable to find traffreg with number %s\n", *trn);
                        }
                        pddby_array_add(question_traffregs, traffreg);
                        trn++;
                    }
                    g_strfreev(traffreg_numbers);
                }
                break;
            case 'C':
                p++;
                {
                    pddby_comment_t *comment = pddby_comment_find_by_number(atoi(*p));
                    if (!comment)
                    {
                        pddby_report_error("unable to find comment with number %s\n", *p);
                    }
                    question->comment_id = comment->id;
                    pddby_comment_free(comment);
                }
                break;
            default:
                pddby_report_error("unknown question data section: %s\n", *p);
            }
            p++;
        }
        g_strfreev(parts);

        result = pddby_question_save(question);
        if (!result)
        {
            pddby_report_error("unable to save question\n");
        }
        for (size_t k = 0; k < pddby_array_size(question_answers); k++)
        {
            pddby_answer_t* answer = (pddby_answer_t*)pddby_array_index(question_answers, k);
            answer->question_id = question->id;
            answer->is_correct = k == answer_number;
            result = pddby_answer_save(answer);
            if (!result)
            {
                pddby_report_error("unable to save answer\n");
            }
        }
        pddby_answer_free_all(question_answers);
        result = pddby_question_set_sections(question, question_sections);
        if (!result)
        {
            pddby_report_error("unable to set question sections\n");
        }
        pddby_section_free_all(question_sections);
        result = pddby_question_set_traffregs(question, question_traffregs);
        if (!result)
        {
            pddby_report_error("unable to set question traffregs\n");
        }
        pddby_traffreg_free_all(question_traffregs);
        pddby_question_free(question);
        printf(".");
    }
    pddby_database_tx_commit();

    printf("\n");

    pddby_regex_free(spaces_regex);
    pddby_regex_free(word_break_regex);
    pddby_regex_free(answers_regex);
    pddby_regex_free(question_data_regex);
    free(str);

    return result;
}

static int compare_topic_questions(void const* first, void const* second)
{
    topic_question_t const* first_tq = (topic_question_t const*)first;
    topic_question_t const* second_tq = (topic_question_t const*)second;

    if (first_tq->topic_number != second_tq->topic_number)
    {
        return first_tq->topic_number - second_tq->topic_number;
    }
    if (first_tq->question_offset != second_tq->question_offset)
    {
        return first_tq->question_offset - second_tq->question_offset;
    }
    return 0;
}

static int decode_questions(decode_context_t* context)
{
    pddby_sections_t* sections = pddby_section_find_all();
    topic_question_t* sections_data = NULL;
    size_t sections_data_size = 0;
    for (size_t i = 0; i < pddby_array_size(sections); i++)
    {
        pddby_section_t* section = pddby_array_index(sections, i);
        char* section_dat_name = g_strdup_printf("%s.dat", section->name);
        char* section_dat_path = make_path(context->root_path, "tickets", "parts", section_dat_name, NULL);
        size_t  size;
        topic_question_t* data = decode_topic_questions_table(context->data_magic, section_dat_path, &size);
        free(section_dat_path);
        free(section_dat_name);
        if (!data)
        {
            pddby_report_error("unable to decode section data\n");
        }
        sections_data = realloc(sections_data, (sections_data_size + size) * sizeof(topic_question_t));
        memmove(&sections_data[sections_data_size], data, size * sizeof(topic_question_t));
        sections_data_size += size;
        free(data);
    }
    pddby_section_free_all(sections);

    qsort(sections_data, sections_data_size, sizeof(topic_question_t), compare_topic_questions);

    pddby_topics_t* topics = pddby_topic_find_all();
    for (size_t i = 0; i < pddby_array_size(topics); i++)
    {
        pddby_topic_t *topic = pddby_array_index(topics, i);

        char* part_dbt_name = g_strdup_printf("part_%d.dbt", topic->number);
        char* part_dbt_path = make_path(context->root_path, "tickets", part_dbt_name, NULL);

        int result = decode_questions_data(context, part_dbt_path, topic->number, sections_data,
            sections_data_size);

        free(part_dbt_path);
        free(part_dbt_name);

        if (!result)
        {
            pddby_report_error("unable to decode questions\n");
        }
    }
    pddby_topic_free_all(topics);
    return 1;
}
