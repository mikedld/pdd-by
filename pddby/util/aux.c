#include "aux.h"

#include <assert.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

char* pddby_aux_build_filename(char const* first_part, ...)
{
    assert(first_part);

    char* result = strdup(first_part);
    if (!result)
    {
        // TODO: report error
        return 0;
    }

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
        if (!result)
        {
            // TODO: report error
            return 0;
        }

        strcat(result, "/");
        strcat(result, part);
    }
    va_end(args);
    return result;
}

char* pddby_aux_path_get_basename(char const* path)
{
    assert(path);

    size_t length = strlen(path);
    size_t result_length = 0;
    while (length > 0 && path[length - 1] != '/')
    {
        length--;
        result_length++;
    }

    char* result = malloc(result_length + 1);
    if (!result)
    {
        // TODO: report error
        return 0;
    }

    strncpy(result, path + length, result_length + 1);
    return result;
}

int pddby_aux_file_get_contents(char const* filename, char** buffer, size_t* buffer_size)
{
    assert(filename);
    assert(buffer);

    *buffer = 0;

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        // TODO: report error
        return 0;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1)
    {
        // TODO: report error
        goto error;
    }

    if (buffer_size)
    {
        *buffer_size = file_size;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        // TODO: report error
        goto error;
    }

    *buffer = malloc(file_size + 1);
    if (!*buffer)
    {
        // TODO: report error
        goto error;
    }

    if (read(fd, *buffer, file_size) == -1)
    {
        // TODO: report error
        goto error;
    }

    if (close(fd) == -1)
    {
        // TODO: report warning
    }

    (*buffer)[file_size] = '\0';
    return 1;

error:
    if (*buffer)
    {
        free(*buffer);
    }

    if (fd != -1)
    {
        if (close(fd) == -1)
        {
            // TODO: report warning
        }
    }

    return 0;
}

int32_t pddby_aux_random_int_range(int32_t begin, int32_t end)
{
    return begin + rand() * (double)(end - begin) / RAND_MAX;
}
