#ifndef PDDBY_AUX_H
#define PDDBY_AUX_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

char* pddby_aux_build_filename(char const* first_part, ...);
char* pddby_aux_path_get_basename(char const* s);
char* pddby_aux_get_user_cache_dir();
int pddby_aux_file_get_contents(char const* filename, char** buffer, size_t* buffer_size);

int32_t pddby_aux_random_int_range(int32_t begin, int32_t end);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_AUX_H
