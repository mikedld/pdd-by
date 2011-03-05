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
    char** image_dirs = NULL;

    char* raw_image_dirs = pddby_settings_get(pddby, "image_dirs");
    if (!raw_image_dirs)
    {
        goto error;
    }

    image_dirs = pddby_string_split(pddby, raw_image_dirs, ":");
    free(raw_image_dirs);
    if (!image_dirs)
    {
        goto error;
    }

    if (!pddby_db_tx_begin(pddby))
    {
        goto error;
    }

    int result = 1;
    char** dir_name = image_dirs;
    while (*dir_name)
    {
        DIR* dir = NULL;

        char* images_path = pddby_aux_build_filename_ci(pddby, pddby->decode_context->root_path, *dir_name, 0);
        if (!images_path)
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

            char* image_path = pddby_aux_build_filename(pddby, images_path, ent->d_name, 0);
            if (!image_path)
            {
                goto cycle_error;
            }

            result = decode_image(pddby, image_path, pddby->decode_context->image_magic);
            free(image_path);
            if (!result)
            {
                break;
            }
        }

        free(images_path);

        if (closedir(dir) == -1)
        {
            pddby_report(pddby, pddby_message_type_warning, "unable to close directory \"%s\"", *dir_name);
        }

        if (!result)
        {
            break;
        }

        dir_name++;
        continue;

cycle_error:
        pddby_report(pddby, pddby_message_type_error, "unable to decode images in \"%s\"", *dir_name);

        if (images_path)
        {
            free(images_path);
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

    pddby_stringv_free(image_dirs);
    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode images");

    if (image_dirs)
    {
        pddby_stringv_free(image_dirs);
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
            table[i]--;
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
    struct markup_regex_data
    {
        char const* regex_text;
        char const* replacement_text;
        int regex_options;
    };
    static struct markup_regex_data const s_markup_regexes_data[] =
    {
        {
            "@(.+?)@",
            "<span underline='single' underline_color='#ff0000'>\\1</span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "^~\\s*.+?$\\s*",
            "",
            PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "^\\^R\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#cc0000'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "^\\^G\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#00cc00'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "^\\^B\\^(.+?)$\\s*",
            "<span underline='single' underline_color='#0000cc'><b>\\1</b></span>",
            PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "\\^R(.+?)\\^K",
            "<span color='#cc0000'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "\\^G(.+?)\\^K",
            "<span color='#00cc00'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "\\^B(.+?)\\^K",
            "<span color='#0000cc'><b>\\1</b></span>",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_DOTALL | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "-\\s*$\\s*",
            "",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "([^.> \t])\\s*$\\s*",
            "\\1 ",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
        },
        {
            "[ \t]{2,}",
            " ",
            PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
        }
    };

    size_t const markup_regexes_size = sizeof(s_markup_regexes_data) / sizeof(*s_markup_regexes_data);

    int32_t* table = NULL;
    char* str = NULL;
    pddby_regex_t* simple_data_regex = NULL;
    pddby_regex_t** markup_regexes = NULL;

    size_t table_size;
    table = pddby_decode_table(pddby, pddby->decode_context->data_magic, dat_path, &table_size);
    if (!table)
    {
        goto error;
    }

    size_t str_size;
    str = pddby->decode_context->decode_string(pddby->decode_context, dbt_path, &str_size, 0);
    if (!str)
    {
        goto error;
    }

    simple_data_regex = pddby_regex_new(pddby, "^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$", PDDBY_REGEX_DOTALL);
    if (!simple_data_regex)
    {
        goto error;
    }

    markup_regexes = calloc(markup_regexes_size, sizeof(pddby_regex_t*));
    if (!markup_regexes)
    {
        goto error;
    }

    for (size_t i = 0; i < markup_regexes_size; i++)
    {
        markup_regexes[i] = pddby_regex_new(pddby, s_markup_regexes_data[i].regex_text,
            s_markup_regexes_data[i].regex_options);
        if (!markup_regexes[i])
        {
            goto error;
        }
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
        char* number = NULL;
        char* images = NULL;
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
                goto cycle_error;
            }

            pddby_regex_match_t* match;
            if (!pddby_regex_match(simple_data_regex, data, &match))
            {
                free(data);
                goto cycle_error;
            }

            number = pddby_string_chomp(pddby_regex_match_fetch(match, 1));
            if (!number)
            {
                pddby_regex_match_free(match);
                free(data);
                goto cycle_error;
            }

            images = pddby_string_chomp(pddby_regex_match_fetch(match, 2));
            if (!images)
            {
                pddby_regex_match_free(match);
                free(data);
                goto cycle_error;
            }

            text = pddby_string_chomp(pddby_regex_match_fetch(match, 3));
            if (!text)
            {
                pddby_regex_match_free(match);
                free(data);
                goto cycle_error;
            }

            pddby_regex_match_free(match);
            free(data);

            if (*images)
            {
                char** image_names = pddby_string_split(pddby, images, "\n");
                if (!image_names)
                {
                    goto cycle_error;
                }

                char** in = image_names;
                while (*in)
                {
                    pddby_image_t* image = pddby_image_find_by_name(pddby, pddby_string_chomp(*in + 1));
                    if (!image)
                    {
                        pddby_stringv_free(image_names);
                        goto cycle_error;
                    }
                    if (!pddby_array_add(object_images, image))
                    {
                        pddby_stringv_free(image_names);
                        goto cycle_error;
                    }
                    in++;
                }
                pddby_stringv_free(image_names);
            }

            for (size_t j = 0; j < markup_regexes_size; j++)
            {
                char* new_text = pddby_regex_replace(markup_regexes[j], text,
                    s_markup_regexes_data[j].replacement_text);
                if (!new_text)
                {
                    goto cycle_error;
                }

                free(text);
                text = new_text;
            }

            object = object_new(pddby, atoi(number), pddby_string_chomp(text));

            free(text);
            free(images);
            free(number);

            number = images = text = NULL;
        }
        if (!object)
        {
            goto error;
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

        pddby_report_progress(pddby, i);
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
        if (images)
        {
            free(images);
        }
        if (number)
        {
            free(number);
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

    for (size_t i = 0; i < markup_regexes_size; i++)
    {
        pddby_regex_free(markup_regexes[i]);
    }

    free(markup_regexes);
    pddby_regex_free(simple_data_regex);
    free(str);
    free(table);

    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to decode simple data");

    if (markup_regexes)
    {
        for (size_t i = 0; i < markup_regexes_size; i++)
        {
            if (markup_regexes[i])
            {
                pddby_regex_free(markup_regexes[i]);
            }
        }
        free(markup_regexes);
    }
    if (simple_data_regex)
    {
        pddby_regex_free(simple_data_regex);
    }
    if (str)
    {
        free(str);
    }
    if (table)
    {
        free(table);
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
        pddby_topic_question_t* data = pddby_decode_topic_questions_table(pddby->decode_context, section_dat_path,
            &size);
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
