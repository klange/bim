#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "themes.h"
#include "syntax.h"
#include "config.h"
#include "display.h"
#include "line.h"
#include "buffer.h"
#include "terminal.h"
#include "util.h"
#include "utf8.h"


/**
 * Print a tab name with fixed width and modifiers
 * into an output buffer and return the written width.
 *
 * TODO this isn't unicode/display-width aware, so it returns
 *      byte lengths and doesn't limit the width of the file
 *      properly if it has wide characters. FIXME
 */
int draw_tab_name(buffer_t * _env, char * out) {
	return sprintf(out, "%s %.40s ",
		_env->modified ? " +" : "",
		_env->file_name ? file_basename(_env->file_name) : "[No Name]");
}

/**
 * Redaw the tabbar, with a tab for each buffer.
 *
 * The active buffer is highlighted.
 */
void redraw_tabbar(void) {
	/* Hide cursor while rendering UI */
	hide_cursor();

	/* Move to upper left */
	place_cursor(1,1);

	paint_line(COLOR_TABBAR_BG);

	/* For each buffer... */
	int offset = 0;
	for (int i = 0; i < buffers_len; i++) {
		buffer_t * _env = buffers[i];

		if (_env == env) {
			/* If this is the active buffer, highlight it */
			reset();
			set_colors(COLOR_FG, COLOR_BG);
			set_bold();
		} else {
			/* Otherwise use default tab color */
			reset();
			set_colors(COLOR_FG, COLOR_TAB_BG);
			set_underline();
		}

		char title[64];
		int size = draw_tab_name(_env, title);

		if (offset + size >= global_config.term_width) {
			if (global_config.term_width - offset - 1 > 0) {
				printf("%*s", global_config.term_width - offset - 1, title);
			}
			break;
		} else {
			printf("%s", title);
		}

		offset += size;
	}

	/* Reset bold/underline */
	reset();
	/* Fill the rest of the tab bar */
	set_colors(COLOR_FG, COLOR_TABBAR_BG);
	clear_to_end();
}

/**
 * Braindead log10 implementation for the line numbers
 */
int log_base_10(unsigned int v) {
	int r = (v >= 1000000000) ? 9 : (v >= 100000000) ? 8 : (v >= 10000000) ? 7 :
		(v >= 1000000) ? 6 : (v >= 100000) ? 5 : (v >= 10000) ? 4 :
		(v >= 1000) ? 3 : (v >= 100) ? 2 : (v >= 10) ? 1 : 0;
	return r;
}

/**
 * Render a line of text
 *
 * This handles rendering the actual text content. A full line of text
 * also includes a line number and some padding.
 *
 * width: width of the text display region (term width - line number width)
 * offset: how many cells into the line to start rendering at
 */
void render_line(line_t * line, int width, int offset, int line_no) {
	int i = 0; /* Offset in char_t line data entries */
	int j = 0; /* Offset in terminal cells */

	const char * last_color = NULL;
	int was_selecting = 0, was_searching = 0;

	/* Set default text colors */
	set_colors(COLOR_FG, line->is_current ? COLOR_ALT_BG : COLOR_BG);

	/*
	 * When we are rendering in the middle of a wide character,
	 * we render -'s to fill the remaining amount of the 
	 * charater's width
	 */
	int remainder = 0;

	int is_spaces = 1;

	/* For each character in the line ... */
	while (i < line->actual) {

		/* If there is remaining text... */
		if (remainder) {

			/* If we should be drawing by now... */
			if (j >= offset) {
				/* Fill remainder with -'s */
				set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("-");
				set_colors(COLOR_FG, COLOR_BG);
			}

			/* One less remaining width cell to fill */
			remainder--;

			/* Terminal offset moves forward */
			j++;

			/*
			 * If this was the last remaining character, move to
			 * the next codepoint in the line
			 */
			if (remainder == 0) {
				i++;
			}

			continue;
		}

		/* Get the next character to draw */
		char_t c = line->text[i];

		if (c.codepoint != ' ') is_spaces = 0;

		/* If we should be drawing by now... */
		if (j >= offset) {

			/* If this character is going to fall off the edge of the screen... */
			if (j - offset + c.display_width >= width) {
				/* We draw this with special colors so it isn't ambiguous */
				set_colors(COLOR_ALT_FG, COLOR_ALT_BG);

				/* If it's wide, draw ---> as needed */
				while (j - offset < width - 1) {
					printf("-");
					j++;
				}

				/* End the line with a > to show it overflows */
				printf(">");
				set_colors(COLOR_FG, COLOR_BG);
				return;
			}

			/* Syntax highlighting */
			const char * color = flag_to_color(c.flags);
			if (c.flags & FLAG_SELECT) {
				set_colors(COLOR_SELECTFG, COLOR_SELECTBG);
				was_selecting = 1;
			} else if ((c.flags & FLAG_SEARCH) || (c.flags == FLAG_NOTICE)) {
				set_colors(COLOR_SEARCH_FG, COLOR_SEARCH_BG);
				was_searching = 1;
			} else if (c.flags == FLAG_ERROR) {
				set_colors(COLOR_ERROR_FG, COLOR_ERROR_BG);
				was_searching = 1; /* co-opting this should work... */
			} else {
				if (was_selecting || was_searching) {
					was_selecting = 0;
					was_searching = 0;
					set_colors(color, line->is_current ? COLOR_ALT_BG : COLOR_BG);
					last_color = color;
				} else if (!last_color || strcmp(color, last_color)) {
					set_fg_color(color);
					last_color = color;
				}
			}

			if ((env->mode == MODE_COL_SELECTION || env->mode == MODE_COL_INSERT) &&
				line_no >= ((env->start_line < env->line_no) ? env->start_line : env->line_no) &&
				line_no <= ((env->start_line < env->line_no) ? env->line_no : env->start_line) &&
				((j == env->sel_col) ||
				(j < env->sel_col && j + c.display_width > env->sel_col))) {
				set_colors(COLOR_SELECTFG, COLOR_SELECTBG);
				was_selecting = 1;
			}

#define _set_colors(fg,bg) \
	if (!(c.flags & FLAG_SELECT) && !(was_selecting)) { \
		set_colors(fg,(line->is_current && bg == COLOR_BG) ? COLOR_ALT_BG : bg); \
	}

			/* Render special characters */
			if (c.codepoint == '\t') {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				if (global_config.can_unicode) {
					printf("»");
					for (int i = 1; i < c.display_width; ++i) {
						printf("·");
					}
				} else {
					printf(">");
					for (int i = 1; i < c.display_width; ++i) {
						printf("-");
					}
				}
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint < 32) {
				/* Codepoints under 32 to get converted to ^@ escapes */
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("^%c", '@' + c.codepoint);
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint == 0x7f) {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("^?");
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint > 0x7f && c.codepoint < 0xa0) {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("<%2x>", c.codepoint);
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint == 0xa0) {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("_");
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.display_width == 8) {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("[U+%04x]", c.codepoint);
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.display_width == 10) {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("[U+%06x]", c.codepoint);
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (i > 0 && is_spaces && c.codepoint == ' ' && !(i % env->tabstop)) {
				_set_colors(COLOR_ALT_FG, COLOR_BG); /* Normal background so this is more subtle */
				if (global_config.can_unicode) {
					printf("▏");
				} else {
					printf("|");
				}
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint == ' ' && i == line->actual - 1) {
				/* Special case: space at end of line */
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				if (global_config.can_unicode) {
					printf("·");
				} else {
					printf("-");
				}
				_set_colors(COLOR_FG, COLOR_BG);
			} else {
				/* Normal characters get output */
				char tmp[7]; /* Max six bytes, use 7 to ensure last is always nil */
				to_eight(c.codepoint, tmp);
				printf("%s", tmp);
			}

			/* Advance the terminal cell offset by the render width of this character */
			j += c.display_width;

			/* Advance to the next character */
			i++;
		} else if (c.display_width > 1) {
			/*
			 * If this is a wide character but we aren't ready to render yet,
			 * we may need to draw some filler text for the remainder of its
			 * width to ensure we don't jump around when horizontally scrolling
			 * past wide characters.
			 */
			remainder = c.display_width - 1;
			j++;
		} else {
			/* Regular character, not ready to draw, advance without doing anything */
			j++;
			i++;
		}
	}

	if (env->mode != MODE_LINE_SELECTION) {
		if (line->is_current) {
			set_colors(COLOR_FG, COLOR_ALT_BG);
		} else {
			set_colors(COLOR_FG, COLOR_BG);
		}
	} else {
		if (!line->actual) {
			if (env->line_no == line_no ||
				(env->start_line > env->line_no && 
					(line_no >= env->line_no && line_no <= env->start_line)) ||
				(env->start_line < env->line_no &&
					(line_no >= env->start_line && line_no <= env->line_no))) {
				set_colors(COLOR_SELECTFG, COLOR_SELECTBG);
			}
		}
	}

	if ((env->mode == MODE_COL_SELECTION  || env->mode == MODE_COL_INSERT) &&
		line_no >= ((env->start_line < env->line_no) ? env->start_line : env->line_no) &&
		line_no <= ((env->start_line < env->line_no) ? env->line_no : env->start_line) &&
		j <= env->sel_col &&
		env->sel_col < width) {
		set_colors(COLOR_FG, COLOR_BG);
		while (j < env->sel_col) {
			printf(" ");
			j++;
		}
		set_colors(COLOR_SELECTFG, COLOR_SELECTBG);
		printf(" ");
		j++;
		set_colors(COLOR_FG, COLOR_BG);
	}

	if (env->left + env->width == global_config.term_width && global_config.can_bce) {
		clear_to_end();
	} else {
		/* Paint the rest of the line */
		for (; j - offset < width; ++j) {
			printf(" ");
		}
	}
}

/**
 * Get the width of the line number region
 */
int num_width(void) {
	int w = log_base_10(env->line_count) + 1;
	if (w < 2) return 2;
	return w;
}

/**
 * Draw the gutter and line numbers.
 */
void draw_line_number(int x) {
	/* Draw the line number */
	if (env->lines[x]->is_current) {
		set_colors(COLOR_NUMBER_BG, COLOR_NUMBER_FG);
	} else {
		set_colors(COLOR_NUMBER_FG, COLOR_NUMBER_BG);
	}
	if (global_config.relative_lines && x+1 != env->line_no) {
		x = x+1 - env->line_no;
		x = ((x < 0) ? -x : x)-1;
	}
	int num_size = num_width();
	for (int y = 0; y < num_size - log_base_10(x + 1); ++y) {
		printf(" ");
	}
	printf("%d%c", x + 1, (x+1 == env->line_no && env->coffset > 0) ? '<' : ' ');
}

/**
 * Used to highlight the current line after moving the cursor.
 */
void recalculate_current_line(void) {
	int something_changed = 0;
	if (global_config.highlight_current_line) {
		for (int i = 0; i < env->line_count; ++i) {
			if (env->lines[i]->is_current && i != env->line_no-1) {
				env->lines[i]->is_current = 0;
				something_changed = 1;
				if ((i) - env->offset > -1 &&
					(i) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 2) {
					redraw_line((i) - env->offset, i);
				}
			} else if (i == env->line_no-1 && !env->lines[i]->is_current) {
				env->lines[i]->is_current = 1;
				something_changed = 1;
				if ((i) - env->offset > -1 &&
					(i) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 2) {
					redraw_line((i) - env->offset, i);
				}
			}
		}
	} else {
		something_changed = 1;
	}
	if (something_changed && global_config.relative_lines) {
		for (int i = env->offset; i < env->offset + global_config.term_height - global_config.bottom_size - 1 && i < env->line_count; ++i) {
			/* Place cursor for line number */
			place_cursor(2 + env->left, (i)-env->offset+2);
			draw_line_number(i);
		}
	}
}

/**
 * Redraw line.
 *
 * This draws the line number as well as the actual text.
 * j = screen-relative line offset.
 */
void redraw_line(int j, int x) {
	if (env->loading) return;
	/* Hide cursor when drawing */
	hide_cursor();

	/* Move cursor to upper left most cell of this line */
	place_cursor(1 + env->left,2 + j);

	/* Draw a gutter on the left. */
	switch (env->lines[x]->rev_status) {
		case 1:
			set_colors(COLOR_NUMBER_FG, COLOR_GREEN);
			printf(" ");
			break;
		case 2:
			set_colors(COLOR_NUMBER_FG, global_config.color_gutter ? COLOR_SEARCH_BG : COLOR_ALT_FG);
			printf(" ");
			break;
		case 3:
			set_colors(COLOR_NUMBER_FG, COLOR_KEYWORD);
			printf(" ");
			break;
		case 4:
			set_colors(COLOR_ALT_FG, COLOR_RED);
			printf("▆");
			break;
		case 5:
			set_colors(COLOR_KEYWORD, COLOR_RED);
			printf("▆");
			break;
		default:
			set_colors(COLOR_NUMBER_FG, COLOR_ALT_FG);
			printf(" ");
			break;
	}

	draw_line_number(x);

	/*
	 * Draw the line text 
	 * If this is the active line, the current character cell offset should be used.
	 * (Non-active lines are not shifted and always render from the start of the line)
	 */
	render_line(env->lines[x], env->width - 3 - num_width(), (x + 1 == env->line_no) ? env->coffset : 0, x+1);

}

/**
 * Draw a ~ line where there is no buffer text.
 */
void draw_excess_line(int j) {
	place_cursor(1+env->left,2 + j);
	paint_line(COLOR_ALT_BG);
	set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
	printf("~");
	if (env->left + env->width == global_config.term_width && global_config.can_bce) {
		clear_to_end();
	} else {
		/* Paint the rest of the line */
		for (int x = 1; x < env->width; ++x) {
			printf(" ");
		}
	}
}

/**
 * Redraw the entire text area
 */
void redraw_text(void) {
	/* Hide cursor while rendering */
	hide_cursor();

	/* Figure out the available size of the text region */
	int l = global_config.term_height - global_config.bottom_size - 1;
	int j = 0;

	/* Draw each line */
	for (int x = env->offset; j < l && x < env->line_count; x++) {
		redraw_line(j,x);
		j++;
	}

	/* Draw the rest of the text region as ~ lines */
	for (; j < l; ++j) {
		draw_excess_line(j);
	}
}

static int view_left_offset = 0;
static int view_right_offset = 0;

/**
 * When in split view, draw the other buffer.
 * Has special handling for when the split is
 * on a single buffer.
 */
void redraw_alt_buffer(buffer_t * buf) {
	if (left_buffer == right_buffer) {
		/* draw the opposite view */
		int left, width, offset;
		left = env->left;
		width = env->width;
		offset = env->offset;
		if (left == 0) {
			/* Draw the right side */

			env->left = width;
			env->width = global_config.term_width - width;
			env->offset = view_right_offset;
			view_left_offset = offset;
		} else {
			env->left = 0;
			env->width = global_config.term_width * global_config.split_percent / 100;
			env->offset = view_left_offset;
			view_right_offset = offset;
		}
		redraw_text();

		env->left = left;
		env->width = width;
		env->offset = offset;
	}
	/* Swap out active buffer */
	buffer_t * tmp = env;
	env = buf;
	/* Redraw text */
	redraw_text();
	/* Return original active buffer */
	env = tmp;
}

/**
 * Draw the status bar
 *
 * The status bar shows the name of the file, whether it has modifications,
 * and (in the future) what syntax highlighting mode is enabled.
 *
 * The right side of the tatus bar shows the line number and column.
 */
void redraw_statusbar(void) {
	/* Hide cursor while rendering */
	hide_cursor();

	/* Move cursor to the status bar line (second from bottom */
	place_cursor(1, global_config.term_height - 1);

	/* Set background colors for status line */
	paint_line(COLOR_STATUS_BG);
	set_colors(COLOR_STATUS_FG, COLOR_STATUS_BG);

	/* Print the file name */
	char status_bits[1024] = {0}; /* Sane maximum */
	char * s = status_bits;

	if (env->syntax) {
		s += snprintf(s, 100, "[%s]", env->syntax->name);
	}

	/* Print file status indicators */
	if (env->modified) {
		s += snprintf(s, 5, "[+]");
	}

	if (env->readonly) {
		s += snprintf(s, 6, "[ro]");
	}

	s += snprintf(s, 2, " ");

	if (env->tabs) {
		s += snprintf(s, 20, "[tabs]");
	} else {
		s += snprintf(s, 20, "[spaces=%d]", env->tabstop);
	}

	if (global_config.yanks) {
		s += snprintf(s, 20, "[y:%ld]", global_config.yank_count);
	}

	if (env->indent) {
		s += snprintf(s, 20, "[indent]");
	}

	/* Pre-render the right hand side of the status bar */
	char right_hand[1024];
	snprintf(right_hand, 1024, "Line %d/%d Col: %d ", env->line_no, env->line_count, env->col_no);

	if (env->file_name) {
		int len = strlen(env->file_name);
		int i = 0;
		while (len > 5 && len > (int)global_config.term_width - (int)strlen(right_hand) - (int)strlen(status_bits) - 5) {
			len--;
			i += 1;
		}
		printf("%s%s", i > 0 ? "<" : "", env->file_name + i);
	} else {
		printf("[No Name]");
	}

	printf(" ");

	printf("%s", status_bits);

	/* Clear the rest of the status bar */
	clear_to_end();

	/* Move the cursor appropriately to draw it */
	place_cursor(global_config.term_width - strlen(right_hand), global_config.term_height - 1);
	/* TODO: What if we're localized and this has wide chars? */
	printf("%s",right_hand);
}

/**
 * Redraw the navigation numbers on the right side of the command line
 */
void redraw_nav_buffer() {
	if (nav_buffer) {
		printf("\0337");
		place_cursor(global_config.term_width - nav_buffer - 2, global_config.term_height);
		printf("%s", nav_buf);
		clear_to_end();
		printf("\0338");
	}
}

/**
 * Draw the command line
 *
 * The command line either has input from the user (:quit, :!make, etc.)
 * or shows the INSERT (or VISUAL in the future) mode name.
 */
void redraw_commandline(void) {
	/* Hide cursor while rendering */
	hide_cursor();

	/* Move cursor to the last line */
	place_cursor(1, global_config.term_height);

	/* Set background color */
	paint_line(COLOR_BG);
	set_colors(COLOR_FG, COLOR_BG);

	/* If we are in an edit mode, note that. */
	if (env->mode == MODE_INSERT) {
		set_bold();
		printf("-- INSERT --");
		clear_to_end();
		unset_bold();
	} else if (env->mode == MODE_LINE_SELECTION) {
		set_bold();
		printf("-- LINE SELECTION -- (%d:%d)",
			(env->start_line < env->line_no) ? env->start_line : env->line_no,
			(env->start_line < env->line_no) ? env->line_no : env->start_line
		);
		clear_to_end();
		unset_bold();
	} else if (env->mode == MODE_COL_SELECTION) {
		set_bold();
		printf("-- COL SELECTION -- (%d:%d %d)",
			(env->start_line < env->line_no) ? env->start_line : env->line_no,
			(env->start_line < env->line_no) ? env->line_no : env->start_line,
			(env->sel_col)
		);
		clear_to_end();
		unset_bold();
	} else if (env->mode == MODE_COL_INSERT) {
		set_bold();
		printf("-- COL INSERT -- (%d:%d %d)",
			(env->start_line < env->line_no) ? env->start_line : env->line_no,
			(env->start_line < env->line_no) ? env->line_no : env->start_line,
			(env->sel_col)
		);
		clear_to_end();
		unset_bold();
	} else if (env->mode == MODE_REPLACE) {
		set_bold();
		printf("-- REPLACE --");
		clear_to_end();
		unset_bold();
	} else if (env->mode == MODE_CHAR_SELECTION) {
		set_bold();
		printf("-- CHAR SELECTION -- ");
		clear_to_end();
		unset_bold();
	} else {
		clear_to_end();
	}

	redraw_nav_buffer();
}

/**
 * Draw a message on the command line.
 */
void render_commandline_message(char * message, ...) {
	/* varargs setup */
	va_list args;
	va_start(args, message);
	char buf[1024];

	/* Process format string */
	vsnprintf(buf, 1024, message, args);
	va_end(args);

	/* Hide cursor while rendering */
	hide_cursor();

	/* Move cursor to the last line */
	place_cursor(1, global_config.term_height);

	/* Set background color */
	paint_line(COLOR_BG);
	set_colors(COLOR_FG, COLOR_BG);

	printf("%s", buf);

	/* Clear the rest of the status bar */
	clear_to_end();

	redraw_nav_buffer();
}

/**
 * Draw all screen elements
 */
void redraw_all(void) {
	redraw_tabbar();
	redraw_text();
	if (left_buffer) {
		redraw_alt_buffer(left_buffer == env ? right_buffer : left_buffer);
	}
	redraw_statusbar();
	redraw_commandline();
}

/**
 * Redraw all screen elements except the other split view.
 */
void redraw_most(void) {
	redraw_tabbar();
	redraw_text();
	redraw_statusbar();
	redraw_commandline();
}

/**
 * Disable screen splitting.
 */
void unsplit(void) {
	if (left_buffer) {
		left_buffer->left = 0;
		left_buffer->width = global_config.term_width;
	}
	if (right_buffer) {
		right_buffer->left = 0;
		right_buffer->width = global_config.term_width;
	}
	left_buffer = NULL;
	right_buffer = NULL;

	redraw_all();
}

/**
 * Switch to the left split view
 * (Primarily to handle cases where the left and right are the same buffer)
 */
void use_left_buffer(void) {
	if (left_buffer == right_buffer && env->left != 0) {
		view_right_offset = env->offset;
		env->width = env->left;
		env->left = 0;
		env->offset = view_left_offset;
	}
	env = left_buffer;
	update_title();
}

/**
 * Switch to the right split view
 * (Primarily to handle cases where the left and right are the same buffer)
 */
void use_right_buffer(void) {
	if (left_buffer == right_buffer && env->left == 0) {
		view_left_offset = env->offset;
		env->left = env->width;
		env->width = global_config.term_width - env->width;
		env->offset = view_right_offset;
	}
	env = right_buffer;
	update_title();
}

/**
 * Draw a message on the status line
 */
void render_status_message(char * message, ...) {
	/* varargs setup */
	va_list args;
	va_start(args, message);
	char buf[1024];

	/* Process format string */
	vsnprintf(buf, 1024, message, args);
	va_end(args);

	/* Hide cursor while rendering */
	hide_cursor();

	/* Move cursor to the status bar line (second from bottom */
	place_cursor(1, global_config.term_height - 1);

	/* Set background colors for status line */
	paint_line(COLOR_STATUS_BG);
	set_colors(COLOR_STATUS_FG, COLOR_STATUS_BG);

	printf("%s", buf);

	/* Clear the rest of the status bar */
	clear_to_end();
}

/**
 * Draw an errormessage to the command line.
 */
void render_error(char * message, ...) {
	/* varargs setup */
	va_list args;
	va_start(args, message);
	char buf[1024];

	/* Process format string */
	vsnprintf(buf, 1024, message, args);
	va_end(args);

	/* Hide cursor while rendering */
	hide_cursor();

	/* Move cursor to the command line */
	place_cursor(1, global_config.term_height);

	/* Set appropriate error message colors */
	set_colors(COLOR_ERROR_FG, COLOR_ERROR_BG);

	/* Draw the message */
	printf("%s", buf);
}

