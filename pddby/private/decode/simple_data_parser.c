#include "simple_data_parser.h"

#include "config.h"
#include "private/pddby.h"
#include "private/platform.h"
#include "private/util/regex.h"
#include "private/util/report.h"
#include "private/util/string.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

struct pddby_simple_data_parser
{
    pddby_t* pddby;

    pddby_regex_t* simple_data_regex;
    size_t markup_regexes_size;
    pddby_regex_t** markup_regexes;
};

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
        PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
    },
    {
        "^\\^R\\^(.+?)$\\s*",
        "<span underline='single' underline_color='#cc0000'><b>\\1</b></span>",
        PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
    },
    {
        "^\\^G\\^(.+?)$\\s*",
        "<span underline='single' underline_color='#00cc00'><b>\\1</b></span>",
        PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
    },
    {
        "^\\^B\\^(.+?)$\\s*",
        "<span underline='single' underline_color='#0000cc'><b>\\1</b></span>",
        PDDBY_REGEX_MULTILINE | PDDBY_REGEX_NEWLINE_ANY
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

pddby_simple_data_parser_t* pddby_simple_data_parser_new(pddby_t* pddby)
{
    pddby_simple_data_parser_t* parser = malloc(sizeof(pddby_simple_data_parser_t));
    if (!parser)
    {
        goto error;
    }

    parser->pddby = pddby;

    parser->simple_data_regex = pddby_regex_new(pddby, "^#(\\d+)\\s*((?:&[a-zA-Z0-9_-]+\\s*)*)(.+)$", PDDBY_REGEX_DOTALL);
    if (!parser->simple_data_regex)
    {
        goto error;
    }

    parser->markup_regexes_size = sizeof(s_markup_regexes_data) / sizeof(*s_markup_regexes_data);
    parser->markup_regexes = calloc(parser->markup_regexes_size, sizeof(pddby_regex_t*));
    if (!parser->markup_regexes)
    {
        goto error;
    }

    for (size_t i = 0; i < parser->markup_regexes_size; i++)
    {
        parser->markup_regexes[i] = pddby_regex_new(pddby, s_markup_regexes_data[i].regex_text,
            s_markup_regexes_data[i].regex_options);
        if (!parser->markup_regexes[i])
        {
            goto error;
        }
    }

    return parser;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to create simple data parser");

    if (parser)
    {
        pddby_simple_data_parser_free(parser);
    }

    return NULL;
}

char* pddby_simple_data_parser_parse(pddby_simple_data_parser_t* parser, char const* text, int32_t* number,
    char*** image_names)
{
    char* number_part = NULL;
    char* image_names_part = NULL;
    char* text_part = NULL;

    pddby_regex_match_t* match;
    if (!pddby_regex_match(parser->simple_data_regex, text, &match))
    {
        goto error;
    }

    number_part = pddby_string_chomp(pddby_regex_match_fetch(match, 1));
    if (!number_part)
    {
        pddby_regex_match_free(match);
        goto error;
    }

    image_names_part = pddby_string_chomp(pddby_regex_match_fetch(match, 2));
    if (!image_names_part)
    {
        pddby_regex_match_free(match);
        goto error;
    }

    text_part = pddby_string_chomp(pddby_regex_match_fetch(match, 3));
    if (!text_part)
    {
        pddby_regex_match_free(match);
        goto error;
    }

    pddby_regex_match_free(match);

    for (size_t i = 0; i < parser->markup_regexes_size; i++)
    {
        char* new_text_part = pddby_regex_replace(parser->markup_regexes[i], text_part,
            s_markup_regexes_data[i].replacement_text);
        if (!new_text_part)
        {
            goto error;
        }

        free(text_part);
        text_part = new_text_part;
    }

    if (image_names && *pddby_string_chomp(image_names_part))
    {
        *image_names = pddby_string_split(parser->pddby, image_names_part, "\n");
        if (!*image_names)
        {
            goto error;
        }

        char** in = *image_names;
        while (*in)
        {
            pddby_string_chomp(*in + 1);
            memmove(*in, *in + 1, strlen(*in));
            in++;
        }
    }

    if (number)
    {
        *number = atoi(number_part);
    }

    free(image_names_part);
    free(number_part);

    return pddby_string_chomp(text_part);

error:
    pddby_report(parser->pddby, pddby_message_type_error, "unable to parse simple data");

    if (number)
    {
        *number = 0;
    }

    if (image_names)
    {
        pddby_stringv_free(*image_names);
        *image_names = NULL;
    }

    if (text_part)
    {
        free(text_part);
    }
    if (image_names_part)
    {
        free(image_names_part);
    }
    if (number_part)
    {
        free(number_part);
    }

    return NULL;
}

void pddby_simple_data_parser_free(pddby_simple_data_parser_t* parser)
{
    assert(parser);

    if (parser->markup_regexes)
    {
        for (size_t i = 0; i < parser->markup_regexes_size; i++)
        {
            if (parser->markup_regexes[i])
            {
                pddby_regex_free(parser->markup_regexes[i]);
            }
        }
        free(parser->markup_regexes);
    }
    if (parser->simple_data_regex)
    {
        pddby_regex_free(parser->simple_data_regex);
    }

    free(parser);
}
