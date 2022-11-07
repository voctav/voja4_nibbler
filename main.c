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

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Nibbler - VM for Voja's 4-bit processor. Eats nibbles for breakfast.\n");
		fprintf(stderr, "Usage: %s <file.bin>\n", argv[0]);
		exit(1);
	}
	size_t size;
	void *buf = read_file(argv[1], &size);
	struct program *prg = load_program(buf, size);
	if (!prg) {
		exit(1);
	}
	vm_execute(prg);
	free(prg);

	return 0;
}
