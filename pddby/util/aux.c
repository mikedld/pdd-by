#include "aux.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* pddby_aux_build_filename(char const* first_part, ...)
{
    char* result = strdup(first_part);
    va_list args;
    va_start(args, first_part);
    for (;;)
    {
        char const* part = va_arg(args, char const*);
        if (!part)
        {
            break;
        }
        result = realloc(result, strlen(result) + strlen(part) + 2);
        strcat(result, "/");
        strcat(result, part);
    }
    va_end(args);
    return result;
}

char* pddby_aux_path_get_basename(char const* s)
{
    size_t length = strlen(s);
    size_t result_length = 0;
    while (length > 0 && s[length - 1] != '/')
    {
        length--;
        result_length++;
    }
    char* result = malloc(result_length + 1);
    strncpy(result, s + length, result_length + 1);
    return result;
}

char* pddby_aux_get_user_cache_dir()
{
    return "/tmp";
}

int pddby_aux_file_get_contents(char const* filename, char** buffer, size_t* buffer_size)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }
    *buffer_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    *buffer = malloc(*buffer_size);
    if (read(fd, *buffer, *buffer_size) == -1)
    {
        free(*buffer);
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
}

int32_t pddby_aux_random_int_range(int32_t begin, int32_t end)
{
    return begin + (int64_t)rand() * (end - begin) / RAND_MAX;
}
