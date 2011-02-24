#include "aux.h"

#include "report.h"

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static char* pddby_aux_find_file_ci(pddby_t* pddby, char const* path, char const* fname)
{
    DIR* dir = opendir(path);
    if (!dir)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to find file \"%s\" inside \"%s\"", fname, path);
        return NULL;
    }

    char* result = NULL;
    struct dirent* ent;
    while ((ent = readdir(dir)))
    {
        if (!strcasecmp(ent->d_name, fname))
        {
            result = pddby_aux_build_filename(pddby, path, ent->d_name, 0);
            break;
        }
    }
    closedir(dir);

    if (!result)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to find file \"%s\" inside \"%s\"", fname, path);
    }

    return result;
}

char* pddby_aux_build_filename(pddby_t* pddby, char const* first_part, ...)
{
    assert(first_part);

    char* result = strdup(first_part);
    if (!result)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to build filename");
        return NULL;
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
            pddby_report(pddby, pddby_message_type_error, "unable to build filename");
            return NULL;
        }

        strcat(result, "/");
        strcat(result, part);
    }
    va_end(args);

    return result;
}

char* pddby_aux_build_filename_ci(pddby_t* pddby, char const* first_part, ...)
{
    char* result = strdup(first_part);
    if (!result)
    {
        pddby_report(pddby, pddby_message_type_error, "unable to build filename (case-insensitive)");
        return NULL;
    }

    va_list list;
    va_start(list, first_part);
    char const* path_part;
    while ((path_part = va_arg(list, char const*)))
    {
        char* path = pddby_aux_find_file_ci(pddby, result, path_part);
        free(result);
        if (!path)
        {
            pddby_report(pddby, pddby_message_type_error, "unable to build filename (case-insensitive)");
            return NULL;
        }

        result = path;
    }
    va_end(list);

    pddby_report(pddby, pddby_message_type_debug, "path (ci): %s", result);

    return result;
}

char* pddby_aux_path_get_basename(pddby_t* pddby, char const* path)
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
        pddby_report(pddby, pddby_message_type_error, "unable to get base name of \"%s\"", path);
        return NULL;
    }

    strncpy(result, path + length, result_length + 1);
    return result;
}

int pddby_aux_file_get_contents(pddby_t* pddby, char const* filename, char** buffer, size_t* buffer_size)
{
    assert(filename);
    assert(buffer);

    *buffer = 0;

    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        goto error;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1)
    {
        goto error;
    }

    if (buffer_size)
    {
        *buffer_size = file_size;
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        goto error;
    }

    *buffer = malloc(file_size + 1);
    if (!*buffer)
    {
        goto error;
    }

    if (read(fd, *buffer, file_size) == -1)
    {
        goto error;
    }

    if (close(fd) == -1)
    {
        pddby_report(pddby, pddby_message_type_warning, "unable to close file %d", fd);
    }

    (*buffer)[file_size] = '\0';
    return 1;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to get contents of \"%s\"", filename);

    if (*buffer)
    {
        free(*buffer);
    }

    if (fd != -1)
    {
        if (close(fd) == -1)
        {
            pddby_report(pddby, pddby_message_type_warning, "unable to close file %d", fd);
        }
    }

    return 0;
}

char* pddby_aux_file_get_checksum(pddby_t* pddby, char const* file_path)
{
    assert(file_path);

    uint8_t* buffer = NULL;

    FILE* f = fopen(file_path, "rb");
    if (!f)
    {
        goto error;
    }

    MD5_CTX md5ctx;
    MD5_Init(&md5ctx);

    buffer = malloc(32 * 1024);
    if (!buffer)
    {
        goto error;
    }

    do
    {
        size_t bytes_read = fread(buffer, 1, 32 * 1024, f);
        if (!bytes_read)
        {
            if (feof(f))
            {
                break;
            }
            goto error;
        }
        MD5_Update(&md5ctx, buffer, bytes_read);
    }
    while (!feof(f));

    uint8_t md5sum[MD5_DIGEST_LENGTH];
    MD5_Final(md5sum, &md5ctx);

    char* result = malloc(MD5_DIGEST_LENGTH * 2 + 1);
    if (!result)
    {
        goto error;
    }

    for (size_t i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        if (sprintf(result + i * 2, "%02x", md5sum[i]) != 2)
        {
            goto error;
        }
    }

    free(buffer);
    if (fclose(f) == EOF)
    {
        pddby_report(pddby, pddby_message_type_warning, "unable to close file %d", fileno(f));
    }

    return result;

error:
    pddby_report(pddby, pddby_message_type_error, "unable to get checksum of \"%s\"", file_path);

    if (buffer)
    {
        free(buffer);
    }

    if (f)
    {
        if (fclose(f) == EOF)
        {
            pddby_report(pddby, pddby_message_type_warning, "unable to close file %d", fileno(f));
        }
    }
    return NULL;
}

int32_t pddby_aux_random_int_range(int32_t begin, int32_t end)
{
    return begin + rand() * (double)(end - begin) / RAND_MAX;
}
