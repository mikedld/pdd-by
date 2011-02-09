#ifndef PDDBY_DELPHI_H
#define PDDBY_DELPHI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

uint32_t delphi_random(uint32_t limit);

void set_randseed(uint64_t seed);
uint64_t get_randseed();
void init_randseed_for_image(char const* name, uint16_t magic);

#ifdef __cplusplus
}
#endif

#endif // PDDBY_DELPHI_H
