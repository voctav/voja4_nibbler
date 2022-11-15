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

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int STATUS_WIDTH = 0x40;
const int STATUS_HEIGHT = 0x20;
const int DISPLAY_WIDTH = 0x10;
const int DISPLAY_HEIGHT = 0x10;

const int DIMMER_LEVELS = 0x10;

const int STATUS_UPDATE_USEC = 100000;

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
		exit(0);
	}
}

void init_ui(struct ui *ui, int step_mode)
{
	memset(ui, 0, sizeof(struct ui));

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
		start_color();

		if (can_change_color() && COLORS >= C_PIXEL_DIM0 + DIMMER_LEVELS) {
			init_color(C_BACKGROUND, 0x00, 0x00, 0x00);

			for (int i = 0; i < DIMMER_LEVELS; i++) {
				short r = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				short g = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				short b = 1000 * (i + 1) / (DIMMER_LEVELS + 1);
				init_color(C_PIXEL_DIM0 + i, r, g, b);
			}
			init_pair(P_PIXEL_OFF, C_BACKGROUND, C_BACKGROUND);
			for (int i = 0; i < DIMMER_LEVELS; i++) {
				init_pair(P_PIXEL_DIM0 + i, C_PIXEL_DIM0 + i, C_BACKGROUND);
			}
		} else {
			init_pair(P_PIXEL_OFF, COLOR_WHITE, COLOR_BLACK);
			for (int i = 0; i < DIMMER_LEVELS; i++) {
				init_pair(P_PIXEL_DIM0 + i, COLOR_WHITE, COLOR_BLACK);
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

	ui->single_step = step_mode;
}

void exit_ui(struct ui *ui)
{
	cleanup();
}

void maybe_update_pages(const struct vm_state *vm, struct ui *ui)
{
	memory_word_t page = vm->reg_page;
	memory_word_t dimmer = vm->reg_dimmer;
	bool matrix_off = vm->reg_wr_flags & WR_FLAG_MATRIX_OFF;
	if (ui->last_dimmer == dimmer && matrix_off == ui->last_matrix_off &&
		!memcmp(&ui->last_pages[0][0], &vm->pages[page][0], DISPLAY_PAGES * PAGE_SIZE * sizeof(memory_word_t))) {
		return;
	}

	ui->last_dimmer = dimmer;
	ui->last_matrix_off = matrix_off;
	memcpy(&ui->last_pages[0][0], &vm->pages[page][0], DISPLAY_PAGES * PAGE_SIZE * sizeof(memory_word_t));

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
}

void handle_keys(struct vm_state *vm, struct ui *ui, bool loop)
{
	do {
		int key = -1;
		int ch = wgetch(ui->status);
		switch (ch) {
		case 'q':
			ui->quit = true;
			loop = false;
			break;
		case '\n':
			ui->single_step = false;
			loop = false;
			break;
		case ' ':
			ui->single_step = true;
			loop = false;
			break;
		case KEY_LEFT:
			vm->reg_page = (vm->reg_page - 1) & 0xf;
			break;
		case KEY_RIGHT:
			vm->reg_page = (vm->reg_page + 1) & 0xf;
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
		}
	} while (loop);
}

void update_ui(struct vm_state *vm, struct ui *ui)
{
	vm_clock_t now = get_vm_clock(&vm->t_start);

	handle_keys(vm, ui, false /* loop */);
	if (ui->quit) {
		return;
	}

	maybe_update_pages(vm, ui);

	if (ui->single_step || vm_clock_as_usec(now - ui->t_last_ui_update) > STATUS_UPDATE_USEC) {
		bool io_pos = vm->reg_wr_flags & WR_FLAG_IN_OUT_POS;
		int row = 1;
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Prev sleep time (ns): %-10lld",
				vm->t_cycle_start - vm->t_cycle_last_sleep);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Last cycle time (ns): %-10lld",
				vm->t_cycle_end - vm->t_cycle_start);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Render time (ns):     %-10lld",
				now - vm->t_cycle_end);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Update delay (ns):    %-10lld",
				now - ui->t_last_ui_update);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "PC:        %03hhx", vm->reg_pc);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "SP:        %hhx", vm->reg_sp);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Flags:     %hhx", vm->reg_flags);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 10 11 12 13 14 15");
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "%hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx  %hhx",
			vm->reg_r0, vm->reg_r1, vm->reg_r2, vm->reg_r3,
			vm->reg_r4, vm->reg_r5, vm->reg_r6, vm->reg_r7,
			vm->reg_r8, vm->reg_r9, vm->reg_r10, vm->reg_r11,
			vm->reg_r12, vm->reg_r13, vm->reg_r14, vm->reg_r15);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Page:      %hhx", vm->reg_page);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Clock:     %hhx", vm->reg_clock);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Sync:      %hhx", vm->reg_sync);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Out:       %hhx", io_pos ? vm->reg_out_b : vm->reg_out);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "In:        %hhx", io_pos ? vm->reg_in_b : vm->reg_in);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "KeyStatus: %hhx", vm->reg_key_status);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "KeyReg:    %hhx", vm->reg_key_reg);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "WrFlags:   %hhx", vm->reg_wr_flags);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "RdFlags:   %hhx", vm->reg_rd_flags);
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "Dimmer:    %hhx", vm->reg_dimmer);

		struct vm_instruction vmi;
		decode_instruction(vm->prg->instructions[vm->reg_pc], &vmi);
		const struct instruction_descriptor *descr = get_instruction_descriptor(&vmi);
		char buf[20];
		disassemble_instruction(&vmi, descr, buf, sizeof(buf));
		wmove(ui->status, row++, 1);
		wprintw(ui->status, "%03hx: %hhx%hhx%hhx  %-20s", vm->reg_pc, vmi.nibble1, vmi.nibble2, vmi.nibble3, buf);

		wrefresh(ui->status);
		ui->t_last_ui_update = now;
	}

	if (ui->single_step) {
		handle_keys(vm, ui, true /* loop */);
	}
}
