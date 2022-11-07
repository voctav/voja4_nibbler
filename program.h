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

/* In memory representation of a program, based on serial protocol. */
struct program {
	uint8_t header[HEADER_MAGIC_SIZE];
	uint16_t length;
	uint16_t checksum;
	program_word_t instructions[0];  /* Memory allocated dynamically. */
};

struct program *load_program(void *buffer, size_t size);

void *read_file(char *path, size_t *size);

#endif /* _PROGRAM_H */
