#ifndef PDDBY_PRIVATE_DELPHI_H
#define PDDBY_PRIVATE_DELPHI_H

#include <stdint.h>

uint32_t delphi_random(uint32_t limit);

void set_randseed(uint64_t seed);
uint64_t get_randseed();

#endif // PDDBY_PRIVATE_DELPHI_H
