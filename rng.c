#include "rng.h"

#include <stdlib.h>
#include <time.h>

const uint32_t RNG_A = 0x41C64E6D;
const uint32_t RNG_C = 0x6073;

void init_rng(struct rng_state *rng)
{
	srand(time(NULL));
	rng->seed = rand();
}

void set_rng_seed(struct rng_state *rng, uint32_t seed)
{
	rng->seed = seed;
}

uint32_t next_rng(struct rng_state *rng)
{
	uint32_t ret = rng->seed;
	rng->seed = RNG_A * rng->seed + RNG_C;
	return ret;
}

