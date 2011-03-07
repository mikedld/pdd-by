#ifndef PDDBY_PRIVATE_AUX_H
#define PDDBY_PRIVATE_AUX_H

#include "pddby.h"

#include <stddef.h>
#include <stdint.h>

char* pddby_aux_build_filename(pddby_t* pddby, char const* first_part, ...);
char* pddby_aux_build_filename_ci(pddby_t* pddby, char const* first_part, ...);
char* pddby_aux_path_get_basename(pddby_t* pddby, char const* path);
int pddby_aux_file_get_contents(pddby_t* pddby, char const* filename, char** buffer, size_t* buffer_size);
char* pddby_aux_file_get_checksum(pddby_t* pddby, char const* file_path);

int32_t pddby_aux_random_int_range(int32_t begin, int32_t end);

#endif // PDDBY_PRIVATE_AUX_H
