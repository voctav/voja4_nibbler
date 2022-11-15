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

#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint8_t HEADER_MAGIC[] = {0x00, 0xff, 0x00, 0xff, 0xa5, 0xc3};

uint16_t read_protocol_word(void *buffer) {
	uint8_t *ptr = (uint8_t *) buffer;
	return ptr[0] | (ptr[1] << 8);
}

struct program *load_program(void *buffer, size_t size) {
	if (size < sizeof(HEADER_MAGIC) + 4) {
		fprintf(stderr, "Buffer size too small: %zu < %zu.\n", size, sizeof(HEADER_MAGIC) + 4);
		return NULL;
	}

	uint8_t *ptr = (uint8_t *) buffer;

	if (memcmp(ptr, HEADER_MAGIC, sizeof(HEADER_MAGIC))) {
		fprintf(stderr, "Invalid magic: %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx.\n",
				ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
		return NULL;
	}

	ptr += sizeof(HEADER_MAGIC);
	uint16_t length = read_protocol_word(ptr);
	ptr += sizeof(uint16_t);

	if (size != sizeof(HEADER_MAGIC) + 4 + length * 2) {
		fprintf(stderr, "Buffer size inconsistent with program length: %zu != %zu.\n", size, sizeof(HEADER_MAGIC) + 4 + length);
		return NULL;
	}

	struct program *prg = calloc(1, sizeof(struct program));
	memcpy(&prg->header, buffer, sizeof(HEADER_MAGIC));
	prg->length = length;

	uint16_t computed_checksum = length;
	for (int i = 0; i < length; i++) {
		program_word_t pi = read_protocol_word(ptr);
		ptr += sizeof(uint16_t);
		computed_checksum += pi;
		prg->instructions[i] = pi;
	}

	uint16_t checksum = read_protocol_word(ptr);
	if (computed_checksum != checksum) {
		free(prg);
		fprintf(stderr, "Bad checksum: computed %04x, expected %04x.\n", computed_checksum, checksum);
		return NULL;
	}

	prg->checksum = checksum;

	return prg;
}

void *read_file(char *path, size_t *size)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror(path);
		exit(EXIT_FAILURE);
	}

	fseek(f, 0L, SEEK_END);
	*size = ftell(f);
	fseek(f, 0L, SEEK_SET);

	void *buf = malloc(*size);
	if (!buf) {
		fclose(f);
		fprintf(stderr, "Error allocating memory for file.\n");
		exit(EXIT_FAILURE);
	}

	size_t count = fread(buf, 1, *size, f);
	if (count != *size) {
		free(buf);
		fclose(f);
		fprintf(stderr, "Short read: %zu < %zu.\n", count, *size);
		exit(EXIT_FAILURE);
	}

	fclose(f);

	return buf;
}
