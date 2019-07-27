#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "buffer.h"
#include "config.h"
#include "display.h"
#include "line.h"
#include "navigation.h"
#include "search.h"
#include "terminal.h"
#include "themes.h"
#include "utf8.h"
#include "util.h"

/**
 * Helper for handling smart case sensitivity.
 */
int search_matches(uint32_t a, uint32_t b, int mode) {
	if (mode == 0) {
		return a == b;
	} else if (mode == 1) {
		return tolower(a) == tolower(b);
	}
	return 0;
}

/**
 * Replace text on a given line with other text.
 */
void perform_replacement(int line_no, uint32_t * needle, uint32_t * replacement, int col, int ignorecase, int *out_col) {
	line_t * line = env->lines[line_no-1];
	int j = col;
	while (j < line->actual + 1) {
		int k = j;
		uint32_t * match = needle;
		while (k < line->actual + 1) {
			if (*match == '\0') {
				/* Perform replacement */
				for (uint32_t * n = needle; *n; ++n) {
					line_delete(line, j+1, line_no-1);
				}
				int t = 0;
				for (uint32_t * r = replacement; *r; ++r) {
					char_t _c;
					_c.codepoint = *r;
					_c.flags = 0;
					_c.display_width = codepoint_width(*r);
					line_t * nline = line_insert(line, _c, j + t, line_no -1);
					if (line != nline) {
						env->lines[line_no-1] = nline;
						line = nline;
					}
					t++;
				}

				*out_col = j + t;
				set_modified();
				return;
			}
			if (k == line->actual) break;
			if (!(search_matches(*match, line->text[k].codepoint, ignorecase))) break;
			match++;
			k++;
		}
		j++;
	}
	*out_col = -1;
}


/**
 * Determine whether a string should be searched
 * case-sensitive or not based on whether it contains
 * any upper-case letters.
 */
int smart_case(uint32_t * str) {
	if (!global_config.smart_case) return 0;

	for (uint32_t * s = str; *s; ++s) {
		if (tolower(*s) != (int)*s) {
			return 0;
		}
	}
	return 1;
}

/**
 * Search forward from the given cursor position
 * to find a basic search match.
 *
 * This could be more complicated...
 */
void find_match(int from_line, int from_col, int * out_line, int * out_col, uint32_t * str) {
	int col = from_col;

	int ignorecase = smart_case(str);

	for (int i = from_line; i <= env->line_count; ++i) {
		line_t * line = env->lines[i - 1];

		int j = col - 1;
		while (j < line->actual + 1) {
			int k = j;
			uint32_t * match = str;
			while (k < line->actual + 1) {
				if (*match == '\0') {
					*out_line = i;
					*out_col = j + 1;
					return;
				}
				if (k == line->actual) break;
				if (!(search_matches(*match, line->text[k].codepoint, ignorecase))) break;
				match++;
				k++;
			}
			j++;
		}
		col = 0;
	}
}

/**
 * Search backwards for matching string.
 */
void find_match_backwards(int from_line, int from_col, int * out_line, int * out_col, uint32_t * str) {
	int col = from_col;

	int ignorecase = smart_case(str);

	for (int i = from_line; i >= 1; --i) {
		line_t * line = env->lines[i-1];

		int j = col - 1;
		while (j > -1) {
			int k = j;
			uint32_t * match = str;
			while (k < line->actual + 1) {
				if (*match == '\0') {
					*out_line = i;
					*out_col = j + 1;
					return;
				}
				if (k == line->actual) break;
				if (!(search_matches(*match, line->text[k].codepoint, ignorecase))) break;
				match++;
				k++;
			}
			j--;
		}
		col = (i > 1) ? (env->lines[i-2]->actual) : -1;
	}
}

/**
 * Re-mark search matches while editing text.
 * This gets called after recalculate_syntax, so it works as we're typing or
 * whenever syntax calculation would redraw other lines.
 * XXX There's a bunch of code duplication between the (now three) search match functions.
 *     and search should be improved to support regex anyway, so this needs to be fixed.
 */
void rehighlight_search(line_t * line) {
	if (!global_config.search) return;
	int j = 0;
	int ignorecase = smart_case(global_config.search);
	while (j < line->actual) {
		int k = j;
		uint32_t * match = global_config.search;
		while (k < line->actual+1) {
			if (*match == '\0') {
				for (int i = j; i < k; ++i) {
					line->text[i].flags |= FLAG_SEARCH;
				}
				break;
			}
			if (k == line->actual) break;
			if (!(search_matches(*match, line->text[k].codepoint, ignorecase))) break;
			match++;
			k++;
		}
		j++;
	}
}

/**
 * Draw the matched search result.
 */
void draw_search_match(uint32_t * buffer, int redraw_buffer) {
	for (int i = 0; i < env->line_count; ++i) {
		for (int j = 0; j < env->lines[i]->actual; ++j) {
			env->lines[i]->text[j].flags &= (~FLAG_SEARCH);
		}
	}
	int line = -1, col = -1, _line = 1, _col = 1;
	do {
		find_match(_line, _col, &line, &col, buffer);
		if (line != -1) {
			line_t * l = env->lines[line-1];
			uint32_t * t = buffer;
			for (int i = col; *t; ++i, ++t) {
				l->text[i-1].flags |= FLAG_SEARCH;
			}
		}
		_line = line;
		_col  = col+1;
		line  = -1;
		col   = -1;
	} while (_line != -1);
	redraw_text();
	place_cursor_actual();
	redraw_statusbar();
	redraw_commandline();
	if (redraw_buffer != -1) {
		printf(redraw_buffer == 1 ? "/" : "?");
		uint32_t * c = buffer;
		while (*c) {
			char tmp[7] = {0}; /* Max six bytes, use 7 to ensure last is always nil */
			to_eight(*c, tmp);
			printf("%s", tmp);
			c++;
		}
	}
}

/**
 * Find the next search result, or loop back around if at the end.
 */
void search_next(void) {
	if (!global_config.search) return;
	if (env->coffset) env->coffset = 0;
	int line = -1, col = -1;
	find_match(env->line_no, env->col_no+1, &line, &col, global_config.search);

	if (line == -1) {
		find_match(1,1, &line, &col, global_config.search);
		if (line == -1) return;
	}

	env->col_no = col;
	env->line_no = line;
	set_preferred_column();
	draw_search_match(global_config.search, -1);
}

/**
 * Find the previous search result, or loop to the end of the file.
 */
void search_prev(void) {
	if (!global_config.search) return;
	if (env->coffset) env->coffset = 0;
	int line = -1, col = -1;
	find_match_backwards(env->line_no, env->col_no-1, &line, &col, global_config.search);

	if (line == -1) {
		find_match_backwards(env->line_count, env->lines[env->line_count-1]->actual, &line, &col, global_config.search);
		if (line == -1) return;
	}

	env->col_no = col;
	env->line_no = line;
	set_preferred_column();
	draw_search_match(global_config.search, -1);
}

/**
 * Search mode
 *
 * Search text for substring match.
 */
void search_mode(int direction) {
	uint32_t c;
	uint32_t buffer[1024] = {0};
	int  buffer_len = 0;

	/* utf-8 decoding */

	/* Remember where the cursor is so we can cancel */
	int prev_line = env->line_no;
	int prev_col  = env->col_no;
	int prev_coffset = env->coffset;
	int prev_offset = env->offset;

	redraw_commandline();
	printf(direction == 1 ? "/" : "?");
	if (global_config.search) {
		printf("\0337");
		set_colors(COLOR_ALT_FG, COLOR_BG);
		uint32_t * c = global_config.search;
		while (*c) {
			char tmp[7] = {0}; /* Max six bytes, use 7 to ensure last is always nil */
			to_eight(*c, tmp);
			printf("%s", tmp);
			c++;
		}
		printf("\0338");
		set_colors(COLOR_FG, COLOR_BG);
	}
	show_cursor();

	uint32_t state = 0;
	int cin;

	while ((cin = bim_getch())) {
		if (cin == -1) {
			/* Time out */
			continue;
		}
		if (!decode(&state, &c, cin)) {
			if (c == '\033' || c == 3) {
				/* Cancel search */
				env->line_no = prev_line;
				env->col_no  = prev_col;
				/* Unhighlight search matches */
				for (int i = 0; i < env->line_count; ++i) {
					for (int j = 0; j < env->lines[i]->actual; ++j) {
						env->lines[i]->text[j].flags &= (~FLAG_SEARCH);
					}
					rehighlight_search(env->lines[i]);
				}
				redraw_all();
				break;
			} else if (c == ENTER_KEY || c == LINE_FEED) {
				/* Exit search */
				if (!buffer_len) {
					if (global_config.search) {
						search_next();
					}
					break;
				}
				if (global_config.search) {
					free(global_config.search);
				}
				global_config.search = malloc((buffer_len + 1) * sizeof(uint32_t));
				memcpy(global_config.search, buffer, (buffer_len + 1) * sizeof(uint32_t));
				break;
			} else if (c == BACKSPACE_KEY || c == DELETE_KEY) {
				/* Backspace, delete last character in search buffer */
				if (buffer_len > 0) {
					buffer_len -= 1;
					buffer[buffer_len] = '\0';
					/* Search from beginning to find first match */
					int line = -1, col = -1;
					if (direction == 1) {
						find_match(prev_line, prev_col, &line, &col, buffer);
					} else {
						find_match_backwards(prev_line, prev_col, &line, &col, buffer);
					}

					if (line != -1) {
						env->col_no = col;
						env->line_no = line;
						set_preferred_column();
					}

					draw_search_match(buffer, direction);

				} else {
					/* If backspaced through entire search term, cancel search */
					redraw_commandline();
					env->coffset = prev_coffset;
					env->offset = prev_offset;
					env->col_no = prev_col;
					set_preferred_column();
					env->line_no = prev_line;
					redraw_all();
					break;
				}
			} else {
				/* Regular character */
				buffer[buffer_len] = c;
				buffer_len++;
				buffer[buffer_len] = '\0';
				char tmp[7] = {0}; /* Max six bytes, use 7 to ensure last is always nil */
				to_eight(c, tmp);
				printf("%s", tmp);

				/* Find the next search match */
				int line = -1, col = -1;
				if (direction == 1) {
					find_match(prev_line, prev_col, &line, &col, buffer);
				} else {
					find_match_backwards(prev_line, prev_col, &line, &col, buffer);
				}

				if (line != -1) {
					env->col_no = col;
					env->line_no = line;
					set_preferred_column();
				} else {
					env->coffset = prev_coffset;
					env->offset = prev_offset;
					env->col_no = prev_col;
					set_preferred_column();
					env->line_no = prev_line;
				}
				draw_search_match(buffer, direction);
			}
			show_cursor();
		} else if (state == UTF8_REJECT) {
			state = 0;
		}
	}
}

