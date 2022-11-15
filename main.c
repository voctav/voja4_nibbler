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
#include "ui.h"
#include "vm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void output_usage(const char* executable_name) {
	fprintf(stderr, "Nibbler - VM for Voja's 4-bit processor. Eats nibbles for breakfast.\n");
	fprintf(stderr, "Usage: %s [-s] [-r] <file.bin>\n", executable_name);
	fprintf(stderr, "  -s: start in step mode, default is to start running\n");
	fprintf(stderr, "  -r: use red for page display to simulate LED color, default is gray\n");
}

int main(int argc, char *argv[]) {
	int opt;
	int ui_options = 0;
	while ((opt = getopt(argc, argv, "sr")) != -1) {
		switch (opt) {
		case 's':
			ui_options |= STEP_MODE;
			break;
		case 'r':
			ui_options |= RED_MODE;
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
	free(buf);
	if (!prg) {
		exit(EXIT_FAILURE);
	}
	vm_execute(prg, ui_options);
	free(prg);

	return EXIT_SUCCESS;
}
