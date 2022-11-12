/*
 * Nibbler - Emulator for Voja's 4-bit processor.
 * Eats nibbles for breakfast.
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
#include "vm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void output_usage(const char* executable_name) {
	fprintf(stderr, "Nibbler - VM for Voja's 4-bit processor. Eats nibbles for breakfast.\n");
	fprintf(stderr, "Usage: %s <[-s]> <file.bin>\n", executable_name);
	fprintf(stderr, "  -s: start in step mode, default is to start running\n");
}

int main(int argc, char *argv[]) {
	int opt;
	int step_mode = 0;
	while ((opt = getopt(argc, argv, "s")) != -1) {
		switch (opt) {
		case 's':
			step_mode = 1;
			break;
		default:
			output_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		output_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	size_t size;
	void *buf = read_file(argv[optind], &size);
	struct program *prg = load_program(buf, size);
	if (!prg) {
		exit(1);
	}
	vm_execute(prg, step_mode);
	free(prg);

	return 0;
}
