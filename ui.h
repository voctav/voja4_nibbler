#ifndef _UI_H
#define _UI_H

#include "clock.h"
#include "vm.h"

#include <stdbool.h>
#include <ncurses.h>
#include <time.h>

#define DISPLAY_PAGES 2

struct ui {
	memory_word_t last_pages[DISPLAY_PAGES][PAGE_SIZE];
	memory_word_t last_dimmer;
	bool last_matrix_off;

	WINDOW *status;
	WINDOW *display;
	vm_clock_t t_last_ui_update;

	bool quit;
	bool single_step;
};

void init_ui(struct ui *ui);

/* The UI can interact with the memory, e.g. to override the page register. */
void update_ui(struct vm_state *vm, struct ui *ui);

void exit_ui(struct ui *ui);

#endif /* _UI_H */
