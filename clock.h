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

#ifndef _CLOCK_H
#define _CLOCK_H

#include <time.h>

/* Measures time elapsed in nanoseconds since a given reference time. */
typedef long long vm_clock_t;

/* Returns time from a monotonic clock. */
void get_time(struct timespec *out);

/* Returns a clock value measured from the given reference. */
vm_clock_t get_vm_clock(const struct timespec *ref);

/* Returns time elapsed as microseconds. */
long vm_clock_as_usec(vm_clock_t clk);


#endif /* _CLOCK_H */
