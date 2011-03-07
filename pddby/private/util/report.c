#include "report.h"

#include "private/pddby.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static void print_prefix(int err_no, int type)
{
    char type_char = '?';
    switch (type)
    {
    case pddby_message_type_debug:
        type_char = 'D';
        break;
    case pddby_message_type_log:
        type_char = 'L';
        break;
    case pddby_message_type_warning:
        type_char = 'W';
        break;
    case pddby_message_type_error:
        type_char = 'E';
        break;
    }

    time_t current_time = time(NULL);
    struct tm tm = *localtime(&current_time);

    printf("[%04d-%02d-%02d %02d:%02d:%02d] [%c] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
        tm.tm_min, tm.tm_sec, type_char);

    if (type > pddby_message_type_log && err_no)
    {
        printf("[%d | %s] ", err_no, strerror(err_no));
    }
}

void pddby_report(pddby_t* pddby, int type, char const* text, ...)
{
    assert(pddby);

    va_list args;
    va_start(args, text);

    if (pddby->callbacks && pddby->callbacks->message)
    {
        char* buffer;
        if (vasprintf(&buffer, text, args) != -1)
        {
            pddby->callbacks->message(pddby, type, buffer);
            free(buffer);
        }
    }
    else
    {
        print_prefix(errno, type);
        vprintf(text, args);
        printf("\n");
    }

    va_end(args);

    errno = 0;
}

void pddby_report_progress_begin(pddby_t* pddby, int size)
{
    assert(pddby);

    if (pddby->callbacks && pddby->callbacks->progress_begin)
    {
        pddby->callbacks->progress_begin(pddby, size);
    }
}

void pddby_report_progress(pddby_t* pddby, int pos)
{
    assert(pddby);

    if (pddby->callbacks && pddby->callbacks->progress)
    {
        pddby->callbacks->progress(pddby, pos);
    }
}

void pddby_report_progress_end(pddby_t* pddby)
{
    assert(pddby);

    if (pddby->callbacks && pddby->callbacks->progress_end)
    {
        pddby->callbacks->progress_end(pddby);
    }
}
