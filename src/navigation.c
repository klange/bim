#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <ctype.h>
#include "config.h"
#include "buffer.h"
#include "line.h"
#include "display.h"
#include "terminal.h"
#include "navigation.h"
#include "util.h"

static char * paren_pairs = "()[]{}<>";

static int is_paren(int c) {
	char * p = paren_pairs;
	while (*p) {
		if (c == *p) return 1;
		p++;
	}
	return 0;
}

/**
 * If the config option is enabled, find the matching
 * paren character and highlight it with the SELECT
 * colors, clearing out other SELECT values. As we
 * co-opt the SELECT flag, don't do this in selection
 * modes - only in normal and insert modes.
 */
void highlight_matching_paren(void) {
	if (env->mode == MODE_LINE_SELECTION || env->mode == MODE_CHAR_SELECTION) return;
	if (!global_config.highlight_parens) return;
	int line = -1, col = -1;
	if (env->line_no <= env->line_count && env->col_no <= env->lines[env->line_no-1]->actual &&
		is_paren(env->lines[env->line_no-1]->text[env->col_no-1].codepoint)) {
		find_matching_paren(&line, &col, 1);
		if (line != -1) env->highlighting_paren = 1;
	} else if (env->line_no <= env->line_count && env->col_no > 1 && is_paren(env->lines[env->line_no-1]->text[env->col_no-2].codepoint)) {
		find_matching_paren(&line, &col, 2);
		if (line != -1) env->highlighting_paren = 1;
	}
	if (!env->highlighting_paren) return;
	for (int i = 0; i < env->line_count; ++i) {
		int redraw = 0;
		for (int j = 0; j < env->lines[i]->actual; ++j) {
			if (i == line-1 && j == col-1) {
				env->lines[line-1]->text[col-1].flags |= FLAG_SELECT;
				redraw = 1;
				continue;
			}
			if (env->lines[i]->text[j].flags & FLAG_SELECT) {
				redraw = 1;
			}
			env->lines[i]->text[j].flags &= (~FLAG_SELECT);
		}
		if (redraw) {
			if ((i) - env->offset > -1 &&
				(i) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 2) {
				redraw_line((i) - env->offset, i);
			}
		}
	}
	if (line == -1) env->highlighting_paren = 0;
}

/**
 * Move the cursor to the appropriate location based
 * on where it is in the text region.
 *
 * This does some additional math to set the text
 * region horizontal offset.
 */
void place_cursor_actual(void) {

	/* Invalid positions */
	if (env->line_no < 1) env->line_no = 1;
	if (env->col_no  < 1) env->col_no  = 1;

	/* Account for the left hand gutter */
	int num_size = num_width() + 3;
	int x = num_size + 1 - env->coffset;

	/* Determine where the cursor is physically */
	for (int i = 0; i < env->col_no - 1; ++i) {
		char_t * c = &env->lines[env->line_no-1]->text[i];
		x += c->display_width;
	}

	/* y is a bit easier to calculate */
	int y = env->line_no - env->offset + 1;

	int needs_redraw = 0;

	while (y < 2 + global_config.cursor_padding && env->offset > 0) {
		y++;
		env->offset--;
		needs_redraw = 1;
	}

	while (y > global_config.term_height - global_config.bottom_size - global_config.cursor_padding) {
		y--;
		env->offset++;
		needs_redraw = 1;
	}

	if (needs_redraw) {
		redraw_text();
		redraw_tabbar();
		redraw_statusbar();
		redraw_commandline();
	}

	/* If the cursor has gone off screen to the right... */
	if (x > env->width - 1) {
		/* Adjust the offset appropriately to scroll horizontally */
		int diff = x - (env->width - 1);
		env->coffset += diff;
		x -= diff;
		redraw_text();
	}

	/* Same for scrolling horizontally to the left */
	if (x < num_size + 1) {
		int diff = (num_size + 1) - x;
		env->coffset -= diff;
		x += diff;
		redraw_text();
	}

	highlight_matching_paren();
	recalculate_current_line();

	/* Move the actual terminal cursor */
	place_cursor(x+env->left,y);

	/* Show the cursor */
	show_cursor();
}

/**
 * Move the cursor to a specific line.
 */
void goto_line(int line) {

	/* Respect file bounds */
	if (line < 1) line = 1;
	if (line > env->line_count) line = env->line_count;

	/* Move the cursor / text region offsets */
	env->coffset = 0;
	env->line_no = line;
	env->col_no  = 1;

	if (!env->loading) {
		if (line > env->offset && line < env->offset + global_config.term_height - global_config.bottom_size) {
			place_cursor_actual();
		} else {
			try_to_center();
		}
		redraw_most();
	} else {
		try_to_center();
	}
}

/**
 * Find the matching paren for this one.
 *
 * This approach skips having to do its own syntax parsing
 * to deal with, eg., erroneous parens in comments. It does
 * this by finding the matching paren with the same flag
 * value, thus parens in strings will match, parens outside
 * of strings will match, but parens in strings won't
 * match parens outside of strings and so on.
 */
void find_matching_paren(int * out_line, int * out_col, int in_col) {
	if (env->col_no - in_col + 1 > env->lines[env->line_no-1]->actual) {
		return; /* Invalid cursor position */
	}

	/* TODO: vim can find the nearest paren to start searching from, we need to be on one right now */

	int paren_match = 0;
	int direction = 0;
	int start = env->lines[env->line_no-1]->text[env->col_no-in_col].codepoint;
	int flags = env->lines[env->line_no-1]->text[env->col_no-in_col].flags & 0x1F;
	int count = 0;

	/* TODO what about unicode parens? */
	for (int i = 0; paren_pairs[i]; ++i) {
		if (start == paren_pairs[i]) {
			direction = (i % 2 == 0) ? 1 : -1;
			paren_match = paren_pairs[(i % 2 == 0) ? (i+1) : (i-1)];
			break;
		}
	}

	if (!paren_match) return;

	/* Scan for match */
	int line = env->line_no;
	int col  = env->col_no - in_col + 1;

	do {
		while (col > 0 && col < env->lines[line-1]->actual + 1) {
			/* Only match on same syntax */
			if ((env->lines[line-1]->text[col-1].flags & 0x1F) == flags) {
				/* Count up on same direction */
				if (env->lines[line-1]->text[col-1].codepoint == start) count++;
				/* Count down on opposite direction */
				if (env->lines[line-1]->text[col-1].codepoint == paren_match) {
					count--;
					/* When count == 0 we have a match */
					if (count == 0) goto _match_found;
				}
			}
			col += direction;
		}

		line += direction;

		/* Reached first/last line with no match */
		if (line == 0 || line == env->line_count + 1) {
			return;
		}

		/* Reset column to start/end of line, depending on direction */
		if (direction > 0) {
			col = 1;
		} else {
			col = env->lines[line-1]->actual;
		}
	} while (1);

_match_found:
	*out_line = line;
	*out_col  = col;
}

/**
 * Set the visual column the cursor should attempt to keep
 * when moving up and down based on where the cursor currently is.
 * This should happen whenever the user intentionally changes
 * the cursor's horizontal positioning, such as with left/right
 * arrow keys, word-move, search, mouse, etc.
 */
void set_preferred_column(void) {
	int c = 0;
	for (int i = 0; i < env->lines[env->line_no-1]->actual && i < env->col_no-1; ++i) {
		c += env->lines[env->line_no-1]->text[i].display_width;
	}
	env->preferred_column = c;
}

/**
 * Move the cursor down one line in the text region
 */
void cursor_down(void) {
	/* If this isn't already the last line... */
	if (env->line_no < env->line_count) {

		/* Move the cursor down */
		env->line_no += 1;

		/* Try to place the cursor horizontally at the preferred column */
		int _x = 0;
		for (int i = 0; i < env->lines[env->line_no-1]->actual; ++i) {
			char_t * c = &env->lines[env->line_no-1]->text[i];
			_x += c->display_width;
			env->col_no = i+1;
			if (_x > env->preferred_column) {
				break;
			}
		}

		if (env->mode == MODE_INSERT && _x <= env->preferred_column) {
			env->col_no = env->lines[env->line_no-1]->actual + 1;
		}

		/*
		 * If the horizontal cursor position exceeds the width the new line,
		 * then move the cursor left to the extent of the new line.
		 *
		 * If we're in insert mode, we can go one cell beyond the end of the line
		 */
		if (env->col_no > env->lines[env->line_no-1]->actual + (env->mode == MODE_INSERT)) {
			env->col_no = env->lines[env->line_no-1]->actual + (env->mode == MODE_INSERT);
			if (env->col_no == 0) env->col_no = 1;
		}

		if (env->loading) return;

		/*
		 * If the screen was scrolled horizontally, unscroll it;
		 * if it will be scrolled on this line as well, that will
		 * be handled by place_cursor_actual
		 */
		int redraw = 0;
		if (env->coffset != 0) {
			env->coffset = 0;
			redraw = 1;
		}

		/* If we've scrolled past the bottom of the screen, scroll the screen */
		if (env->line_no > env->offset + global_config.term_height - global_config.bottom_size - 1 - global_config.cursor_padding) {
			env->offset += 1;

			/* Tell terminal to scroll */
			if (global_config.can_scroll && !left_buffer) {
				shift_up(1);

				/* A new line appears on screen at the bottom, draw it */
				int l = global_config.term_height - global_config.bottom_size - 1;
				if (env->offset + l < env->line_count + 1) {
					redraw_line(l-1, env->offset + l-1);
				} else {
					draw_excess_line(l - 1);
				}
			} else {
				redraw_text();
			}
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
			return;
		} else if (redraw) {
			/* Otherwise, if we need to redraw because of coffset change, do that */
			redraw_text();
		}

		set_history_break();

		/* Update the status bar */
		redraw_statusbar();

		/* Place the terminal cursor again */
		place_cursor_actual();
	}
}

/**
 * Move the cursor up one line in the text region
 */
void cursor_up(void) {
	/* If this isn't the first line already */
	if (env->line_no > 1) {

		/* Move the cursor down */
		env->line_no -= 1;

		/* Try to place the cursor horizontally at the preferred column */
		int _x = 0;
		for (int i = 0; i < env->lines[env->line_no-1]->actual; ++i) {
			char_t * c = &env->lines[env->line_no-1]->text[i];
			_x += c->display_width;
			env->col_no = i+1;
			if (_x > env->preferred_column) {
				break;
			}
		}

		if (env->mode == MODE_INSERT && _x <= env->preferred_column) {
			env->col_no = env->lines[env->line_no-1]->actual + 1;
		}

		/*
		 * If the horizontal cursor position exceeds the width the new line,
		 * then move the cursor left to the extent of the new line.
		 *
		 * If we're in insert mode, we can go one cell beyond the end of the line
		 */
		if (env->col_no > env->lines[env->line_no-1]->actual + (env->mode == MODE_INSERT)) {
			env->col_no = env->lines[env->line_no-1]->actual + (env->mode == MODE_INSERT);
			if (env->col_no == 0) env->col_no = 1;
		}

		if (env->loading) return;

		/*
		 * If the screen was scrolled horizontally, unscroll it;
		 * if it will be scrolled on this line as well, that will
		 * be handled by place_cursor_actual
		 */
		int redraw = 0;
		if (env->coffset != 0) {
			env->coffset = 0;
			redraw = 1;
		}

		int e = (env->offset == 0) ? env->offset : env->offset + global_config.cursor_padding;
		if (env->line_no <= e) {
			env->offset -= 1;

			/* Tell terminal to scroll */
			if (global_config.can_scroll && !left_buffer) {
				shift_down(1);

				/*
				 * The line at the top of the screen should always be real
				 * so we can just call redraw_line here
				 */
				redraw_line(0,env->offset);
			} else {
				redraw_text();
			}
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
			return;
		} else if (redraw) {
			/* Otherwise, if we need to redraw because of coffset change, do that */
			redraw_text();
		}

		set_history_break();

		/* Update the status bar */
		redraw_statusbar();

		/* Place the terminal cursor again */
		place_cursor_actual();
	}
}

/**
 * Move the cursor one column left.
 */
void cursor_left(void) {
	if (env->col_no > 1) {
		env->col_no -= 1;

		/* Update the status bar */
		redraw_statusbar();

		/* Place the terminal cursor again */
		place_cursor_actual();
	}
	set_history_break();
	set_preferred_column();
}

/**
 * Move the cursor one column right.
 */
void cursor_right(void) {

	/* If this isn't already the rightmost column we can reach on this line in this mode... */
	if (env->col_no < env->lines[env->line_no-1]->actual + !!(env->mode == MODE_INSERT)) {
		env->col_no += 1;

		/* Update the status bar */
		redraw_statusbar();

		/* Place the terminal cursor again */
		place_cursor_actual();
	}
	set_history_break();
	set_preferred_column();
}

/**
 * Move the cursor to the fron the of the line
 */
void cursor_home(void) {
	env->col_no = 1;
	set_history_break();
	set_preferred_column();

	/* Update the status bar */
	redraw_statusbar();

	/* Place the terminal cursor again */
	place_cursor_actual();
}

/**
 * Move the cursor to the end of the line.
 *
 * In INSERT mode, moves one cell right of the end of the line.
 * In NORMAL mode, moves the cursor to the last occupied cell.
 */
void cursor_end(void) {
	env->col_no = env->lines[env->line_no-1]->actual+!!(env->mode == MODE_INSERT);
	set_history_break();
	set_preferred_column();

	/* Update the status bar */
	redraw_statusbar();

	/* Place the terminal cursor again */
	place_cursor_actual();
}

/**
 * Switch to the previous buffer
 */
void previous_tab(void) {
	buffer_t * last = NULL;
	for (int i = 0; i < buffers_len; i++) {
		buffer_t * _env = buffers[i];
		if (_env == env) {
			if (last) {
				/* Wrap around */
				env = last;
				if (left_buffer && (left_buffer != env && right_buffer != env)) unsplit();
				redraw_all();
				update_title();
				return;
			} else {
				env = buffers[buffers_len-1];
				if (left_buffer && (left_buffer != env && right_buffer != env)) unsplit();
				redraw_all();
				update_title();
				return;
			}
		}
		last = _env;
	}
}

/**
 * Switch to the next buffer
 */
void next_tab(void) {
	for (int i = 0; i < buffers_len; i++) {
		buffer_t * _env = buffers[i];
		if (_env == env) {
			if (i != buffers_len - 1) {
				env = buffers[i+1];
				if (left_buffer && (left_buffer != env && right_buffer != env)) unsplit();
				redraw_all();
				update_title();
				return;
			} else {
				/* Wrap around */
				env = buffers[0];
				if (left_buffer && (left_buffer != env && right_buffer != env)) unsplit();
				redraw_all();
				update_title();
				return;
			}
		}
	}
}

/**
 * Handle mouse event
 */
void handle_mouse(void) {
	int buttons = bim_getch() - 32;
	int x = bim_getch() - 32;
	int y = bim_getch() - 32;

	if (buttons == 64) {
		/* Scroll up */
		if (global_config.shift_scrolling) {
			env->loading = 1;
			int shifted = 0;
			for (int i = 0; i < global_config.scroll_amount; ++i) {
				if (env->offset > 0) {
					env->offset--;
					if (env->line_no > env->offset + global_config.term_height - global_config.bottom_size - 1 - global_config.cursor_padding) {
						cursor_up();
					}
					shifted++;
				}
			}
			env->loading = 0;
			if (!shifted) return;
			if (global_config.can_scroll && !left_buffer) {
				shift_down(shifted);
				for (int i = 0; i < shifted; ++i) {
					redraw_line(i,env->offset+i);
				}
			} else {
				redraw_text();
			}
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
		} else {
			for (int i = 0; i < global_config.scroll_amount; ++i) {
				cursor_up();
			}
		}
		return;
	} else if (buttons == 65) {
		/* Scroll down */
		if (global_config.shift_scrolling) {
			env->loading = 1;
			int shifted = 0;
			for (int i = 0; i < global_config.scroll_amount; ++i) {
				if (env->offset < env->line_count-1) {
					env->offset++;
					int e = (env->offset == 0) ? env->offset : env->offset + global_config.cursor_padding;
					if (env->line_no <= e) {
						cursor_down();
					}
					shifted++;
				}
			}
			env->loading = 0;
			if (!shifted) return;
			if (global_config.can_scroll && !left_buffer) {
				shift_up(shifted);
				int l = global_config.term_height - global_config.bottom_size - 1;
				for (int i = 0; i < shifted; ++i) {
					if (env->offset + l - i < env->line_count + 1) {
						redraw_line(l-1-i, env->offset + l-1-i);
					} else {
						draw_excess_line(l - 1 - i);
					}
				}
			} else {
				redraw_text();
			}
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
		} else {
			for (int i = 0; i < global_config.scroll_amount; ++i) {
				cursor_down();
			}
		}
		return;
	} else if (buttons == 3) {
		/* Move cursor to position */

		if (x < 0) return;
		if (y < 0) return;

		if (y == 1) {
			/* Pick from tabs */
			int _x = 0;
			for (int i = 0; i < buffers_len; i++) {
				buffer_t * _env = buffers[i];
				char tmp[64];
				_x += draw_tab_name(_env, tmp);
				if (_x >= x) {
					if (left_buffer && buffers[i] != left_buffer && buffers[i] != right_buffer) unsplit();
					env = buffers[i];
					redraw_all();
					update_title();
					return;
				}
			}
			return;
		}

		if (env->mode == MODE_NORMAL || env->mode == MODE_INSERT) {
			int current_mode = env->mode;
			if (x < env->left && env == right_buffer) {
				use_left_buffer();
			} else if (x > env->width && env == left_buffer) {
				use_right_buffer();
			}
			env->mode = current_mode;
			redraw_all();
		}

		if (env->left) {
			x -= env->left;
		}

		/* Figure out y coordinate */
		int line_no = y + env->offset - 1;
		int col_no = -1;

		if (line_no > env->line_count) {
			line_no = env->line_count;
		}

		if (line_no != env->line_no) {
			env->coffset = 0;
		}

		/* Account for the left hand gutter */
		int num_size = num_width() + 3;
		int _x = num_size - (line_no == env->line_no ? env->coffset : 0);

		/* Determine where the cursor is physically */
		for (int i = 0; i < env->lines[line_no-1]->actual; ++i) {
			char_t * c = &env->lines[line_no-1]->text[i];
			_x += c->display_width;
			if (_x > x-1) {
				col_no = i+1;
				break;
			}
		}

		if (col_no == -1 || col_no > env->lines[line_no-1]->actual) {
			col_no = env->lines[line_no-1]->actual;
		}

		env->line_no = line_no;
		env->col_no = col_no;
		set_history_break();
		set_preferred_column();
		redraw_statusbar();
		place_cursor_actual();
	}
	return;
}

/**
 * Move the cursor the start of the previous word.
 */
void word_left(void) {
	if (!env->lines[env->line_no-1]) return;

	while (env->col_no > 1 && is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint)) {
		env->col_no -= 1;
	}

	if (env->col_no == 1) {
		if (env->line_no == 1) goto _place;
		env->line_no--;
		env->col_no = env->lines[env->line_no-1]->actual;
		goto _place;
	}

	int (*inverse_comparator)(int) = is_special;
	if (env->col_no > 1 && is_special(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint)) {
		inverse_comparator = is_normal;
	}

	do {
		if (env->col_no > 1) {
			env->col_no -= 1;
		}
	} while (env->col_no > 1 && !is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint) && !inverse_comparator(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint));

_place:
	set_preferred_column();
	place_cursor_actual();
}

/**
 * Move cursor to the start of the previous "WORD".
 */
void big_word_left(void) {
	int line_no = env->line_no;
	int col_no = env->col_no;

	do {
		col_no--;
		while (col_no == 0) {
			line_no--;
			if (line_no == 0) {
				goto_line(1);
				set_preferred_column();
				return;
			}
			col_no = env->lines[line_no-1]->actual;
		}
	} while (isspace(env->lines[line_no-1]->text[col_no-1].codepoint));

	do {
		col_no--;
		if (col_no == 0) {
			env->col_no = 1;
			env->line_no = line_no;
			set_preferred_column();
			redraw_statusbar();
			place_cursor_actual();
			return;
		}
	} while (!isspace(env->lines[line_no-1]->text[col_no-1].codepoint));

	env->col_no = col_no;
	env->line_no = line_no;
	set_preferred_column();
	cursor_right();
}

/**
 * Word right
 */
void word_right(void) {
	if (!env->lines[env->line_no-1]) return;

	if (env->col_no >= env->lines[env->line_no-1]->actual) {
		/* next line */
		if (env->line_no == env->line_count) return;
		env->line_no++;
		env->col_no = 0;
		if (env->col_no >= env->lines[env->line_no-1]->actual) {
			goto _place;
		}
	}

	if (env->col_no < env->lines[env->line_no-1]->actual && is_whitespace(env->lines[env->line_no-1]->text[env->col_no - 1].codepoint)) {
		while (env->col_no < env->lines[env->line_no-1]->actual && is_whitespace(env->lines[env->line_no-1]->text[env->col_no - 1].codepoint)) {
			env->col_no++;
		}
		goto _place;
	}

	int (*inverse_comparator)(int) = is_special;
	if (is_special(env->lines[env->line_no - 1]->text[env->col_no - 1].codepoint)) {
		inverse_comparator = is_normal;
	}

	while (env->col_no < env->lines[env->line_no-1]->actual && !is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 1].codepoint) && !inverse_comparator(env->lines[env->line_no - 1]->text[env->col_no - 1].codepoint)) {
		env->col_no++;
	}

	while (env->col_no < env->lines[env->line_no-1]->actual && is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 1].codepoint)) {
		env->col_no++;
	}

_place:
	set_preferred_column();
	place_cursor_actual();
}

/**
 * Word right
 */
void big_word_right(void) {
	int line_no = env->line_no;
	int col_no = env->col_no;

	do {
		col_no++;
		if (col_no > env->lines[line_no-1]->actual) {
			line_no++;
			if (line_no > env->line_count) {
				env->line_no = env->line_count;
				env->col_no  = env->lines[env->line_no-1]->actual;
				set_preferred_column();
				redraw_statusbar();
				place_cursor_actual();
				return;
			}
			col_no = 0;
			break;
		}
	} while (!isspace(env->lines[line_no-1]->text[col_no-1].codepoint));

	do {
		col_no++;
		while (col_no > env->lines[line_no-1]->actual) {
			line_no++;
			if (line_no >= env->line_count) {
				env->col_no = env->lines[env->line_count-1]->actual;
				env->line_no = env->line_count;
				set_preferred_column();
				redraw_statusbar();
				place_cursor_actual();
				return;
			}
			col_no = 1;
		}
	} while (isspace(env->lines[line_no-1]->text[col_no-1].codepoint));

	env->col_no = col_no;
	env->line_no = line_no;
	set_preferred_column();
	redraw_statusbar();
	place_cursor_actual();
	return;
}
