#ifndef PDDBY_IMAGE_H
#define PDDBY_IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"

#include <stdint.h>

struct pddby_image_s
{
    int64_t id;
    char* name;
    void* data;
    size_t data_length;
};

typedef struct pddby_image_s pddby_image_t;
typedef pddby_array_t pddby_images_t;

pddby_image_t* pddby_image_new(char const* name, void const* data, size_t data_length);
void pddby_image_free(pddby_image_t* image);

int pddby_image_save(pddby_image_t* image);

pddby_image_t* pddby_image_find_by_id(int64_t id);
pddby_image_t* pddby_image_find_by_name(char const* name);

pddby_images_t* pddby_image_find_by_traffreg(int64_t traffreg_id);
pddby_images_t* pddby_image_copy_all(pddby_images_t const* images);
void pddby_image_free_all(pddby_images_t* images);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_IMAGE_H
