#ifndef PDDBY_PRIVATE_DECODE_ROLLING_STONES_H
#define PDDBY_PRIVATE_DECODE_ROLLING_STONES_H

#include "pddby.h"

#include <stdint.h>
#include <unistd.h>

struct pddby_rolling_stones;
typedef struct pddby_rolling_stones pddby_rolling_stones_t;

pddby_rolling_stones_t* pddby_rolling_stones_new(uint8_t* seed, size_t seed_size);
uint8_t pddby_rolling_stones_next(pddby_rolling_stones_t* ctx, off_t pos);
void pddby_rolling_stones_free(pddby_rolling_stones_t* ctx);

struct pddby_rolling_rocks;
typedef struct pddby_rolling_rocks pddby_rolling_rocks_t;

pddby_rolling_rocks_t* pddby_rolling_rocks_new(uint32_t seed);
uint8_t pddby_rolling_rocks_next(pddby_rolling_rocks_t* ctx);
void pddby_rolling_rocks_free(pddby_rolling_rocks_t* ctx);

#endif // PDDBY_PRIVATE_DECODE_ROLLING_STONES_H
