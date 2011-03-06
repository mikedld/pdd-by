#ifndef PDDBY_PRIVATE_DECODE_IMAGE_H
#define PDDBY_PRIVATE_DECODE_IMAGE_H

#include "pddby.h"

#include <stdint.h>

int pddby_decode_image(pddby_t* pddby, char const* path, uint16_t magic);

#endif // PDDBY_PRIVATE_DECODE_IMAGE_H
