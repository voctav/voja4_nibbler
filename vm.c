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

#include "vm.h"

#include "program.h"
#include "ops.h"
#include "ui.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Clock periods in microseconds indexed by the value of the Clock register. */
long CLOCK_PERIODS_USEC[] = {
	1,
	10,
	33,
	100,
	333,
	1000,
	2000,
	5000,
	10000,
	20000,
	50000,
	100000,
	200000,
	500000,
	1000000,
	2000000,
};

/* Sync periods in microseconds indexed by the value of the Sync register. */
long SYNC_PERIODS_USEC[] = {
	1000,
	1667,
	2500,
	4000,
	6667,
	10000,
	16667,
	25000,
	40000,
	66667,
	100000,
	166667,
	250000,
	400000,
	666667,
	1000000,
};

void vm_init(struct vm_state *vm, struct program *prg)
{
	free(vm->prg);
	vm->prg = prg;

	vm->reg_ser_ctrl = SERIAL_BAUD_9600;
	vm->reg_auto_off = 0x2;
	vm->reg_dimmer = 0xf;
	vm->reg_random = init_rng(&vm->rng);

	get_time(&vm->t_start);
}

void vm_destroy(struct vm_state *vm)
{
	free(vm->prg);
	vm->prg = NULL;
}

void vm_decode_next(struct vm_state *vm, struct vm_instruction *vmi)
{
	/* Should not happen as the program counter cannot exceed the size of program memory. */
	assert(vm->reg_pc < PROGRAM_MEMORY_SIZE);
	program_word_t pi = vm->prg->instructions[vm->reg_pc];
	vm->reg_pc++;
	if (vm->reg_pc == PROGRAM_MEMORY_SIZE) {
		vm->reg_pc = 0; /* Loop back to the first instruction. */
	}
	decode_instruction(pi, vmi);
}

long vm_get_cycle_wait_usec(struct vm_state *vm)
{
	vm_clock_t now = get_vm_clock(&vm->t_start);
	vm->t_cycle_last_sleep = now;
	long elapsed_usec = vm_clock_as_usec(now - vm->t_cycle_start);
	long period_usec = CLOCK_PERIODS_USEC[vm->reg_clock];
	if (period_usec >= elapsed_usec) {
		return period_usec - elapsed_usec;
	} else {
		return 0;
	}
}

/* Updates UserSync flag. */
void vm_update_user_sync(struct vm_state *vm)
{
	vm_clock_t now = get_vm_clock(&vm->t_start);
	long elapsed_usec = vm_clock_as_usec(now - vm->t_last_sync);
	long period_usec = SYNC_PERIODS_USEC[vm->reg_sync];
	if (elapsed_usec >= period_usec) {
		vm->t_last_sync = now;
		vm->reg_rd_flags |= RD_FLAG_USER_SYNC;
	}
}

void vm_update_in_reg(struct vm_state *vm) {
	if (vm->reg_wr_flags & WR_FLAG_IN_OUT_POS) {
		vm->reg_in_b = 0xf;
	} else {
		vm->reg_in = 0xf;
	}
}

void vm_execute_cycle(struct vm_state *vm)
{
	vm->t_cycle_start = get_vm_clock(&vm->t_start);

	vm_update_user_sync(vm);
	vm_update_in_reg(vm);

	struct vm_instruction vmi;
	vm_decode_next(vm, &vmi);
	const struct instruction_descriptor *descr = get_instruction_descriptor(&vmi);
	descr->op->op_fn(&vmi, descr, vm);

	vm->t_cycle_end = get_vm_clock(&vm->t_start);
}

void vm_execute(struct program *prg, int ui_options)
{
	struct vm_state *vm = calloc(1, sizeof(struct vm_state));
	vm_init(vm, prg); /* vm takes ownership of prg. */
	prg = NULL;

	struct ui ui;
	init_ui(&ui, ui_options);

	/* do one pass of UI so single-step mode will start on first instruction */
	update_ui(vm, &ui);

	while (!ui.quit) {
		long delay_usec = vm_get_cycle_wait_usec(vm);
		usleep(delay_usec);

		vm_execute_cycle(vm);

		update_ui(vm, &ui);
	}

	exit_ui(&ui);

	vm_destroy(vm);
	free(vm);
}

