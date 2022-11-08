#ifndef _VM_H
#define _VM_H

#include "clock.h"
#include "program.h"
#include "rng.h"

#include <stdint.h>

#define PAGE_SIZE 0x10
#define NUM_PAGES 0x10

/* Bit masks for Flags internal register. */
enum {
	FLAG_CARRY    = 0x1,
	FLAG_ZERO     = 0x2,
	FLAG_OVERFLOW = 0x4,
};

/* Special function registers. */
enum {
	SFR_OUT = 0x0a,
	SFR_IN,
	SFR_JSR,
	SFR_PCL,
	SFR_PCM,
	SFR_PCH,

	SFR_FIRST = 0xf0,
	SFR_PAGE = SFR_FIRST,
	SFR_CLOCK,
	SFR_SYNC,
	SFR_WR_FLAGS,
	SFR_RD_FLAGS,
	SFR_SER_CTRL,
	SFR_SER_LOW,
	SFR_SER_HIGH,
	SFR_RECEIVED,
	SFR_AUTO_OFF,
	SFR_OUT_B,
	SFR_IN_B,
	SFR_KEY_STATUS,
	SFR_KEY_REG,
	SFR_DIMMER,
	SFR_RANDOM,
	SFR_LAST = SFR_RANDOM,
};

/* Values for Clock special function register. */
enum {
	CLOCK_FASTEST = 0x0,
	CLOCK_100KHZ  = 0x1,
	CLOCK_30KHZ   = 0x2,
	CLOCK_10KHZ   = 0x3,
	CLOCK_3KHZ    = 0x4,
	CLOCK_1KHZ    = 0x5,
	CLOCK_500HZ   = 0x6,
	CLOCK_200HZ   = 0x7,
	CLOCK_100HZ   = 0x8,
	CLOCK_50HZ    = 0x9,
	CLOCK_20HZ    = 0xa,
	CLOCK_10HZ    = 0xb,
	CLOCK_5HZ     = 0xc,
	CLOCK_2HZ     = 0xd,
	CLOCK_1HZ     = 0xe,
	CLOCK_0_5HZ   = 0xf,
};

/* Values for Sync special function register. */
enum {
	SYNC_1000HZ   = 0x0,
	SYNC_600HZ    = 0x1,
	SYNC_400HZ    = 0x2,
	SYNC_250HZ    = 0x3,
	SYNC_150HZ    = 0x4,
	SYNC_100HZ    = 0x5,
	SYNC_60HZ     = 0x6,
	SYNC_40HZ     = 0x7,
	SYNC_25HZ     = 0x8,
	SYNC_15HZ     = 0x9,
	SYNC_10HZ     = 0xa,
	SYNC_6HZ      = 0xb,
	SYNC_4HZ      = 0xc,
	SYNC_2_5HZ    = 0xd,
	SYNC_1_5HZ    = 0xe,
	SYNC_1HZ      = 0xf,
};

/* Bit masks for WrFlags special function register. */
enum {
	WR_FLAG_RX_TX_POS  = 0x1,
	WR_FLAG_IN_OUT_POS = 0x2,
	WR_FLAG_MATRIX_OFF = 0x4,
	WR_FLAG_LEDS_OFF   = 0x8,
};

/* Bit masks for RdFlags special function register. */
enum {
	RD_FLAG_USER_SYNC  = 0x1,
	RD_FLAG_V_FLAG     = 0x2,
};

/* Values and bit masks for SerCtrl special function register. */
enum {
	SERIAL_BAUD_1200   = 0x0,
	SERIAL_BAUD_2400   = 0x1,
	SERIAL_BAUD_4800   = 0x2,
	SERIAL_BAUD_9600   = 0x3,
	SERIAL_BAUD_19200  = 0x4,
	SERIAL_BAUD_38600  = 0x5,
	SERIAL_BAUD_57600  = 0x6,
	SERIAL_BAUD_115200 = 0x7,
	SERIAL_ERROR       = 0x8,
};

/* Type of a memory word. This is a nibble on the actual hardware. */
typedef uint8_t memory_word_t;

/* Address of a word in data memory as offset in words from the beginning. */
typedef uint16_t memory_addr_t;

/* The state of a running virtual machine. */
struct vm_state {
	const struct program *prg;

	/* All user accessible memory. Union is used to allow different views of it. */
	union {
		memory_word_t pages[NUM_PAGES][PAGE_SIZE];
		memory_word_t user_mem[NUM_PAGES * PAGE_SIZE];
		struct {
			union {
				memory_word_t main_regs_page[PAGE_SIZE];
				struct {
					memory_word_t reg_r0;
					memory_word_t reg_r1;
					memory_word_t reg_r2;
					memory_word_t reg_r3;
					memory_word_t reg_r4;
					memory_word_t reg_r5;
					memory_word_t reg_r6;
					memory_word_t reg_r7;
					memory_word_t reg_r8;
					memory_word_t reg_r9;
					union {
						memory_word_t reg_r10;
						memory_word_t reg_out;
					};
					union {
						memory_word_t reg_r11;
						memory_word_t reg_in;
					};
					union {
						memory_word_t reg_r12;
						memory_word_t reg_jsr;
					};
					union {
						memory_word_t reg_r13;
						memory_word_t reg_pcl;
					};
					union {
						memory_word_t reg_r14;
						memory_word_t reg_pcm;
					};
					union {
						memory_word_t reg_r15;
						memory_word_t reg_pch;
					};
				};
			};
			memory_word_t stack[PAGE_SIZE];
			memory_word_t data_ram[NUM_PAGES - 4][PAGE_SIZE];
			memory_word_t alt_regs_page[PAGE_SIZE];
			union {
				memory_word_t special_regs_page[PAGE_SIZE];
				struct {
					memory_word_t reg_page;
					memory_word_t reg_clock;
					memory_word_t reg_sync;
					memory_word_t reg_wr_flags;
					memory_word_t reg_rd_flags;
					memory_word_t reg_ser_ctrl;
					memory_word_t reg_ser_low;
					memory_word_t reg_ser_high;
					memory_word_t reg_received;
					memory_word_t reg_auto_off;
					memory_word_t reg_out_b;
					memory_word_t reg_in_b;
					memory_word_t reg_key_status;
					memory_word_t reg_key_reg;
					memory_word_t reg_dimmer;
					memory_word_t reg_random;
				};
			};
		};
	};

	/* Extra registers that are not directly accessible. */
	program_addr_t reg_pc; /* Program counter. */
	uint8_t reg_sp;        /* Stack pointer. */
	uint8_t reg_flags;     /* Flags. */

	struct rng_state rng;   /* Random number generator state. */

	struct timespec t_start; /* Time of VM startup. */
	vm_clock_t t_cycle_start;
	vm_clock_t t_cycle_end;
	vm_clock_t t_cycle_last_sleep;
	
	vm_clock_t t_last_sync;
};

struct vm_instruction {
	uint8_t nibble1;
	uint8_t nibble2;
	uint8_t nibble3;
};

void vm_init(struct vm_state *vm);

void vm_decode_next(const struct program *prog, struct vm_state *vm, struct vm_instruction *out);

void vm_execute(struct program *prg, int step_mode);

#endif /* _VM_H */
