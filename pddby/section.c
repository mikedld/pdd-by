#include "section.h"

#include "config.h"
#include "private/util/database.h"
#include "question.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static pddby_section_t* pddby_section_new_with_id(pddby_t* pddby, int64_t id, char const* name, char const* title_prefix,
    char const* title)
{
    pddby_section_t* section = calloc(1, sizeof(pddby_section_t));
    if (!section)
    {
        goto error;
    }

    section->name = name ? strdup(name) : NULL;
    if (name && !section->name)
    {
        goto error;
    }

    section->title_prefix = title_prefix ? strdup(title_prefix) : NULL;
    if (title_prefix && !section->title_prefix)
    {
        goto error;
    }

    section->title = title ? strdup(title) : NULL;
    if (title && !section->title)
    {
        goto error;
    }

    section->id = id;
    section->pddby = pddby;

    return section;

error:
    // TODO: report error
    return NULL;
}

pddby_section_t* pddby_section_new(pddby_t* pddby, char const* name, char const* title_prefix, char const* title)
{
    return pddby_section_new_with_id(pddby, 0, name, title_prefix, title);
}

void pddby_section_free(pddby_section_t* section)
{
    assert(section);

    if (section->name)
    {
        free(section->name);
    }
    if (section->title_prefix)
    {
        free(section->title_prefix);
    }
    if (section->title)
    {
        free(section->title);
    }
    free(section);
}

int pddby_section_save(pddby_section_t* section)
{
    assert(section);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(section->pddby, "INSERT INTO `sections` (`name`, `title_prefix`, `title`) VALUES (?, ?, ?)");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_text(db_stmt, 1, section->name) ||
        !pddby_db_bind_text(db_stmt, 2, section->title_prefix) ||
        !pddby_db_bind_text(db_stmt, 3, section->title))
    {
        goto error;
    }

    int ret = pddby_db_step(db_stmt);
    if (ret == -1)
    {
        goto error;
    }

    assert(ret == 0);

    section->id = pddby_db_last_insert_id(section->pddby);

    return 1;

error:
    // TODO: report error
    return 0;
}

pddby_section_t* pddby_section_find_by_id(pddby_t* pddby, int64_t id)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `name`, `title_prefix`, `title` FROM `sections` WHERE `rowid`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, id))
    {
        goto error;
    }

    switch (pddby_db_step(db_stmt))
    {
    case -1:
        goto error;
    case 0:
        return NULL;
    }

    char const* name = pddby_db_column_text(db_stmt, 0);
    char const* title_prefix = pddby_db_column_text(db_stmt, 1);
    char const* title = pddby_db_column_text(db_stmt, 2);

    return pddby_section_new_with_id(pddby, id, name, title_prefix, title);

error:
    // TODO: report error
    return NULL;
}

pddby_section_t* pddby_section_find_by_name(pddby_t* pddby, char const* name)
{
    assert(name);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `title_prefix`, `title` FROM `sections` WHERE `name`=? LIMIT 1");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_text(db_stmt, 1, name))
    {
        goto error;
    }

    switch (pddby_db_step(db_stmt))
    {
    case -1:
        goto error;
    case 0:
        return NULL;
    }

    int64_t id = pddby_db_column_int64(db_stmt, 0);
    char const* title_prefix = pddby_db_column_text(db_stmt, 1);
    char const* title = pddby_db_column_text(db_stmt, 2);

    return pddby_section_new_with_id(pddby, id, name, title_prefix, title);

error:
    // TODO: report error
    return NULL;
}

pddby_sections_t* pddby_sections_new(pddby_t* pddby)
{
    return pddby_array_new(pddby, (pddby_array_free_func_t)pddby_section_free);
}

pddby_sections_t* pddby_sections_find_all(pddby_t* pddby)
{
    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(pddby, "SELECT `rowid`, `name`, `title_prefix`, `title` FROM `sections`");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt))
    {
        goto error;
    }

    pddby_sections_t* sections = pddby_sections_new(pddby);
    if (!sections)
    {
        goto error;
    }

    int ret;
    while ((ret = pddby_db_step(db_stmt)) == 1)
    {
        int64_t id = pddby_db_column_int64(db_stmt, 0);
        char const* name = pddby_db_column_text(db_stmt, 1);
        char const* title_prefix = pddby_db_column_text(db_stmt, 2);
        char const* title = pddby_db_column_text(db_stmt, 3);

        if (!pddby_array_add(sections, pddby_section_new_with_id(pddby, id, name, title_prefix, title)))
        {
            ret = -1;
            break;
        }
    }

    if (ret == -1)
    {
        pddby_sections_free(sections);
        goto error;
    }

    return sections;

error:
    // TODO: report error
    return NULL;
}

void pddby_sections_free(pddby_sections_t* sections)
{
    assert(sections);

    pddby_array_free(sections, 1);
}

size_t pddby_section_get_question_count(pddby_section_t* section)
{
    assert(section);

    static pddby_db_stmt_t* db_stmt = NULL;
    if (!db_stmt)
    {
        db_stmt = pddby_db_prepare(section->pddby, "SELECT COUNT(*) FROM `questions_sections` WHERE `section_id`=?");
        if (!db_stmt)
        {
            goto error;
        }
    }

    if (!pddby_db_reset(db_stmt) ||
        !pddby_db_bind_int64(db_stmt, 1, section->id))
    {
        goto error;
    }

    if (pddby_db_step(db_stmt) != 1)
    {
        goto error;
    }

    return pddby_db_column_int(db_stmt, 0);

error:
    // TODO: report error
    return 0;
}
