#include "decode.h"

#include "decode_context.h"
#include "decode_image.h"
#include "decode_questions.h"

#include "comment.h"
#include "config.h"
#include "private/pddby.h"
#include "private/platform.h"
#include "private/util/aux.h"
#include "private/util/database.h"
#include "private/util/delphi.h"
#include "private/util/regex.h"
#include "private/util/settings.h"
#include "section.h"
#include "topic.h"
#include "traffreg.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

typedef void* (*pddby_object_new_t)(pddby_t* pddby, int32_t, char const*);
typedef int (*pddby_object_save_t)(void*);
typedef void (*pddby_object_free_t)(void*);
typedef void (*pddby_object_set_images_t)(void*, pddby_images_t* images);

int pddby_decode_images(pddby_t* pddby)
{
    char* raw_image_dirs = pddby_settings_get(pddby, "image_dirs");
    char** image_dirs = pddby_string_split(raw_image_dirs, ":");
    free(raw_image_dirs);

    int result = 1;
    pddby_db_tx_begin(pddby);
    char** dir_name = image_dirs;
    while (*dir_name)
    {
        char* images_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, *dir_name, 0);
        DIR* dir = opendir(images_path);
        if (!dir)
        {
            //pddby_report_error("");
        }

        struct dirent* ent;
        while ((ent = readdir(dir)))
        {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            {
                continue;
            }
            char* image_path = pddby_aux_build_filename(pddby, images_path, ent->d_name, 0);
            result = decode_image(pddby, image_path, pddby->decode_context->image_magic);
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
    pddby_db_tx_commit(pddby);
    pddby_stringv_free(image_dirs);
    return result;
}

static int32_t* pddby_decode_table(pddby_t* pddby, uint16_t magic, char const* path, size_t* table_size)
{
    //GError *err = NULL;
    char* t;
    int32_t* table;
    if (!pddby_aux_file_get_contents(pddby, path, &t, table_size))
    {
        //pddby_report_error("");
        //pddby_report_error("%s\n", err->message);
    }

    table = (int32_t *)t;

    if (*table_size % sizeof(int32_t))
    {
        //pddby_report_error("invalid file size: %s (should be multiple of %ld)\n", path, sizeof(int32_t));
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

static int pddby_decode_simple_data(pddby_t* pddby, char const* dat_path, char const* dbt_path,
    pddby_object_new_t object_new, pddby_object_save_t object_save, pddby_object_free_t object_free,
    pddby_object_set_images_t object_set_images)
{
    size_t table_size;
    int32_t* table = pddby_decode_table(pddby, pddby->decode_context->data_magic, dat_path, &table_size);

    size_t str_size;
    char* str = pddby->decode_context->decode_string(pddby->decode_context, dbt_path, &str_size, 0);

    pddby_regex_t* simple_data_regex = pddby_regex_new(pddby, "^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$",
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
        markup_regexes[i].regex = pddby_regex_new(pddby, markup_regexes[i].regex_text, markup_regexes[i].regex_options);
    }

    int result = 1;
    pddby_db_tx_begin(pddby);
    for (size_t i = 0; i < table_size; i++)
    {
        void* object = NULL;
        pddby_images_t* object_images = pddby_images_new(pddby);
        if (table[i] == -1)
        {
            object = object_new(pddby, table[i], NULL);
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

            char* data = pddby_string_convert(pddby->decode_context->iconv, str + table[i], next_offset - table[i]);
            if (!data)
            {
                //pddby_report_error("");
                //pddby_report_error("%s\n", err->message);
            }
            //GMatchInfo *match_info;
            pddby_regex_match_t* match;
            //if (!g_regex_match(simple_data_regex, data, 0, &match_info))
            if (!pddby_regex_match(simple_data_regex, data, &match))
            {
                //pddby_report_error("unable to match simple data\n");
            }
            char* number = pddby_string_chomp(pddby_regex_match_fetch(match, 1));
            char* images = pddby_string_chomp(pddby_regex_match_fetch(match, 2));
            char* text = pddby_string_chomp(pddby_regex_match_fetch(match, 3));
            if (*images)
            {
                char** image_names = pddby_string_split(images, "\n");
                char** in = image_names;
                while (*in)
                {
                    pddby_image_t* image = pddby_image_find_by_name(pddby, pddby_string_chomp(*in + 1));
                    if (!image)
                    {
                        //pddby_report_error("unable to find image with name %s\n", *in + 1);
                    }
                    pddby_array_add(object_images, image);
                    in++;
                }
                pddby_stringv_free(image_names);
            }
            for (size_t j = 0; j < sizeof(markup_regexes) / sizeof(*markup_regexes); j++)
            {
                char* new_text = pddby_regex_replace(markup_regexes[j].regex, text, markup_regexes[j].replacement_text);
                free(text);
                text = new_text;
            }
            object = object_new(pddby, atoi(number), pddby_string_chomp(text));
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
        pddby_images_free(object_images);
        if (!result)
        {
            //pddby_report_error("unable to save object\n");
        }
        object_free(object);
        printf(".");
    }
    pddby_db_tx_commit(pddby);

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

int pddby_decode_comments(pddby_t* pddby)
{
    char* comments_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "comments", "comments.dat", 0);
    char* comments_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "comments", "comments.dbt", 0);

    int result = pddby_decode_simple_data(pddby, comments_dat_path, comments_dbt_path,
        (pddby_object_new_t)pddby_comment_new, (pddby_object_save_t)pddby_comment_save,
        (pddby_object_free_t)pddby_comment_free, NULL);

    if (!result)
    {
        //pddby_report_error("unable to decode comments\n");
    }

    free(comments_dat_path);
    free(comments_dbt_path);

    return result;
}

int pddby_decode_traffregs(pddby_t* pddby)
{
    char* traffreg_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "traffreg", "traffreg.dat",
        NULL);
    char* traffreg_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "traffreg", "traffreg.dbt",
        NULL);

    int result = pddby_decode_simple_data(pddby, traffreg_dat_path, traffreg_dbt_path,
        (pddby_object_new_t)pddby_traffreg_new, (pddby_object_save_t)pddby_traffreg_save,
        (pddby_object_free_t)pddby_traffreg_free, (pddby_object_set_images_t)pddby_traffreg_set_images);

    if (!result)
    {
        //pddby_report_error("unable to decode traffregs");
    }

    free(traffreg_dat_path);
    free(traffreg_dbt_path);

    return result;
}

int pddby_decode_questions(pddby_t* pddby)
{
    pddby_sections_t* sections = pddby_sections_find_all(pddby);
    pddby_topic_question_t* sections_data = NULL;
    size_t sections_data_size = 0;
    for (size_t i = 0; i < pddby_array_size(sections); i++)
    {
        pddby_section_t* section = pddby_array_index(sections, i);
        char section_dat_name[32];
        sprintf(section_dat_name, "%s.dat", section->name);
        char* section_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "parts", section_dat_name,
            NULL);
        size_t  size;
        pddby_topic_question_t* data = pddby_decode_topic_questions_table(pddby->decode_context, section_dat_path, &size);
        free(section_dat_path);
        if (!data)
        {
            //pddby_report_error("unable to decode section data\n");
        }
        sections_data = realloc(sections_data, (sections_data_size + size) * sizeof(pddby_topic_question_t));
        memmove(&sections_data[sections_data_size], data, size * sizeof(pddby_topic_question_t));
        sections_data_size += size;
        free(data);
    }
    pddby_sections_free(sections);

    qsort(sections_data, sections_data_size, sizeof(pddby_topic_question_t), pddby_compare_topic_questions);

    pddby_topics_t* topics = pddby_topics_find_all(pddby);
    for (size_t i = 0; i < pddby_array_size(topics); i++)
    {
        pddby_topic_t *topic = pddby_array_index(topics, i);

        char part_dbt_name[32];
        sprintf(part_dbt_name, "part_%d.dbt", topic->number);
        char* part_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", part_dbt_name, NULL);

        int result = pddby_decode_questions_data(pddby->decode_context, part_dbt_path, topic->number, sections_data,
            sections_data_size);

        free(part_dbt_path);

        if (!result)
        {
            //pddby_report_error("unable to decode questions\n");
        }
    }
    pddby_topics_free(topics);
    free(sections_data);
    return 1;
}
