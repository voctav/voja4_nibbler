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

#include "ui.h"

#include "ops.h"
#include "program.h"
#include "vm.h"

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int STATUS_WIDTH = 0x40;
const int STATUS_HEIGHT = 0x24;
const int DISPLAY_WIDTH = 0x10;
const int DISPLAY_HEIGHT = 0x10;

const int DIMMER_LEVELS = 0x10;

const int KEY_UP_DELAY_USEC = 200000;	/* Delay after which a key press will generate a corresponding key release. */
const int STATUS_UPDATE_USEC = 100000;	/* Minimum period between redrawing status during execution. */

/*
 * MAX_UI_SLEEP_USEC should be greater than UI_UPDATE_PERIOD_USEC by a margin of
 * at least the time it takes to do one update to ensure proper VM cycle timing.
 */
const int UI_UPDATE_PERIOD_USEC = 1000;	/* Minimum period between UI updates. */
const int MAX_UI_SLEEP_USEC = 5000;	/* Maximum time to sleep when waiting to synchronize to the next cycle. */

const int DISASSEMBLE_CONTEXT_SIZE = 5;	/* Number of disassembled instructions to show before and after the current one. */
const int DISASSEMBLE_MAX_LEN = 20;	/* Maximum length of a disassembled line. */

char *CLOCK_FREQUENCIES[] = {
	"MAX",
	"100 KHz",
	"30 KHz",
	"10 KHz",
	"3 KHz",
	"1 KHz",
	"500 Hz",
	"200 Hz",
	"100 Hz",
	"50 Hz",
	"20 Hz",
	"10 Hz",
	"5 KHz",
	"2 KHz",
	"1 KHz",
	"0.5 KHz",
};

enum {
	/* Don't mess with the first 8 colors. */
	C_BACKGROUND = 0x08,
	C_PIXEL_DIM0 = 0x10,
};

enum {
	P_PIXEL_OFF = 1,
	P_PIXEL_DIM0 = 2,
};

bool need_cleanup; /* True iff ncurses is initialized and needs cleanup. */

void cleanup()
{
	if (need_cleanup) {
		endwin();
		need_cleanup = false;
	}
}

void handle_signal(int sig)
{
	cleanup();
	/* Don't exit on SIGUSR1 so error can be printed after UI cleanup. */
	if (sig != SIGUSR1) {
		exit(EXIT_SUCCESS);
	}
}

void ui_init(struct ui *ui, int ui_options)
{
	memset(ui, 0, sizeof(struct ui));
	ui->ui_options = ui_options;
}

void ui_start(struct ui *ui)
{
	atexit(cleanup);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	signal(SIGUSR1, handle_signal); /* Custom signal to cleanup UI on error. */

	need_cleanup = true;

	/* Required to display window borders correctly when using UTF-8. */
	setlocale(LC_ALL, "");

	initscr();
	raw();
	noecho();
	curs_set(0);
	cbreak();

	if (has_colors()) {
		bool red_mode = ui->ui_options & RED_MODE;
		start_color();

		if (can_change_color() && COLORS >= C_PIXEL_DIM0 + DIMMER_LEVELS) {
			init_color(C_BACKGROUND, 0x00, 0x00, 0x00);

			for (int i = 0; i < DIMMER_LEVELS; i++) {
				short r = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				short g = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				short b = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				if (red_mode) {
					g = 0;
					b = 0;
				}
				init_color(C_PIXEL_DIM0 + i, r, g, b);
			}
			init_pair(P_PIXEL_OFF, C_BACKGROUND, C_BACKGROUND);
			for (int i = 0; i < DIMMER_LEVELS; i++) {
				init_pair(P_PIXEL_DIM0 + i, C_PIXEL_DIM0 + i, C_BACKGROUND);
			}
		} else {
			init_pair(P_PIXEL_OFF, COLOR_WHITE, COLOR_BLACK);
			for (int i = 0; i < DIMMER_LEVELS; i++) {
				if (red_mode) {
					init_pair(P_PIXEL_DIM0 + i, COLOR_RED, COLOR_BLACK);
				} else {
					init_pair(P_PIXEL_DIM0 + i, COLOR_WHITE, COLOR_BLACK);
				}
			}
		}
	}

	ui->display = newwin(DISPLAY_HEIGHT + 2, DISPLAY_WIDTH + 2, 0, 0);
	box(ui->display, 0, 0);
	wrefresh(ui->display);

	ui->status = newwin(STATUS_HEIGHT + 2, STATUS_WIDTH + 2, 0, DISPLAY_WIDTH + 3);
	box(ui->status, 0, 0);
	wrefresh(ui->status);
	wtimeout(ui->status, 0);
	keypad(ui->status, true);
}

void ui_destroy(struct ui *ui)
{
	cleanup();
}

void maybe_update_display(const struct vm_state *vm, struct ui *ui)
{
	vm_clock_t start = get_vm_clock(&vm->t_start);

	/* Detect if nothing changed and skip update. */
	memory_word_t page = vm->reg_page;
	memory_word_t next_page = (page + 1) % NUM_PAGES;
	memory_word_t dimmer = vm->reg_dimmer;
	bool matrix_off = vm->reg_wr_flags & WR_FLAG_MATRIX_OFF;
	if (ui->last_dimmer == dimmer && matrix_off == ui->last_matrix_off &&
			!memcmp(&ui->last_pages[0][0], &vm->pages[page][0], PAGE_SIZE * sizeof(memory_word_t)) &&
			!memcmp(&ui->last_pages[1][0], &vm->pages[next_page][0], PAGE_SIZE * sizeof(memory_word_t))) {
		vm_clock_t end = get_vm_clock(&vm->t_start);
		ui->dt_last_display_update = end - start;
		return;
	}

	ui->last_dimmer = dimmer;
	ui->last_matrix_off = matrix_off;
	/* TODO(octav): Improve speed for updates to a small number of pixels. */
	memcpy(&ui->last_pages[0][0], &vm->pages[page][0], PAGE_SIZE * sizeof(memory_word_t));
	memcpy(&ui->last_pages[1][0], &vm->pages[next_page][0], PAGE_SIZE * sizeof(memory_word_t));

	int pixel_on_attr, pixel_off_attr;
	if (has_colors()) {
		pixel_on_attr = COLOR_PAIR(P_PIXEL_DIM0 + dimmer);
		pixel_off_attr = COLOR_PAIR(P_PIXEL_OFF);
	} else {
		pixel_on_attr = 0;
		pixel_off_attr = 0;
	}

	for (int i = 0; i < 0x10; i++) {
		wmove(ui->display, i + 1, 1);
		for (int j = page + 1; j >= page; j--) {
			for (int k = 3; k >= 0; k--) {
				if (vm->pages[j][i] & (1 << k)) {
					wattrset(ui->display, pixel_on_attr);
					wprintw(ui->display, "▐▌");
				} else {
					wattrset(ui->display, pixel_off_attr);
					wprintw(ui->display, "  ");
				}
			}
		}
	}
	wrefresh(ui->display);

	vm_clock_t end = get_vm_clock(&vm->t_start);
	ui->dt_last_full_display_update = end - start;
}

void maybe_update_status(const struct vm_state *vm, struct ui *ui)
{
	vm_clock_t start = get_vm_clock(&vm->t_start);

	if (!ui->paused && vm_clock_as_usec(start - ui->t_last_status_update) < STATUS_UPDATE_USEC) {
		return; /* Rate limit status updates when running to avoid execution slowdowns. */
	}

	bool io_pos = vm->reg_wr_flags & WR_FLAG_IN_OUT_POS;
	int row = 1;
	int col = 1;
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last cycle (ns):               %-10lld", vm->dt_last_cycle);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last cycle period (ns):        %-10lld", vm->dt_last_cycle_period);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last user sync period (ns):    %-10lld", vm->dt_last_user_sync_period);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last full display update (ns): %-10lld", ui->dt_last_full_display_update);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last display update (ns):      %-10lld", ui->dt_last_display_update);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Last status update (ns):       %-10lld", ui->dt_last_status_update);
	row++;

	int regs_row = row;
	wmove(ui->status, row++, col);
	wprintw(ui->status, "PC:     %03hx", vm->reg_pc);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "SP:     %hhx", vm->reg_sp);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Flags:  %hhx", vm->reg_flags);
	row++;
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Page:   %hhx", vm->reg_page);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Clock:  %hhx", vm->reg_clock);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Sync:   %hhx", vm->reg_sync);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Out:    %hhx", io_pos ? vm->reg_out_b : vm->reg_out);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "In:     %hhx", io_pos ? vm->reg_in_b : vm->reg_in);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "KeySts: %hhx", vm->reg_key_status);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "KeyReg: %hhx", vm->reg_key_reg);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "WrFlgs: %hhx", vm->reg_wr_flags);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "RdFlgs: %hhx", vm->reg_rd_flags);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "Dimmer: %hhx", vm->reg_dimmer);
	row++;

	int asm_row = row;
	row = regs_row;
	col = 14;
	wmove(ui->status, row++, col);
	wprintw(ui->status, "R0 R1 R2 R3 R4 R5 R6 R7");
	wmove(ui->status, row++, col);
	wprintw(ui->status, " %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx",
		vm->reg_r0, vm->reg_r1, vm->reg_r2, vm->reg_r3,
		vm->reg_r4, vm->reg_r5, vm->reg_r6, vm->reg_r7);
	wmove(ui->status, row++, col);
	wprintw(ui->status, "R8 R9 10 11 12 13 14 15");
	wmove(ui->status, row++, col);
	wprintw(ui->status, " %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx",
		vm->reg_r8, vm->reg_r9, vm->reg_r10, vm->reg_r11,
		vm->reg_r12, vm->reg_r13, vm->reg_r14, vm->reg_r15);

	/* Disassemble current instruction with a context around it. */
	row = asm_row;
	col = 1;
	int first_pc = vm->reg_pc - DISASSEMBLE_CONTEXT_SIZE;
	if (first_pc < 0) {
		first_pc = 0;
	}
	int last_pc = vm->reg_pc + DISASSEMBLE_CONTEXT_SIZE;
	if (last_pc >= PROGRAM_MEMORY_SIZE) {
		last_pc = PROGRAM_MEMORY_SIZE - 1;
	}
	char buf[DISASSEMBLE_MAX_LEN];
	wmove(ui->status, row++, col);
	wprintw(ui->status, "ADDR:  OPC  INSTRUCTION");
	wmove(ui->status, row++, col);
	wprintw(ui->status, "-----------------------");
	for (int pc = first_pc; pc <= last_pc; pc++) {
		struct vm_instruction vmi;
		decode_instruction(vm->prg->instructions[pc], &vmi);
		const struct instruction_descriptor *descr = get_instruction_descriptor(&vmi);
		disassemble_instruction(&vmi, descr, buf, sizeof(buf));
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "%c%03hx:  %hhx%hhx%hhx  %-*s",
			pc == vm->reg_pc ? '>' : ' ', pc, vmi.nibble1, vmi.nibble2, vmi.nibble3, sizeof(buf), buf);
	}

	wrefresh(ui->status);

	vm_clock_t end = get_vm_clock(&vm->t_start);
	ui->dt_last_status_update = end - start;
	ui->t_last_status_update = end;
}

void handle_keys(struct vm_state *vm, struct ui *ui)
{
	int key = -1;
	int ch = wgetch(ui->status);
	switch (ch) {
	case 'q':
		ui->quit = true;
		break;
	case '\n':
		ui->single_step = false;
		ui->paused = false;
		break;
	case ' ':
		ui->single_step = true;
		ui->paused = false;
		break;
	case KEY_LEFT:
		vm->reg_page = (vm->reg_page - 1) & 0xf;
		ui->vm_dirty = true;
		break;
	case KEY_RIGHT:
		vm->reg_page = (vm->reg_page + 1) & 0xf;
		ui->vm_dirty = true;
		break;
	case '\t':
		key = 0;
		break;
	case '1':
		key = 1;
		break;
	case '2':
		key = 2;
		break;
	case '3':
		key = 3;
		break;
	case '4':
		key = 4;
		break;
	case 'a':
		key = 5;
		break;
	case 's':
		key = 6;
		break;
	case 'd':
		key = 7;
		break;
	case 'f':
		key = 8;
		break;
	case 'z':
		key = 9;
		break;
	case 'x':
		key = 10;
		break;
	case 'c':
		key = 11;
		break;
	case 'v':
		key = 12;
		break;
	case '/':
		key = 13;
		break;
	case ERR:
		break;
	}
	if (key >= 0) {
		vm->reg_key_status = KEY_STATUS_JUST_PRESS | KEY_STATUS_LAST_PRESS | KEY_STATUS_ANY_PRESS;
		vm->reg_key_reg = key;
		ui->vm_dirty = true;
		ui->t_last_key_press = get_vm_clock(&vm->t_start);
	} else if (vm->reg_key_status & KEY_STATUS_LAST_PRESS) {
		/*
		 * There's no easy/portable way to get key release events, so assume keys are released
		 * after a preset amount of time.
		 */
		long elapsed_usec = vm_clock_as_usec(get_vm_clock(&vm->t_start) - ui->t_last_key_press);
		if (elapsed_usec >= KEY_UP_DELAY_USEC) {
			/* Generate an artificial key release event, assume all keys have been released. */
			vm->reg_key_status &= ~(KEY_STATUS_LAST_PRESS | KEY_STATUS_ANY_PRESS);
		}
	}
}

void ui_update(struct ui *ui, struct vm_state *vm)
{
	handle_keys(vm, ui);
	if (ui->quit) {
		return;
	}
	if (!ui->vm_dirty) {
		return;
	}

	maybe_update_display(vm, ui);
	maybe_update_status(vm, ui);

	ui->vm_dirty = false;
}

bool ui_run(struct ui *ui, const char *binary_path)
{
	size_t size;
	void *buf = read_file(binary_path, &size);
	if (!buf) {
		return false;
	}
	struct program *prg = load_program(buf, size);
	free(buf);
	buf = NULL;
	if (!prg) {
		return false;
	}

	struct vm_state *vm = calloc(1, sizeof(struct vm_state));
	if (!vm) {
		fprintf(stderr, "Failed to allocate VM state.\n");
		return false;
	}
	vm_init(vm, prg); /* vm takes ownership of prg. */
	prg = NULL;

	ui_start(ui);

	ui->paused = ui->ui_options & START_PAUSED;
	ui->vm_dirty = true;
	vm_clock_t t_last_update = 0;

	while (!ui->quit) {
		/* Rate limit updates if nothing interesting happened since the last one. */
		long elapsed_usec = vm_clock_as_usec(get_vm_clock(&vm->t_start) - t_last_update);
		if (UI_UPDATE_PERIOD_USEC > elapsed_usec) {
			usleep(UI_UPDATE_PERIOD_USEC - elapsed_usec);
		}

		/* Process input and optionally update the screen. */
		ui_update(ui, vm);
		if (ui->paused) {
			continue; /* The cycle clock is paused. */
		}

		/* Check how much time is left until the next cycle. */
		long cycle_delay_usec = vm_get_cycle_wait_usec(vm);
		if (cycle_delay_usec && cycle_delay_usec <= MAX_UI_SLEEP_USEC) {
			/* Not too much, wait now so cycle execution happens below. */
			usleep(cycle_delay_usec);
			cycle_delay_usec = 0;
		}
		if (cycle_delay_usec) {
			continue; /* The next cycle is not here yet.*/
		}

		/* Execute the next cycle. */
		vm_execute_cycle(vm);
		ui->vm_dirty = true; /* VM state probably changed. */
		if (ui->single_step) {
			ui->paused = true; /* Single step mode pauses after each instruction. */
		}
		t_last_update = 0; /* Execute the next update immediately. */
	}

	vm_destroy(vm);
	free(vm);

	return true;
}
