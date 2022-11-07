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
	C_BACKGROUND = 0x00,
	C_TEXT       = 0x01,
	C_BORDER     = 0x02,
	C_PIXEL_DIM0 = 0x10,
};

enum {
	P_TEXT = 1,
	P_BORDER = 2,
	P_PIXEL_OFF = 1,
	P_PIXEL_DIM0 = 2,
};

void finish(int sig)
{
	endwin();
	exit(0);
}

void init_ui(struct ui *ui, int step_mode)
{
	memset(ui, 0, sizeof(struct ui));

	signal(SIGINT, finish);
	signal(SIGTERM, finish);

	setlocale(LC_ALL, "");

	initscr();
	raw();
	noecho();
	curs_set(0);
	cbreak();

	if (has_colors()) {
		start_color();
	}

	init_color(C_BACKGROUND, 0x00, 0x00, 0x00);
	init_color(C_TEXT, 0xc0, 0xc0, 0xc0);
	init_color(C_BORDER, 0x80, 0x80, 0x80);

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
	finish(0);
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

	for (int i = 0; i < 0x10; i++) {
		wmove(ui->display, i + 1, 1);
		for (int j = page + 1; j >= page; j--) {
			for (int k = 3; k >= 0; k--) {
				short c;
				if (vm->pages[j][i] & (1 << k)) {
					c = P_PIXEL_DIM0 + dimmer;
				} else {
					c = P_PIXEL_OFF;
				}
				wattron(ui->display, COLOR_PAIR(c));
				wprintw(ui->display, "▐▌");
			}
		}
	}
	wrefresh(ui->display);
}

void handle_keys(struct vm_state *vm, struct ui *ui, bool loop)
{
	do {
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
		case ERR:
			break;
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
