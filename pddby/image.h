#ifndef PDDBY_IMAGE_H
#define PDDBY_IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "array.h"
#include "pddby.h"

#include <stdint.h>

struct pddby_image
{
    pddby_t* pddby;

    int64_t id;
    char* name;
    void* data;
    size_t data_length;
};

typedef struct pddby_image pddby_image_t;
typedef pddby_array_t pddby_images_t;

pddby_image_t* pddby_image_new(pddby_t* pddby, char const* name, void const* data, size_t data_length);
void pddby_image_free(pddby_image_t* image);

int pddby_image_save(pddby_image_t* image);

pddby_image_t* pddby_image_find_by_id(pddby_t* pddby, int64_t id);
pddby_image_t* pddby_image_find_by_name(pddby_t* pddby, char const* name);

pddby_images_t* pddby_images_new(pddby_t* pddby);
pddby_images_t* pddby_images_find_by_traffreg(pddby_t* pddby, int64_t traffreg_id);
void pddby_images_free(pddby_images_t* images);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_IMAGE_H
