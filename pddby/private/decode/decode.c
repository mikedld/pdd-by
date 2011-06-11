#include "decode.h"

#include "decode_context.h"
#include "decode_image.h"
#include "decode_questions.h"
#include "simple_data_parser.h"

#include "comment.h"
#include "config.h"
#include "private/pddby.h"
#include "private/platform.h"
#include "private/util/aux.h"
#include "private/util/database.h"
#include "private/util/delphi.h"
#include "private/util/regex.h"
#include "private/util/report.h"
#include "private/util/settings.h"
#include "section.h"
#include "topic.h"
#include "traffreg.h"

#include <dirent.h>
#include <errno.h>
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
typedef int (*pddby_object_set_images_t)(void*, pddby_images_t* images);

int pddby_decode_images(pddby_t* pddby)
{
    char** image_dir_names = NULL;

    char* raw_image_dirs = pddby_settings_get(pddby, "image_dirs");
    if (!raw_image_dirs)
    {
        goto error;
    }

    image_dir_names = pddby_string_split(pddby, raw_image_dirs, ":");
    free(raw_image_dirs);
    if (!image_dir_names)
    {
        goto error;
    }

    if (!pddby_db_tx_begin(pddby))
    {
        goto error;
    }

    int result = 1;
    char** dir_name = image_dir_names;
    while (*dir_name)
    {
        DIR* dir = NULL;
        pddby_array_t* image_dirs = NULL;

        char* images_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, *dir_name, 0);
        if (!images_path)
        {
            goto cycle_error;
        }

        image_dirs = pddby_array_new(pddby, free);
        if (!image_dirs)
        {
            goto cycle_error;
        }

        dir = opendir(images_path);
        if (!dir)
        {
            goto cycle_error;
        }

        for (;;)
        {
            errno = 0;
            struct dirent* ent = readdir(dir);
            if (!ent)
            {
                if (errno)
                {
                    goto cycle_error;
                }
                break;
            }

            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            {
                continue;
            }

            if (!pddby_array_add(image_dirs, strdup(ent->d_name)))
            {
                goto cycle_error;
            }
        }

        if (closedir(dir) == -1)
        {
            pddby_report(pddby, pddby_message_type_warning, "unable to close directory \"%s\"", *dir_name);
        }
        dir = NULL;

        pddby_report_progress_begin(pddby, pddby_array_size(image_dirs));

        for (size_t i = 0, size = pddby_array_size(image_dirs); i < size; i++)
        {
            char* image_path = pddby_aux_build_filename(pddby, images_path, pddby_array_index(image_dirs, i), 0);
            if (!image_path)
            {
                goto cycle_error;
            }

            result = pddby_decode_image(pddby, image_path, pddby->decode_context->image_magic);
            free(image_path);
            if (!result)
            {
                goto cycle_error;
            }

            pddby_report_progress(pddby, i + 1);
        }

        pddby_report_progress_end(pddby);

        pddby_array_free(image_dirs, 1);
        free(images_path);

        dir_name++;
        continue;

cycle_error:
        pddby_report(pddby, pddby_message_type_error, "unable to decode images in \"%s\"", *dir_name);

        if (images_path)
        {
            free(images_path);
        }
        if (image_dirs)
        {
            pddby_array_free(image_dirs, 1);
        }
        if (dir)
        {
            if (closedir(dir) == -1)
            {
                pddby_report(pddby, pddby_message_type_warning, "unable to close directory \"%s\"", *dir_name);
            }
        }

        goto error;
    }

    if (!pddby_db_tx_commit(pddby))
    {
        goto error;
    }

    pddby_stringv_free(image_dir_names);
    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode images");

    if (image_dir_names)
    {
        pddby_stringv_free(image_dir_names);
    }

    return 0;
}

static int32_t* pddby_decode_table(pddby_t* pddby, uint16_t magic, char const* path, size_t* table_size)
{
    int32_t* table = NULL;
    char* t = NULL;
    if (!pddby_aux_file_get_contents(pddby, path, &t, table_size))
    {
        goto error;
    }

    table = (int32_t *)t;

    if (*table_size % sizeof(int32_t))
    {
        pddby_report(pddby, pddby_message_type_error, "invalid file size: %s (should be multiple of %ld)\n", path,
            sizeof(int32_t));
        goto error;
    }

    *table_size /= sizeof(int32_t);

    for (size_t i = 0; i < *table_size; i++)
    {
        table[i] = PDDBY_INT32_FROM_LE(table[i]);
        if (table[i] != -1)
        {
            table[i] ^= magic;
            // delphi has file offsets starting from 1, we need 0
            //table[i]--;
        }
    }

    return table;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode table");

    if (table)
    {
        free(table);
    }

    return NULL;
}

static int pddby_decode_simple_data(pddby_t* pddby, char const* dat_path, char const* dbt_path,
    pddby_object_new_t object_new, pddby_object_save_t object_save, pddby_object_free_t object_free,
    pddby_object_set_images_t object_set_images)
{
    pddby_simple_data_parser_t* parser = NULL;
    int32_t* table = NULL;
    char* dbt_content = NULL;

    parser = pddby_simple_data_parser_new(pddby);
    if (!parser)
    {
        goto error;
    }

    size_t table_size;
    table = pddby_decode_table(pddby, pddby->decode_context->simple_table_magic, dat_path, &table_size);
    if (!table)
    {
        goto error;
    }

    size_t dbt_content_size;
    if (!pddby_aux_file_get_contents(pddby, dbt_path, &dbt_content, &dbt_content_size))
    {
        goto error;
    }

    if (!pddby_db_tx_begin(pddby))
    {
        goto error;
    }

    pddby_report_progress_begin(pddby, table_size);

    int result = 1;
    for (size_t i = 0; i < table_size; i++)
    {
        void* object = NULL;
        int32_t number = 0;
        char** image_names = NULL;
        char* text = NULL;

        pddby_images_t* object_images = pddby_images_new(pddby);
        if (!object_images)
        {
            goto cycle_error;
        }

        if (table[i] == -1)
        {
            object = object_new(pddby, table[i], NULL);
        }
        else
        {
            int32_t next_offset = dbt_content_size;
            for (size_t j = 0; j < table_size; j++)
            {
                if (table[j] > table[i] && table[j] < next_offset)
                {
                    next_offset = table[j];
                }
            }

            size_t str_size = next_offset - table[i];
            char* str = pddby->decode_context->decode_string(pddby->decode_context, dbt_content + table[i], &str_size,
                0, table[i]);
            if (!str)
            {
                goto cycle_error;
            }

            char* data = pddby_string_convert(pddby->decode_context->iconv, str, str_size);
            free(str);
            if (!data)
            {
                goto cycle_error;
            }

            text = pddby_simple_data_parser_parse(parser, data, &number, &image_names);
            free(data);
            if (!text)
            {
                goto cycle_error;
            }

            if (image_names)
            {
                char** in = image_names;
                while (*in)
                {
                    pddby_image_t* image = pddby_image_find_by_name(pddby, *in);
                    if (!image)
                    {
                        goto cycle_error;
                    }
                    if (!pddby_array_add(object_images, image))
                    {
                        goto cycle_error;
                    }
                    in++;
                }

                pddby_stringv_free(image_names);
                image_names = NULL;
            }

            object = object_new(pddby, number, text);

            free(text);
            text = NULL;
        }
        if (!object)
        {
            goto cycle_error;
        }

        if (!object_save(object))
        {
            goto cycle_error;
        }

        if (object_set_images && !object_set_images(object, object_images))
        {
            goto cycle_error;
        }

        pddby_images_free(object_images);
        object_free(object);

        pddby_report_progress(pddby, i + 1);
        continue;

cycle_error:
        pddby_report(pddby, pddby_message_type_error, "unable to decode simple data object #%lu", i);

        if (object_images)
        {
            pddby_images_free(object_images);
        }
        if (text)
        {
            free(text);
        }
        if (image_names)
        {
            pddby_stringv_free(image_names);
        }
        if (object)
        {
            object_free(object);
        }

        goto error;
    }

    pddby_report_progress_end(pddby);

    if (!pddby_db_tx_commit(pddby))
    {
        goto error;
    }

    free(dbt_content);
    free(table);
    pddby_simple_data_parser_free(parser);

    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode simple data");

    if (dbt_content)
    {
        free(dbt_content);
    }
    if (table)
    {
        free(table);
    }
    if (parser)
    {
        pddby_simple_data_parser_free(parser);
    }

    return 0;
}

int pddby_decode_comments(pddby_t* pddby)
{
    char* comments_dat_path = NULL;
    char* comments_dbt_path = NULL;

    comments_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "comments",
        "comments.dat", NULL);
    if (!comments_dat_path)
    {
        goto error;
    }

    comments_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "comments",
        "comments.dbt", NULL);
    if (!comments_dbt_path)
    {
        goto error;
    }

    if (!pddby_decode_simple_data(pddby, comments_dat_path, comments_dbt_path,
        (pddby_object_new_t)pddby_comment_new, (pddby_object_save_t)pddby_comment_save,
        (pddby_object_free_t)pddby_comment_free, NULL))
    {
        goto error;
    }

    free(comments_dat_path);
    free(comments_dbt_path);

    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode comments");

    if (comments_dbt_path)
    {
        free(comments_dbt_path);
    }
    if (comments_dat_path)
    {
        free(comments_dat_path);
    }

    return 0;
}

int pddby_decode_traffregs(pddby_t* pddby)
{
    char* traffreg_dat_path = NULL;
    char* traffreg_dbt_path = NULL;

    traffreg_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "traffreg",
        "traffreg.dat", NULL);
    if (!traffreg_dat_path)
    {
        goto error;
    }

    traffreg_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets", "traffreg",
        "traffreg.dbt", NULL);
    if (!traffreg_dbt_path)
    {
        goto error;
    }

    if (!pddby_decode_simple_data(pddby, traffreg_dat_path, traffreg_dbt_path,
        (pddby_object_new_t)pddby_traffreg_new, (pddby_object_save_t)pddby_traffreg_save,
        (pddby_object_free_t)pddby_traffreg_free, (pddby_object_set_images_t)pddby_traffreg_set_images))
    {
        goto error;
    }

    free(traffreg_dat_path);
    free(traffreg_dbt_path);

    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode traffregs");

    if (traffreg_dbt_path)
    {
        free(traffreg_dbt_path);
    }
    if (traffreg_dat_path)
    {
        free(traffreg_dat_path);
    }

    return 0;
}

int pddby_decode_questions(pddby_t* pddby)
{
    pddby_sections_t* sections = NULL;
    pddby_topic_question_t* sections_data = NULL;
    pddby_topics_t* topics = NULL;

    sections = pddby_sections_find_all(pddby);
    if (!sections)
    {
        goto error;
    }

    size_t sections_data_size = 0;
    for (size_t i = 0, size = pddby_array_size(sections); i < size; i++)
    {
        pddby_section_t* section = pddby_array_index(sections, i);

        char section_dat_name[32];
        if (snprintf(section_dat_name, sizeof(section_dat_name), "%s.dat", section->name) >=
            (int)sizeof(section_dat_name))
        {
            goto error;
        }

        char* section_dat_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets",
            "parts", section_dat_name, NULL);
        if (!section_dat_path)
        {
            goto error;
        }

        size_t size;
        pddby_topic_question_t* data = pddby->decode_context->decode_topic_questions_table(pddby->decode_context,
            section_dat_path, &size);
        free(section_dat_path);
        if (!data)
        {
            goto error;
        }

        sections_data = realloc(sections_data, (sections_data_size + size) * sizeof(pddby_topic_question_t));
        if (!sections_data)
        {
            free(data);
            goto error;
        }

        memmove(&sections_data[sections_data_size], data, size * sizeof(pddby_topic_question_t));
        sections_data_size += size;
        free(data);
    }

    pddby_sections_free(sections);
    sections = NULL;

    qsort(sections_data, sections_data_size, sizeof(pddby_topic_question_t), pddby_compare_topic_questions);

    topics = pddby_topics_find_all(pddby);
    if (!topics)
    {
        goto error;
    }

    for (size_t i = 0, size = pddby_array_size(topics); i < size; i++)
    {
        pddby_topic_t *topic = pddby_array_index(topics, i);

        char part_dbt_name[32];
        if (snprintf(part_dbt_name, sizeof(part_dbt_name), "part_%d.dbt", topic->number) >= (int)sizeof(part_dbt_name))
        {
            goto error;
        }

        char* part_dbt_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, "tickets",
            part_dbt_name, NULL);
        if (!part_dbt_path)
        {
            goto error;
        }

        int result = pddby_decode_questions_data(pddby->decode_context, part_dbt_path, topic->number, sections_data,
            sections_data_size);
        free(part_dbt_path);
        if (!result)
        {
            goto error;
        }
    }

    pddby_topics_free(topics);
    free(sections_data);

    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode questions");

    if (topics)
    {
        pddby_topics_free(topics);
    }
    if (sections_data)
    {
        free(sections_data);
    }
    if (sections)
    {
        pddby_sections_free(sections);
    }

    return 0;
}
