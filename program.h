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

#ifndef _PROGRAM_H
#define _PROGRAM_H

#include <stddef.h>
#include <stdint.h>

/* Type of a program word. This is 12 bits on the actual hardware. */
typedef uint16_t program_word_t;

/* Address of a word in program memory as offset in words from the beginning. */
typedef uint16_t program_addr_t;

#define PROGRAM_MEMORY_SIZE 4096
#define HEADER_MAGIC_SIZE   6

/* In memory representation of a program, based on the serial protocol. */
struct program {
	uint8_t header[HEADER_MAGIC_SIZE];
	/* Number of instructions excluding zero memory beyond the loaded program. */
	uint16_t length;
	uint16_t checksum;
	program_word_t instructions[PROGRAM_MEMORY_SIZE];
};

struct program *load_program(void *buffer, size_t size);

void *read_file(char *path, size_t *size);

#endif /* _PROGRAM_H */
