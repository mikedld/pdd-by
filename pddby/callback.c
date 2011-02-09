#include "callback.h"

#include <stdio.h>
#include <stdlib.h>

void pddby_report_error(char const* message, ...)
{
    va_list args;
    vfprintf(stderr, message, args);
    exit(1);
}
