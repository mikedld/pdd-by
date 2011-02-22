#ifndef PDDBY_DECODE_IMAGE_H
#define PDDBY_DECODE_IMAGE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

int decode_image(char const* path, uint16_t magic);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_DECODE_IMAGE_H
