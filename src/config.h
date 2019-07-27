#ifndef _BIM_CONFIG_H
#define _BIM_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include "line.h"

/**
 * Global configuration state
 */
struct bim_config {
	/* Terminal size */
	int term_width, term_height;
	int bottom_size;

	line_t ** yanks;
	size_t    yank_count;
	int       yank_is_full_lines;

	int tty_in;

	const char * bimrc_path;

	unsigned int highlight_on_open:1;
	unsigned int initial_file_is_read_only:1;
	unsigned int can_scroll:1;
	unsigned int can_hideshow:1;
	unsigned int can_altscreen:1;
	unsigned int can_mouse:1;
	unsigned int can_unicode:1;
	unsigned int can_bright:1;
	unsigned int can_title:1;
	unsigned int can_bce:1;
	unsigned int history_enabled:1;
	unsigned int highlight_parens:1;
	unsigned int smart_case:1;
	unsigned int can_24bit:1;
	unsigned int can_256color:1;
	unsigned int can_italic:1;
	unsigned int go_to_line:1;
	unsigned int highlight_current_line:1;
	unsigned int shift_scrolling:1;
	unsigned int check_git:1;
	unsigned int color_gutter:1;
	unsigned int relative_lines:1;
	unsigned int break_from_selection:1;

	int cursor_padding;
	int split_percent;
	int scroll_amount;
	const char * syntax_fallback;
	uint32_t * search;
};

extern struct bim_config global_config;

/**
 * Editor modes (like in vim)
 */
#define MODE_NORMAL 0
#define MODE_INSERT 1
#define MODE_LINE_SELECTION 2
#define MODE_REPLACE 3
#define MODE_CHAR_SELECTION 4
#define MODE_COL_SELECTION 5
#define MODE_COL_INSERT 6

#endif // _BIM_CONFIG_H
