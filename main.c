/*
 * Nibbler - Emulator for Voja's 4-bit processor.
 * Eats nibbles for breakfast.
 *
 * See https://hackaday.io/project/182568-badge-for-supercon6-november-2022/discussion-181045.
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
