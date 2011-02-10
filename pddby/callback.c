#include "callback.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void pddby_report_error(char const* message, ...)
{
    va_list args;
    va_start(args, message);
    fprintf(stderr, "error: ");
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}
