#include "vm.h"

#include "program.h"
#include "ops.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
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

void vm_init(struct vm_state *vm)
{
	vm->reg_ser_ctrl = SERIAL_BAUD_9600;
	vm->reg_auto_off = 0x2;
	vm->reg_dimmer = 0xf;

	init_rng(&vm->rng);
	vm->reg_random = next_rng(&vm->rng) & 0xf;

	get_time(&vm->t_start);
}

void vm_decode_next(const struct program *prog, struct vm_state *vm, struct vm_instruction *vmi)
{
	if (vm->reg_pc > prog->length) {
		fprintf(stderr, "Program jumped beyond last instruction.\n");
		exit(1);
	}
	if (vm->reg_pc == prog->length) {
		fprintf(stderr, "Program finished.\n");
		exit(0);
	}
	program_word_t pi = prog->instructions[vm->reg_pc];
	vm->reg_pc++;
	decode_instruction(pi, vmi);
}

/* Sleeps if necessary to synchronize to the start of the next clock cycle. */
void vm_wait_cycle(struct vm_state *vm)
{
	vm_clock_t now = get_vm_clock(&vm->t_start);
	vm->t_cycle_last_sleep = now;
	long elapsed_usec = vm_clock_as_usec(now - vm->t_cycle_start);
	long period_usec = CLOCK_PERIODS_USEC[vm->reg_clock];
	if (period_usec > elapsed_usec) {
		usleep(period_usec - elapsed_usec);
	}

	vm->t_cycle_start = get_vm_clock(&vm->t_start);
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

void vm_execute(struct program *prg)
{
	struct vm_state *vm = calloc(1, sizeof(struct vm_state));
	vm->prg = prg;
	vm_init(vm);

	struct ui ui;
	init_ui(&ui);

	while (!ui.quit) {
		vm_wait_cycle(vm);
		vm_update_user_sync(vm);

		struct vm_instruction vmi;
		vm_decode_next(prg, vm, &vmi);
		const struct instruction_descriptor *descr = get_instruction_descriptor(&vmi);
		descr->op->op_fn(&vmi, descr, vm);

		vm->t_cycle_end = get_vm_clock(&vm->t_start);

		update_ui(vm, &ui);
	}

	exit_ui(&ui);

	free(vm);
}

