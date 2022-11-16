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

#ifndef _UI_H
#define _UI_H

#include "clock.h"
#include "vm.h"

#include <stdbool.h>
#include <ncurses.h>

#define DISPLAY_PAGES 2

struct ui {
	int ui_options; /* Options as bit flags. */

	/* True iff the VM state may have changed since the last update. */
	bool vm_dirty;
	memory_word_t last_pages[DISPLAY_PAGES][PAGE_SIZE];
	memory_word_t last_dimmer;
	bool last_matrix_off;

	WINDOW *status;
	WINDOW *display;

	vm_clock_t t_last_status_update;	/* Timestamp of the last status update. */

	/* Stats. */
	vm_clock_t dt_last_full_display_update;	/* Elapsed time for the last full display update. */
	vm_clock_t dt_last_display_update;	/* Elapsed time for the last display update. */
	vm_clock_t dt_last_status_update;	/* Elapsed time for the last status update. */

	bool quit;
	bool single_step;
	bool paused;
};

enum {
	START_PAUSED = 0x1,
	RED_MODE = 0x2,
};

void ui_init(struct ui *ui, int ui_options);

void ui_destroy(struct ui *ui);

bool ui_run(struct ui *ui, const char *binary_path);

#endif /* _UI_H */
