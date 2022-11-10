#include "rng.h"

#include <stdlib.h>
#include <sys/random.h>

/*
 * There was a bug in the firmware implementation that used a different constant
 * for the PRNG than documented. This was fixed in revision 4. You can activate
 * the old PRNG behavior by compiling with -DFIRMWARE_R3.
 */
#ifdef FIRMWARE_R3
const uint32_t RNG_A = 0x838c4e6d; /* Firmware revision 3 and older. */
#else /* FIRMWARE_R3 */
const uint32_t RNG_A = 0x41c64e6d; /* Firmware revision 4. */
#endif /* FIRMWARE_R3 */
const uint32_t RNG_C = 0x6073;

uint8_t init_rng(struct rng_state *rng)
{
	return set_rng_seed(rng, RNG_USE_RANDOM_SEED);
}

/* Expands a 4 bit seed to a 32 bit seed. */
uint32_t seed_from_nibble(uint8_t nibble)
{
	uint32_t seed = nibble;
	seed = (seed << 4) | seed;
	seed = (seed << 8) | seed;
	seed = (seed << 16) | seed;
	return seed;
}

/* Scrambles a 32 bit pseudorandom number into a 4 bit pseudorandom number. */
uint8_t seed_to_nibble(uint32_t seed)
{
	seed = (seed >> 16) ^ (seed & 0xffff);
	seed = ((seed >> 8) + (seed & 0xff)) & 0xff;
	seed = (seed >> 4) ^ (seed & 0xf);
	return seed;
}

/*
 * Sets the seed for the random number generator.
 * The seed must be a nibble, which will be duplicated 8 times to expand to 32
 * bits. The special value 0xf causes all 32 bits of the seed to be initialized
 * from a random source.
 */
uint8_t set_rng_seed(struct rng_state *rng, uint8_t seed)
{
	if (seed == RNG_USE_RANDOM_SEED) {
		getrandom(&rng->seed, sizeof(rng->seed), 0 /* flags */);
	} else {
		rng->seed = seed_from_nibble(seed);
	}
	return seed_to_nibble(rng->seed);
}

/*
 * Returns the next 4 bit pseudorandom number based on an internal 32 bit state.
 * This is a 32 bit congruential pseudorandom number generator with some
 * additional scrambling to transform the generated 32 bit number to 4 bits
 * with higher entropy than if simply taking the low order 4 bits.
 * This matches the behavior of the badge generator.
 */
uint8_t next_rng(struct rng_state *rng)
{
	uint32_t rnd = RNG_A * rng->seed + RNG_C;
	rng->seed = rnd;
	return seed_to_nibble(rnd);
}

