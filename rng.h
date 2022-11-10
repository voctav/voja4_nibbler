#ifndef _RNG_H
#define _RNG_H

#include <stdint.h>

#define RNG_USE_RANDOM_SEED	0xf

struct rng_state {
	uint32_t seed;
};

/* Initializes the PRNG state and returns the first number in the sequence. */
uint8_t init_rng(struct rng_state *rng);

/* Resets the PRNG seed and returns the first number in the sequence. */
uint8_t set_rng_seed(struct rng_state *rng, uint8_t seed);

/* Gets the next number in the sequence from the PRNG. */
uint8_t next_rng(struct rng_state *rng);

#endif /* _RNG_H */
