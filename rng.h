#ifndef _RNG_H
#define _RNG_H

#include <stdint.h>

struct rng_state {
	uint32_t seed;
};

void init_rng(struct rng_state *rng);

void set_rng_seed(struct rng_state *rng, uint32_t seed);

uint32_t next_rng(struct rng_state *rng);

#endif /* _RNG_H */
