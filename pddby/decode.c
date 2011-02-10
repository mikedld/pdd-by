#include "decode.h"
#include "answer.h"
#include "config.h"
#include "database.h"
#include "decode_image.h"
#include "image.h"
#include "question.h"
#include "section.h"
#include "settings.h"
#include "topic.h"
#include "traffreg.h"
#include "delphi_helper.h"

#include <ctype.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef struct topic_question_s
{
    gint8 topic_number;
    gint32 question_offset;
} __attribute__((__packed__)) topic_question_t;

typedef gchar* (*decode_string_func_t)(guint16 magic, gchar const* path, gsize* str_size, gint8 topic_number);

typedef struct decode_context_s
{
    gchar const* root_path;
    guint16 data_magic;
    guint16 image_magic;
    decode_string_func_t decode_string;
} decode_context_t;

typedef gboolean (*decode_stage_t) (decode_context_t* context);

static gboolean init_magic(decode_context_t* context);
static gboolean decode_images(decode_context_t* context);
static gboolean decode_comments(decode_context_t* context);
static gboolean decode_traffregs(decode_context_t* context);
static gboolean decode_questions(decode_context_t* context);

static gchar *decode_string(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number);
static gchar *decode_string_v12(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number);
static gchar *decode_string_v13(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number);

static const decode_stage_t decode_stages[] =
{
    &init_magic,
    &decode_images,
    &decode_comments,
    &decode_traffregs,
    &decode_questions,
    NULL
};

gboolean decode(const gchar *root_path)
{
    decode_context_t context;
    context.root_path = root_path;

    const decode_stage_t *stage;

    for (stage = decode_stages; NULL != *stage; stage++)
    {
        if (!(*stage)(&context))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static gchar *find_file_ci(const gchar *path, const gchar *fname)
{
    GError *err;
    GDir *dir = g_dir_open(path, 0, &err);
    if (!dir)
    {
        g_error("%s\n", err->message);
    }
    gchar *file_path = NULL;
    const gchar *name;
    while ((name = g_dir_read_name(dir)))
    {
        if (!g_strcasecmp(name, fname))
        {
            file_path = g_build_filename(path, name, NULL);
            break;
        }
    }
    g_dir_close(dir);
    return file_path;
}

static gchar *make_path(const gchar *root_path, ...)
{
    gchar *result = g_strdup(root_path);
    va_list list;
    va_start(list, root_path);
    const gchar *path_part;
    while ((path_part = va_arg(list, const gchar *)))
    {
        gchar *path = find_file_ci(result, path_part);
        g_free(result);
        result = path;
        if (!result)
        {
            break;
        }
    }
    va_end(list);
    g_print("make_path: %s\n", result);
    return result;
}

static gboolean init_magic_2006(decode_context_t* context)
{
    gchar *pdd32_path = make_path(context->root_path, "pdd32.exe", NULL);
    FILE *f = g_fopen(pdd32_path, "rb");
    g_free(pdd32_path);
    if (!f)
    {
        g_error("pdd32.exe not found");
    }

    guchar *buffer = g_malloc(16 * 1024 + 1);
    fseek(f, 16 * 1024 + 1, SEEK_SET);
    if (fread(buffer, 16 * 1024 + 1, 1, f) != 1)
    {
        g_error("pdd32.exe invalid");
    }

    context->data_magic = 0;
    set_randseed((buffer[0] | (buffer[1] << 8)) & 0x0ffff);

    int i;
    for (i = 0; i < 255; i++)
    {
        context->data_magic ^= buffer[delphi_random(16 * 1024)];
    }
    context->data_magic = context->data_magic * buffer[16 * 1024] + 0x1998;

    g_free(buffer);
    fclose(f);
    return TRUE;
}

static gboolean init_magic_2008(decode_context_t* context)
{
    gchar *pdd32_path = make_path(context->root_path, "pdd32.exe", NULL);
    FILE *f = g_fopen(pdd32_path, "rb");
    g_free(pdd32_path);
    if (!f)
    {
        g_error("pdd32.exe not found");
    }

    guchar *buffer = g_malloc(32 * 1024);
    fseek(f, 32 * 1024, SEEK_SET);
    if (fread(buffer, 32 * 1024, 1, f) != 1)
    {
        g_error("pdd32.exe invalid");
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
        int i, j;
        for (i = 0; i < 256; i++)
        {
            guchar ch = buffer[delphi_random(length)];
            for (j = 0; j < 8; j++)
            {
                guint16 old_magic = context->data_magic;
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

    g_free(buffer);
    fclose(f);
    return TRUE;
}

static GChecksum* get_file_checksum(gchar const* file_path, GChecksumType type)
{
    FILE *f = g_fopen(file_path, "rb");
    if (!f)
    {
        g_error("unable to open file: %s", file_path);
    }

    GChecksum* result = g_checksum_new(type);
    guchar *buffer = g_malloc(32 * 1024);

    do
    {
        size_t bytes_read = fread(buffer, 1, 32 * 1024, f);
        if (!bytes_read)
        {
            if (feof(f))
            {
                break;
            }
            g_error("unable to read file: %s", file_path);
        }
        g_checksum_update(result, buffer, bytes_read);
    }
    while (!feof(f));

    g_free(buffer);
    fclose(f);

    return result;
}

static gboolean init_magic(decode_context_t* context)
{
    struct stat root_stat;
    if (stat(context->root_path, &root_stat) == -1)
    {
        g_error("unable to stat root directory");
    }

    struct tm root_tm = *gmtime(&root_stat.st_mtime);

    gboolean result = FALSE;
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
                gchar const* checksum;
                guint16 data_magic;
                guint16 image_magic; // == data_magic ^ 0x1a80
                decode_string_func_t decode_string;
            };
            struct checksum_magic_s const s_checksums[] =
            {
                // v12
                {"2d8a027c323c8a8688c42fe5ccd57c5d", 0x1e35, 0x04b5, decode_string_v12},
                {"fa3f431b556b9e2529a79eb649531af6", 0x4184, 0x5b04, decode_string_v12},
                // v13
                {"7444b8c559cf5a003e1058ece7b267dc", 0x3492, 0x2e12, decode_string_v13}
            };

            gchar *pdd32_path = make_path(context->root_path, "pdd32.exe", NULL);
            GChecksum* checksum = get_file_checksum(pdd32_path, G_CHECKSUM_MD5);
            g_free(pdd32_path);

            for (gsize i = 0; i < sizeof(s_checksums) / sizeof(*s_checksums); i++)
            {
                if (!strcmp(s_checksums[i].checksum, g_checksum_get_string(checksum)))
                {
                    context->data_magic = s_checksums[i].data_magic;
                    context->image_magic = s_checksums[i].image_magic;
                    context->decode_string = s_checksums[i].decode_string;
                    result = TRUE;
                    break;
                }
            }

            g_checksum_free(checksum);
        }
    }
    if (!result)
    {
        g_error("unable to calculate magic number");
    }

    g_print("magic: 0x%04x\n", context->data_magic);
    return TRUE;
}

static gboolean decode_images(decode_context_t* context)
{
    gchar *raw_image_dirs = get_settings("image_dirs");
    gchar **image_dirs = g_strsplit(raw_image_dirs, ":", 0);
    g_free(raw_image_dirs);

    gboolean result = TRUE;
    database_tx_begin();
    gchar **dir_name = image_dirs;
    while (*dir_name)
    {
        gchar *images_path = make_path(context->root_path, *dir_name, NULL);
        GError *err = NULL;
        GDir *dir = g_dir_open(images_path, 0, &err);
        if (!dir)
        {
            g_error("%s\n", err->message);
        }

        const gchar *name;
        while ((name = g_dir_read_name(dir)))
        {
            gchar *image_path = g_build_filename(images_path, name, NULL);
            result = decode_image(image_path, context->image_magic);
            g_free(image_path);
            if (!result)
            {
                break;
            }
        }

        g_dir_close(dir);
        g_free(images_path);

        if (!result)
        {
            break;
        }

        dir_name++;
    }
    database_tx_commit();
    g_strfreev(image_dirs);
    return result;
}

static gint32 *decode_table(guint16 magic, const gchar *path, gsize *table_size)
{
    GError *err = NULL;
    gchar *t;
    gint32 *table;
    if (!g_file_get_contents(path, &t, table_size, &err))
    {
        g_error("%s\n", err->message);
    }

    table = (gint32 *)t;

    if (*table_size % sizeof(gint32))
    {
        g_error("invalid file size: %s (should be multiple of %u)\n", path, (guint)sizeof(gint32));
    }

    *table_size /= sizeof(gint32);

    gsize i;
    for (i = 0; i < *table_size; i++)
    {
        table[i] = GINT32_FROM_LE(table[i]);
        if (table[i] != -1)
        {
            table[i] ^= magic;
            // delphi has file offsets starting from 1, we need 0
            table[i]--;
        }
    }

    return table;
}

static topic_question_t *decode_topic_questions_table(guint16 magic, const gchar *path, gsize *table_size)
{
    GError *err = NULL;
    gchar *t;
    topic_question_t *table;
    if (!g_file_get_contents(path, &t, table_size, &err))
    {
        g_error("%s\n", err->message);
    }

    table = (topic_question_t *)t;

    if (*table_size % sizeof(topic_question_t))
    {
        g_error("invalid file size: %s (should be multiple of %u)\n", path, (guint)sizeof(topic_question_t));
    }

    *table_size /= sizeof(topic_question_t);

    gsize i;
    for (i = 0; i < *table_size; i++)
    {
        table[i].question_offset = GINT32_FROM_LE(table[i].question_offset) ^ magic;
        // delphi has file offsets starting from 1, we need 0
        // have to subtract another 1 from it (tell me why it points to 'R', not '[')
        table[i].question_offset -= 2;
    }

    return table;
}

static gchar *decode_string(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number)
{
    GError *err = NULL;
    gchar *str;
    if (!g_file_get_contents(path, &str, str_size, &err))
    {
        g_error("%s\n", err->message);
    }

    guint32 i;
    for (i = 0; i < *str_size; i++)
    {
        // TODO: magic numbers?
        str[i] ^= (magic & 0x0ff) ^ topic_number ^ (i & 1 ? 0x30 : 0x16) ^ ((i + 1) % 255);
    }

    return str;
}

static gchar *decode_string_v12(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number)
{
    GError *err = NULL;
    gchar *str;
    if (!g_file_get_contents(path, &str, str_size, &err))
    {
        g_error("%s\n", err->message);
    }

    for (gsize i = 0; i < *str_size; i++)
    {
        str[i] ^= (magic >> 8) ^ (i & 1 ? topic_number : 0) ^ (i & 1 ? 0x80 : 0xaa) ^ ((i + 1) % 255);
    }

    return str;
}

static gchar *decode_string_v13(guint16 magic, const gchar *path, gsize *str_size, gint8 topic_number)
{
    GError *err = NULL;
    gchar *str;
    if (!g_file_get_contents(path, &str, str_size, &err))
    {
        g_error("%s\n", err->message);
    }

    for (gsize i = 0; i < *str_size; i++)
    {
        str[i] ^= (magic >> 8) ^ (i & 1 ? topic_number : 0) ^ (i & 1 ? 0x13 : 0x11) ^ ((i + 1) % 255);
    }

    return str;
}

typedef gpointer (*object_new_t)(gint32, const gchar *);
typedef gboolean (*object_save_t)(gpointer *);
typedef void (*object_free_t)(gpointer *);
typedef void (*object_set_images_t)(gpointer *, pdd_images_t *images);

static gboolean decode_simple_data(decode_context_t* context, const gchar *dat_path, const gchar *dbt_path,
    object_new_t object_new, object_save_t object_save, object_free_t object_free,
    object_set_images_t object_set_images)
{
    gsize table_size;
    gint32 *table = decode_table(context->data_magic, dat_path, &table_size);

    gsize str_size;
    gchar *str = context->decode_string(context->data_magic, dbt_path, &str_size, 0);

    GError *err = NULL;
    GRegex *simple_data_regex = g_regex_new("^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$",
        G_REGEX_OPTIMIZE | G_REGEX_DOTALL, 0, &err);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    struct
    {
        GRegex *regex;
        const gchar *replacement;
    } markup_regexes[] =
    {
        {
            g_regex_new("@(.+?)@", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE | G_REGEX_DOTALL, G_REGEX_MATCH_NEWLINE_ANY,
                &err),
            "<span underline='single' underline_color='#ff0000'>\\1</span>"
        },
        {
            g_regex_new("^~\\s*.+?$\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            ""
        },
        {
            g_regex_new("^\\^R\\^(.+?)$\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span underline='single' underline_color='#cc0000'><b>\\1</b></span>"
        },
        {
            g_regex_new("^\\^G\\^(.+?)$\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span underline='single' underline_color='#00cc00'><b>\\1</b></span>"
        },
        {
            g_regex_new("^\\^B\\^(.+?)$\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span underline='single' underline_color='#0000cc'><b>\\1</b></span>"
        },
        {
            g_regex_new("\\^R(.+?)\\^K", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE | G_REGEX_DOTALL,
                G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span color='#cc0000'><b>\\1</b></span>"
        },
        {
            g_regex_new("\\^G(.+?)\\^K", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE | G_REGEX_DOTALL,
                G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span color='#00cc00'><b>\\1</b></span>"
        },
        {
            g_regex_new("\\^B(.+?)\\^K", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE | G_REGEX_DOTALL,
                G_REGEX_MATCH_NEWLINE_ANY, &err),
            "<span color='#0000cc'><b>\\1</b></span>"
        },
        {
            g_regex_new("-\\s*$\\s*", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            ""
        },
        {
            g_regex_new("([^.> \t])\\s*$\\s*", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            "\\1 "
        },
        {
            g_regex_new("[ \t]{2,}", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE, G_REGEX_MATCH_NEWLINE_ANY, &err),
            " "
        },
    };

    gsize i, j;
    gboolean result = TRUE;
    database_tx_begin();
    for (i = 0; i < table_size; i++)
    {
        gpointer *object = NULL;
        pdd_images_t *object_images = g_ptr_array_new();
        if (table[i] == -1)
        {
            object = object_new(table[i], NULL);
        }
        else
        {
            gint32 next_offset = str_size;
            for (j = 0; j < table_size; j++)
            {
                if (table[j] > table[i] && table[j] < next_offset)
                {
                    next_offset = table[j];
                }
            }

            gchar *data = g_convert(str + table[i], next_offset - table[i], "utf-8", "cp1251", NULL, NULL, &err);
            if (!data)
            {
                g_error("%s\n", err->message);
            }
            GMatchInfo *match_info;
            if (!g_regex_match(simple_data_regex, data, 0, &match_info))
            {
                g_error("unable to match simple data\n");
            }
            gchar *number = g_strchomp(g_match_info_fetch(match_info, 1));
            gchar *images = g_strchomp(g_match_info_fetch(match_info, 2));
            gchar *text = g_strchomp(g_match_info_fetch(match_info, 3));
            if (*images)
            {
                gchar **image_names = g_strsplit(images, "\n", 0);
                gchar **in = image_names;
                while (*in)
                {
                    pdd_image_t *image = image_find_by_name(g_strchomp(*in + 1));
                    if (!image)
                    {
                        g_error("unable to find image with name %s\n", *in + 1);
                    }
                    g_ptr_array_add(object_images, image);
                    in++;
                }
                g_strfreev(image_names);
            }
            for (j = 0; j < sizeof(markup_regexes) / sizeof(*markup_regexes); j++)
            {
                gchar *new_text = g_regex_replace(markup_regexes[j].regex, text, -1, 0, markup_regexes[j].replacement,
                    0, &err);
                if (err)
                {
                    g_error("%s\n", err->message);
                }
                g_free(text);
                text = new_text;
            }
            object = object_new(atoi(number), g_strchomp(text));
            g_free(text);
            g_free(images);
            g_free(number);
            g_match_info_free(match_info);
            g_free(data);
        }
        result = object_save(object);
        if (object_set_images)
        {
            object_set_images(object, object_images);
        }
        image_free_all(object_images);
        if (!result)
        {
            g_error("unable to save object\n");
        }
        object_free(object);
        g_print(".");
    }
    database_tx_commit();

    g_print("\n");

    for (i = 0; i < sizeof(markup_regexes) / sizeof(*markup_regexes); i++)
    {
        g_regex_unref(markup_regexes[i].regex);
    }

    g_regex_unref(simple_data_regex);
    g_free(str);
    g_free(table);

    return result;
}

static gboolean decode_comments(decode_context_t* context)
{
    gchar *comments_dat_path = make_path(context->root_path, "tickets", "comments", "comments.dat", NULL);
    gchar *comments_dbt_path = make_path(context->root_path, "tickets", "comments", "comments.dbt", NULL);

    gboolean result = decode_simple_data(context, comments_dat_path, comments_dbt_path,
        (object_new_t)comment_new, (object_save_t)comment_save, (object_free_t)comment_free, NULL);

    if (!result)
    {
        g_error("unable to decode comments\n");
    }

    g_free(comments_dat_path);
    g_free(comments_dbt_path);

    return result;
}

static gboolean decode_traffregs(decode_context_t* context)
{
    gchar *traffreg_dat_path = make_path(context->root_path, "tickets", "traffreg", "traffreg.dat", NULL);
    gchar *traffreg_dbt_path = make_path(context->root_path, "tickets", "traffreg", "traffreg.dbt", NULL);

    gboolean result = decode_simple_data(context, traffreg_dat_path, traffreg_dbt_path,
        (object_new_t)traffreg_new, (object_save_t)traffreg_save, (object_free_t)traffreg_free,
        (object_set_images_t)traffreg_set_images);

    if (!result)
    {
        g_error("unable to decode traffregs");
    }

    g_free(traffreg_dat_path);
    g_free(traffreg_dbt_path);

    return result;
}

static gboolean decode_questions_data(decode_context_t* context, const gchar *dbt_path, gint8 topic_number,
    topic_question_t *sections_data, gsize sections_data_size)
{
    topic_question_t *table = sections_data;
    while (table->topic_number != topic_number)
    {
        table++;
        sections_data_size--;
    }
    gsize table_size = 0;
    while (table_size < sections_data_size && table[table_size].topic_number == topic_number)
    {
        table_size++;
    }

    gsize str_size;
    gchar *str = context->decode_string(context->data_magic, dbt_path, &str_size, topic_number);

    GError *err = NULL;
    GRegex *question_data_regex = g_regex_new("\\s*\\[|\\]\\s*", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
    if (err)
    {
        g_error("%s\n", err->message);
    }
    GRegex *answers_regex = g_regex_new("^($^$)?\\d\\.?\\s+", G_REGEX_OPTIMIZE | G_REGEX_MULTILINE,
        G_REGEX_MATCH_NEWLINE_ANY, &err);
    if (err)
    {
        g_error("%s\n", err->message);
    }
    GRegex *word_break_regex = g_regex_new("(?<!\\s)-\\s+", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
    if (err)
    {
        g_error("%s\n", err->message);
    }
    GRegex *spaces_regex = g_regex_new("\\s{2,}", G_REGEX_OPTIMIZE, G_REGEX_MATCH_NEWLINE_ANY, &err);
    if (err)
    {
        g_error("%s\n", err->message);
    }

    gsize i, j;
    gboolean result = TRUE;
    database_tx_begin();
    for (i = 0; i < table_size; i++)
    {
        gint32 next_offset = str_size;
        for (j = 0; j < table_size; j++)
        {
            if (table[j].question_offset > table[i].question_offset && table[j].question_offset < next_offset)
            {
                next_offset = table[j].question_offset;
            }
        }

        gchar *text = g_convert(str + table[i].question_offset, next_offset - table[i].question_offset, "utf-8",
            "cp1251", NULL, NULL, &err);
        if (!text)
        {
            g_error("%s\n", err->message);
        }
        gchar **parts = g_regex_split(question_data_regex, text, 0);
        g_free(text);
        gchar **p = parts + 1;
        pdd_question_t *question = question_new(topic_number, NULL, 0, NULL, 0);
        pdd_traffregs_t *question_traffregs = g_ptr_array_new();
        pdd_sections_t *question_sections = g_ptr_array_new();
        pdd_answers_t *question_answers = g_ptr_array_new();
        gsize answer_number = 0;
        while (*p)
        {
            switch ((*p)[0])
            {
            case 'R':
                p++;
                {
                    gchar **section_names = g_strsplit(*p, " ", 0);
                    gchar **sn = section_names;
                    while (*sn)
                    {
                        pdd_section_t *section = section_find_by_name(*sn);
                        if (!section)
                        {
                            g_error("unable to find section with name %s\n", *sn);
                        }
                        g_ptr_array_add(question_sections, section);
                        sn++;
                    }
                    g_strfreev(section_names);
                }
                break;
            case 'G':
                p++;
                {
                    pdd_image_t *image = image_find_by_name(*p);
                    if (!image)
                    {
                        g_error("unable to find image with name %s\n", *p);
                    }
                    question->image_id = image->id;
                    image_free(image);
                }
                break;
            case 'Q':
                p++;
                {
                    gchar *question_text = g_regex_replace_literal(word_break_regex, *p, -1, 0, "", 0, &err);
                    if (err)
                    {
                        g_error("%s\n", err->message);
                    }
                    question->text = g_regex_replace_literal(spaces_regex, question_text, -1, 0, " ", 0, &err);
                    g_free(question_text);
                    if (err)
                    {
                        g_error("%s\n", err->message);
                    }
                    g_strchomp(question->text);
                }
                break;
            case 'W':
            case 'V':
                p++;
                {
                    gchar **answers = g_regex_split(answers_regex, *p, 0);
                    gchar **a = answers + 1;
                    while (*a)
                    {
                        gchar *answer_text = g_regex_replace_literal(word_break_regex, *a, -1, 0, "", 0, &err);
                        if (err)
                        {
                            g_error("%s\n", err->message);
                        }
                        pdd_answer_t *answer = answer_new(0, g_regex_replace_literal(spaces_regex, answer_text, -1, 0,
                            " ", 0, &err), FALSE);
                        g_free(answer_text);
                        if (err)
                        {
                            g_error("%s\n", err->message);
                        }
                        g_strchomp(answer->text);
                        g_ptr_array_add(question_answers, answer);
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
                    gchar *advice_text = g_regex_replace_literal(word_break_regex, *p, -1, 0, "", 0, &err);
                    if (err)
                    {
                        g_error("%s\n", err->message);
                    }
                    question->advice = g_regex_replace_literal(spaces_regex, advice_text, -1, 0, " ", 0, &err);
                    g_free(advice_text);
                    if (err)
                    {
                        g_error("%s\n", err->message);
                    }
                    g_strchomp(question->advice);
                }
                break;
            case 'L':
                p++;
                {
                    gchar **traffreg_numbers = g_strsplit(*p, " ", 0);
                    gchar **trn = traffreg_numbers;
                    while (*trn)
                    {
                        pdd_traffreg_t *traffreg = traffreg_find_by_number(atoi(*trn));
                        if (!traffreg)
                        {
                            g_error("unable to find traffreg with number %s\n", *trn);
                        }
                        g_ptr_array_add(question_traffregs, traffreg);
                        trn++;
                    }
                    g_strfreev(traffreg_numbers);
                }
                break;
            case 'C':
                p++;
                {
                    pdd_comment_t *comment = comment_find_by_number(atoi(*p));
                    if (!comment)
                    {
                        g_error("unable to find comment with number %s\n", *p);
                    }
                    question->comment_id = comment->id;
                    comment_free(comment);
                }
                break;
            default:
                g_error("unknown question data section: %s\n", *p);
            }
            p++;
        }
        g_strfreev(parts);

        result = question_save(question);
        if (!result)
        {
            g_error("unable to save question\n");
        }
        gsize k;
        for (k = 0; k < question_answers->len; k++)
        {
            pdd_answer_t *answer = (pdd_answer_t *)g_ptr_array_index(question_answers, k);
            answer->question_id = question->id;
            answer->is_correct = k == answer_number;
            result = answer_save(answer);
            if (!result)
            {
                g_error("unable to save answer\n");
            }
        }
        answer_free_all(question_answers);
        result = question_set_sections(question, question_sections);
        if (!result)
        {
            g_error("unable to set question sections\n");
        }
        section_free_all(question_sections);
        result = question_set_traffregs(question, question_traffregs);
        if (!result)
        {
            g_error("unable to set question traffregs\n");
        }
        traffreg_free_all(question_traffregs);
        question_free(question);
        g_print(".");
    }
    database_tx_commit();

    g_print("\n");

    g_regex_unref(spaces_regex);
    g_regex_unref(word_break_regex);
    g_regex_unref(question_data_regex);
    g_free(str);

    return result;
}

static int compare_topic_questions(const void *first, const void *second)
{
    const topic_question_t *first_tq = (const topic_question_t *)first;
    const topic_question_t *second_tq = (const topic_question_t *)second;

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

static gboolean decode_questions(decode_context_t* context)
{
    pdd_sections_t *sections = section_find_all();
    gsize i;
    topic_question_t *sections_data = NULL;
    gsize sections_data_size = 0;
    for (i = 0; i < sections->len; i++)
    {
        pdd_section_t *section = ((pdd_section_t **)sections->pdata)[i];
        gchar *section_dat_name = g_strdup_printf("%s.dat", section->name);
        gchar *section_dat_path = make_path(context->root_path, "tickets", "parts", section_dat_name, NULL);
        gsize size;
        topic_question_t *data = decode_topic_questions_table(context->data_magic, section_dat_path, &size);
        g_free(section_dat_path);
        g_free(section_dat_name);
        if (!data)
        {
            g_error("unable to decode section data\n");
        }
        sections_data = g_realloc(sections_data, (sections_data_size + size) * sizeof(topic_question_t));
        g_memmove(&sections_data[sections_data_size], data, size * sizeof(topic_question_t));
        sections_data_size += size;
        g_free(data);
    }
    section_free_all(sections);

    qsort(sections_data, sections_data_size, sizeof(topic_question_t), compare_topic_questions);

    pdd_topics_t *topics = topic_find_all();
    for (i = 0; i < topics->len; i++)
    {
        pdd_topic_t *topic = ((pdd_topic_t **)topics->pdata)[i];

        gchar *part_dbt_name = g_strdup_printf("part_%d.dbt", topic->number);
        gchar *part_dbt_path = make_path(context->root_path, "tickets", part_dbt_name, NULL);

        gboolean result = decode_questions_data(context, part_dbt_path, topic->number, sections_data,
            sections_data_size);

        g_free(part_dbt_path);
        g_free(part_dbt_name);

        if (!result)
        {
            g_error("unable to decode questions\n");
        }
    }
    topic_free_all(topics);
    return TRUE;
}
