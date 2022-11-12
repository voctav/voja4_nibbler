/*
 * Nibbler - Emulator for Voja's 4-bit processor.
 *
 * Copyright (c) 2022 Octavian Voicu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
