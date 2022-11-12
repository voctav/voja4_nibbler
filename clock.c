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

#include "clock.h"

#include <time.h>

const unsigned long long NSEC_PER_SEC = 1000000000;

void get_time(struct timespec *out)
{
	clock_gettime(CLOCK_MONOTONIC, out);
}

void timespec_subtract(const struct timespec *x, const struct timespec *y, struct timespec *result)
{
	long sec = 0;
	if (x->tv_nsec < y->tv_nsec) {
		sec = (y->tv_nsec - x->tv_nsec + NSEC_PER_SEC - 1) / NSEC_PER_SEC;
	}
	result->tv_sec = x->tv_sec - (y->tv_sec + sec);
	result->tv_nsec = x->tv_nsec - (y->tv_nsec - NSEC_PER_SEC * sec);
}

vm_clock_t get_vm_clock(const struct timespec *ref)
{
	struct timespec now;
	get_time(&now);
	struct timespec diff;
	timespec_subtract(&now, ref, &diff);
	return diff.tv_nsec + diff.tv_sec * NSEC_PER_SEC;
}

long vm_clock_as_usec(vm_clock_t clk)
{
	return clk / 1000;
}
