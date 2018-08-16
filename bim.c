/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2018 K. Lange
 * Copyright (C) 2014 Lioncash
 *
 * bim - Text editor
 *
 * bim is a "Bad IMitation" of vim. This is the standard text
 * editor fo ToaruOS. It provides basic editing capabilities.
 *
 * Expansion of bim is planned. I'd love to see it one day have
 * syntax hilighting, for example. It's also painfully slow,
 * so speed improvement is a must - most probably by not
 * redrawing the entire screen all the time.
 */
#define _XOPEN_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>

#ifdef __linux__
#include <stdio_ext.h>
#define BACKSPACE_KEY 0x7F
#else
#include <sys/fswait.h>
#define BACKSPACE_KEY 0x08
#endif

#include <toaru/utf8decode.h>

#define BLOCK_SIZE 256
#define ENTER_KEY     '\n'

/**
 * Theming data
 */
#define COLOR_FG        230
#define COLOR_BG        235
#define COLOR_CURSOR    15
#define COLOR_ALT_FG    244
#define COLOR_ALT_BG    236
#define COLOR_NUMBER_BG 232
#define COLOR_NUMBER_FG 101
#define COLOR_STATUS_BG 238
#define COLOR_TABBAR_BG 230
#define COLOR_TAB_BG    248
#define COLOR_ERROR_FG  15
#define COLOR_ERROR_BG  196

/**
 * Line buffer definitions
 *
 * Lines are essentially resizable vectors of char_t structs,
 * which represent single codepoints in the file.
 */
typedef struct {
	uint32_t display_width:3;
	uint32_t flags:5;
	uint32_t codepoint:24;
} __attribute__((packed)) char_t;

/**
 * Lines have available and actual lengths, describing
 * how much space was allocated vs. how much is being
 * used at the moment.
 */
typedef struct {
	int available;
	int actual;
	char_t   text[0];
} line_t;

/**
 * Global terminal state
 */
int term_width, term_height;
int csr_x_actual, csr_y_actual;

/**
 * Special implementation of getch with a timeout
 */
int _bim_unget = -1;

void bim_unget(int c) {
	_bim_unget = c;
}

int bim_getch(void) {
	if (_bim_unget != -1) {
		int out = _bim_unget;
		_bim_unget = -1;
		return out;
	}
	int fds[] = {STDIN_FILENO};
#ifdef __linux__
#error Need to replace fswait2 with select/poll
#else
	int index = fswait2(1,fds,200);
	if (index == 0) {
		return fgetc(stdin);
	} else {
		return -1;
	}
#endif
}

/**
 * Buffer data
 *
 * A buffer describes a file, and stores
 * its name as well as the editor state
 * (cursor offsets, etc.) and the actual
 * line buffers.
 */
typedef struct _env {
	int    bottom_size;
	short  lineno_width;
	char * file_name;
	int    offset;
	int    coffset;
	int    line_no;
	int    line_count;
	int    line_avail;
	int    col_no;
	short  modified;
	short  mode;
	line_t ** lines;
} buffer_t;

/**
 * Pointer to current active buffer
 */
buffer_t * env;

/**
 * Editor modes (like in vim)
 */
#define MODE_NORMAL 0
#define MODE_INSERT 1

/**
 * Available buffers
 */
int    buffers_len;
int    buffers_avail;
buffer_t ** buffers;

/**
 * Create a new buffer
 */
buffer_t * buffer_new(void) {
	if (buffers_len == buffers_avail) {
		/* If we are out of buffer space, expand the buffers vector */
		buffers_avail *= 2;
		buffers = realloc(buffers, sizeof(buffer_t *) * buffers_avail);
	}

	/* Allocate a new buffer */
	buffers[buffers_len] = malloc(sizeof(buffer_t));
	memset(buffers[buffers_len], 0x00, sizeof(buffer_t));
	buffers_len++;

	return buffers[buffers_len-1];
}

/**
 * Close a buffer
 */
buffer_t * buffer_close(buffer_t * buf) {
	int i;

	/* Locate the buffer in the buffer pointer vector */
	for (i = 0; i < buffers_len; i++) {
		if (buf == buffers[i])
			break;
	}

	/* Invalid buffer? */
	if (i == buffers_len) {
		return env; /* wtf */
	}

	/* Remove the buffer from the vector, moving others up */
	if (i != buffers_len - 1) {
		memmove(&buffers[i], &buffers[i+1], buffers_len - i);
	}

	/* There is one less buffer */
	buffers_len--;
	if (!buffers_len) { 
		/* There are no more buffers. */
		return NULL;
	}

	/* If this was the last buffer, return the previous last buffer */
	if (i == buffers_len) {
		return buffers[buffers_len-1];
	}

	/* Otherwise return the new last buffer */
	return buffers[buffers_len];
}

/**
 * TODO:
 *
 * The line editing functions should probably take a buffer_t *
 * so that they can act on buffers other than the active one.
 */

/**
 * Insert a character into an existing line.
 */
line_t * line_insert(line_t * line, char_t c, int offset) {

	/* If there is not enough space... */
	if (line->actual == line->available) {
		/* Expand the line buffer */
		line->available *= 2;
		line = realloc(line, sizeof(line_t) + sizeof(char_t) * line->available);
	}

	/* If this was not the last character, then shift remaining characters forward. */
	if (offset < line->actual) {
		memmove(&line->text[offset+1], &line->text[offset], sizeof(char_t) * (line->actual - offset));
	}

	/* Insert the new character */
	line->text[offset] = c;

	/* There is one new character in the line */
	line->actual += 1;
	return line;
}

/**
 * Delete a character from a line
 */
void line_delete(line_t * line, int offset) {
	/* Can't delete character before start of line. */
	if (offset == 0) return;

	/* If this isn't the last character, we need to move all subsequent characters backwards */
	if (offset < line->actual) {
		memmove(&line->text[offset-1], &line->text[offset], sizeof(char_t) * (line->actual - offset));
	}

	/* The line is one character shorter */
	line->actual -= 1;
}

/**
 * Remove a line from the active buffer
 */
line_t ** remove_line(line_t ** lines, int offset) {

	/* If there is only one line, clear it instead of removing it. */
	if (env->line_count == 1) {
		lines[offset]->actual = 0;
		return lines;
	}

	/* Otherwise, free the data used by the line */
	free(lines[offset]);

	/* Move other lines up */
	if (offset < env->line_count) {
		memmove(&lines[offset], &lines[offset+1], sizeof(line_t *) * (env->line_count - (offset - 1)));
		lines[env->line_count-1] = NULL;
	}

	/* There is one less line */
	env->line_count -= 1;
	return lines;
}

/**
 * Add a new line to the active buffer.
 */
line_t ** add_line(line_t ** lines, int offset) {

	/* Invalid offset? */
	if (offset > env->line_count) return lines;

	/* Not enough space */
	if (env->line_count == env->line_avail) {
		/* Allocate more space */
		env->line_avail *= 2;
		lines = realloc(lines, sizeof(line_t *) * env->line_avail);
	}

	/* If this isn't the last line, move other lines down */
	if (offset < env->line_count) {
		memmove(&lines[offset+1], &lines[offset], sizeof(line_t *) * (env->line_count - offset));
	}

	/* Allocate the new line */
	lines[offset] = malloc(sizeof(line_t) + sizeof(char_t) * 32);
	lines[offset]->available = 32;
	lines[offset]->actual    = 0;

	/* There is one new line */
	env->line_count += 1;
	return lines;
}

/**
 * Merge two consecutive lines.
 * lineb is the offset of the second line.
 */
line_t ** merge_lines(line_t ** lines, int lineb) {

	/* linea is the line immediately before lineb */
	int linea = lineb - 1;

	/* If there isn't enough space in linea hold both... */
	while (lines[linea]->available < lines[linea]->actual + lines[lineb]->actual) {
		/* ... allocate more space until it fits */
		lines[linea]->available *= 2;
		/* XXX why not just do this once after calculating appropriate size */
		lines[linea] = realloc(lines[linea], sizeof(line_t) + sizeof(char_t) * lines[linea]->available);
	}

	/* Copy the second line into the first line */
	memcpy(&lines[linea]->text[lines[linea]->actual], &lines[lineb]->text, sizeof(char_t) * lines[lineb]->actual);

	/* The first line is now longer */
	lines[linea]->actual = lines[linea]->actual + lines[lineb]->actual;

	/* Remove the second line */
	return remove_line(lines, lineb);
}

/**
 * Split a line into two lines at the given column
 */
line_t ** split_line(line_t ** lines, int line, int split) {

	/* If we're trying to split from the start, just add a new blank line before */
	if (split == 0) {
		return add_line(lines, line - 1);
	}

	/* Allocate more space as needed */
	if (env->line_count == env->line_avail) {
		env->line_avail *= 2;
		lines = realloc(lines, sizeof(line_t *) * env->line_avail);
	}

	/* Shift later lines down */
	if (line < env->line_count) {
		memmove(&lines[line+1], &lines[line], sizeof(line_t *) * (env->line_count - line));
	}

	/* I have no idea what this is doing */
	int remaining = lines[line-1]->actual - split;

	int v = remaining;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	/* Allocate space for the new line */
	lines[line] = malloc(sizeof(line_t) + sizeof(char_t) * v);
	lines[line]->available = v;
	lines[line]->actual = remaining;

	/* Move the data from the old line into the new line */
	memmove(lines[line]->text, &lines[line-1]->text[split], sizeof(char_t) * remaining);
	lines[line-1]->actual = split;

	/* There is one new line */
	env->line_count += 1;

	/* We may have reallocated lines */
	return lines;
}

/**
 * Initialize a buffer with default values
 */
void setup_buffer(buffer_t * env) {
	/* If this buffer was already initialized, clear out its line data */
	if (env->lines) {
		for (int i = 0; i < env->line_count; ++i) {
			free(env->lines[i]);
		}
		free(env->lines);
	}

	/* Default state parameters */
	env->line_no     = 1; /* Default cursor position */
	env->col_no      = 1;
	env->line_count  = 1; /* Buffers always have at least one line */
	env->modified    = 0;
	env->bottom_size = 2;
	env->offset      = 0;
	env->line_avail  = 8; /* Default line buffer capacity */

	/* Allocate line buffer */
	env->lines = malloc(sizeof(line_t *) * env->line_avail);

	/* Initialize the first line */
	env->lines[0] = malloc(sizeof(line_t) + sizeof(char_t) * 32);
	env->lines[0]->available = 32;
	env->lines[0]->actual    = 0;
}

/**
 * Toggle buffered / unbuffered modes
 */
struct termios old;
void set_unbuffered(void) {
	tcgetattr(fileno(stdin), &old);
	struct termios new = old;
	new.c_lflag &= (~ICANON & ~ECHO);
	tcsetattr(fileno(stdin), TCSAFLUSH, &new);
}

void set_buffered(void) {
	tcsetattr(fileno(stdin), TCSAFLUSH, &old);
}

/**
 * Convert codepoint to utf-8 string
 */
int to_eight(uint32_t codepoint, char * out) {
	memset(out, 0x00, 7);

	if (codepoint < 0x0080) {
		out[0] = (char)codepoint;
	} else if (codepoint < 0x0800) {
		out[0] = 0xC0 | (codepoint >> 6);
		out[1] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x10000) {
		out[0] = 0xE0 | (codepoint >> 12);
		out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[2] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x200000) {
		out[0] = 0xF0 | (codepoint >> 18);
		out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[3] = 0x80 | ((codepoint) & 0x3F);
	} else if (codepoint < 0x4000000) {
		out[0] = 0xF8 | (codepoint >> 24);
		out[1] = 0x80 | (codepoint >> 18);
		out[2] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[4] = 0x80 | ((codepoint) & 0x3F);
	} else {
		out[0] = 0xF8 | (codepoint >> 30);
		out[1] = 0x80 | ((codepoint >> 24) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 18) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[4] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[5] = 0x80 | ((codepoint) & 0x3F);
	}

	return strlen(out);
}

/**
 * Get the presentation width of a codepoint
 */
int codepoint_width(uint16_t codepoint) {
	if (codepoint < 32) {
		/* We render these as <XX> */
		return 4;
	}
	/* Skip wcwidth for anything under 256 */
	if (codepoint > 256) {
		/* Higher codepoints may be wider (eg. Japanese) */
		return wcwidth(codepoint);
	}
	return 1;
}

/**
 * Move the terminal cursor
 */
void place_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
	fflush(stdout);
}

/**
 * Move the terminal cursor, but only horizontally
 */
void place_cursor_h(int h) {
	printf("\033[%dG", h);
	fflush(stdout);
}

/**
 * Set text colors
 */
void set_colors(int fg, int bg) {
	printf("\033[48;5;%dm", bg);
	printf("\033[38;5;%dm", fg);
	fflush(stdout);
}

/**
 * Clear the rest of this line
 */
void clear_to_end(void) {
	printf("\033[K");
	fflush(stdout);
}

/**
 * Enable bold text display
 */
void set_bold(void) {
	printf("\033[1m");
	fflush(stdout);
}

/**
 * Enable underlined text display
 */
void set_underline(void) {
	printf("\033[4m");
	fflush(stdout);
}

/**
 * Reset text display attributes
 */
void reset(void) {
	printf("\033[0m");
	fflush(stdout);
}

/**
 * Clear the entire screen
 */
void clear_screen(void) {
	printf("\033[H\033[2J");
	fflush(stdout);
}

/**
 * Hide the cursor
 */
void hide_cursor(void) {
	printf("\033[?25l");
	fflush(stdout);
}

/*
 * Show the cursor
 */
void show_cursor(void) {
	printf("\033[?25h");
	fflush(stdout);
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

	/* For each buffer... */
	for (int i = 0; i < buffers_len; i++) {
		buffer_t * _env = buffers[i];

		if (_env == env) {
			/* If this is the active buffer, hilight it */
			reset();
			set_colors(COLOR_FG, COLOR_BG);
			set_bold();
		} else {
			/* Otherwise use default tab color */
			reset();
			set_colors(COLOR_FG, COLOR_TAB_BG);
			set_underline();
		}

		/* If this buffer is modified, indicate that with a prepended + */
		if (_env->modified) {
			printf(" +");
		}

		/* Print the filename */
		if (_env->file_name) {
			printf(" %s ", _env->file_name);
		} else {
			printf(" [No Name] ");
		}
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
void render_line(line_t * line, int width, int offset) {
	int i = 0; /* Offset in char_t line data entries */
	int j = 0; /* Offset in terminal cells */

	/* Set default text colors */
	set_colors(COLOR_FG, COLOR_BG);

	/*
	 * When we are rendering in the middle of a wide character,
	 * we render -'s to fill the remaining amount of the 
	 * charater's width
	 */
	int remainder = 0;

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
				break;
			}

			/* Render special characters */
			if (c.codepoint == '\t') {
				/* TODO: This should adapt based on tabstops. */
				set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("»···");
				set_colors(COLOR_FG, COLOR_BG);
			} else if (c.codepoint < 32) {
				/* Codepoints under 32 to get converted to <XX> bytes */
				set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("<%02x>", (unsigned int)c.codepoint);
				set_colors(COLOR_FG, COLOR_BG);
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
}

/**
 * Redraw line.
 *
 * This draws the line number as well as the actual text.
 * j = screen-relative line offset.
 */
void redraw_line(int j, int x) {
	/* Hide cursor when drawing */
	hide_cursor();

	/* Move cursor to upper left most cell of this line */
	place_cursor(1,2 + j);

	/* Draw a gutter on the left.
	 * TODO: The gutter can be used to show single-character
	 *       line annotations, such as collapse state, or
	 *       whether a search result was found on this line.
	 */
	set_colors(COLOR_NUMBER_FG, COLOR_ALT_FG);
	printf(" ");

	/* Draw the line number */
	set_colors(COLOR_NUMBER_FG, COLOR_NUMBER_BG);
	int num_size = log_base_10(env->line_count) + 2;
	for (int y = 0; y < num_size - log_base_10(x + 1); ++y) {
		printf(" ");
	}
	printf("%d%c", x + 1, (x+1 == env->line_no && env->coffset > 0) ? '<' : ' ');

	/*
	 * Draw the line text 
	 * If this is the active line, the current character cell offset should be used.
	 * (Non-active lines are not shifted and always render from the start of the line)
	 */
	render_line(env->lines[x], term_width - 3 - num_size, (x + 1 == env->line_no) ? env->coffset : 0);

	/* Clear the rest of the line */
	clear_to_end();
}

/**
 * Redraw the entire text area
 */
void redraw_text(void) {
	/* Hide cursor while rendering */
	hide_cursor();

	/* Figure out the available size of the text region */
	int l = term_height - env->bottom_size - 1;
	int j = 0;

	/* Draw each line */
	for (int x = env->offset; j < l && x < env->line_count; x++) {
		redraw_line(j,x);
		j++;
	}

	/* Draw the rest of the text region as ~ lines */
	for (; j < l; ++j) {
		place_cursor(1,2 + j);
		set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
		printf("~");
		clear_to_end();
	}
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
	place_cursor(1, term_height - 1);

	/* Set background colors for status line */
	set_colors(COLOR_FG, COLOR_STATUS_BG);

	/* Print the file name */
	if (env->file_name) {
		printf("%s", env->file_name);
	} else {
		printf("[No Name]");
	}

	/* Print file status indicators */
	if (env->modified) {
		printf(" [+]");
	}

	/* Clear the rest of the status bar */
	clear_to_end();

	/* Pre-render the right hand side of the status bar */
	char right_hand[1024];
	sprintf(right_hand, "Line %d/%d Col: %d ", env->line_no, env->line_count, env->col_no);

	/* Move the cursor appropriately to draw it */
	place_cursor_h(term_width - strlen(right_hand)); /* TODO: What if we're localized and this has wide chars? */
	printf("%s",right_hand);
	fflush(stdout);
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
	place_cursor(1, term_height);

	/* Set background color */
	set_colors(COLOR_FG, COLOR_BG);

	/* If we are in an edit mode, note that. */
	if (env->mode == MODE_INSERT) {
		set_bold();
		printf("-- INSERT --");
	}

	/* Fill the rest of the command line with the background color */
	clear_to_end();

	/* Reset text colors */
	reset();
}

/**
 * Draw all screen elements
 */
void redraw_all(void) {
	redraw_tabbar();
	redraw_text();
	redraw_statusbar();
	redraw_commandline();
}

/**
 * Update the terminal title bar
 */
void update_title(void) {
	char cwd[1024] = {'/',0};
	getcwd(cwd, 1024);

	/*
	 * XXX: I think this only works in a few terminals.
	 *      VTE seems to have a different escape sequence for tab names.
	 */
	printf("\033]1;%s%s (%s) - BIM\007", env->file_name, env->modified ? " +" : "", cwd);
}

/**
 * Mark this buffer as modified and
 * redraw the status and tabbar if needed.
 */
void set_modified(void) {
	/* If it was already marked modified, no need to do anything */
	if (env->modified) return;

	/* Mark as modified */
	env->modified = 1;

	/* Redraw some things */
	update_title();
	redraw_tabbar();
	redraw_statusbar();
}

/**
 * Draw an errormessage to the command line.
 */
void render_error(char * message) {
	redraw_commandline(); /* Note: this hides the cursor for us */

	/* Set appropriate error message colors */
	set_colors(COLOR_ERROR_FG, COLOR_ERROR_BG);

	/* Draw the message */
	printf("%s", message);
	fflush(stdout);
}

/**
 * Move the cursor to the appropriate location based
 * on where it is in the text region.
 *
 * This does some additional math to set the text
 * region horizontal offset.
 */
void place_cursor_actual(void) {

	/* Account for the left hand gutter */
	int num_size = log_base_10(env->line_count) + 5;
	int x = num_size + 1 - env->coffset;

	/* Determine where the cursor is physically */
	for (int i = 0; i < env->col_no - 1; ++i) {
		char_t * c = &env->lines[env->line_no-1]->text[i];
		x += c->display_width;
	}

	/* y is a bit easier to calculate */
	int y = env->line_no - env->offset + 1;

	/* If the cursor has gone off screen to the right... */
	if (x > term_width - 1) {
		/* Adjust the offset appropriately to scroll horizontally */
		int diff = x - (term_width - 1);
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

	/* Move the actual terminal cursor */
	place_cursor(x,y);
	csr_x_actual = x;
	csr_y_actual = y;

	/* Show the cursor */
	show_cursor();
}

/**
 * Handle terminal size changes
 */
void SIGWINCH_handler(int sig) {
	(void)sig;
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	term_width = w.ws_col;
	term_height = w.ws_row;
	redraw_all();

	signal(SIGWINCH, SIGWINCH_handler);
}

/**
 * Run global initialization tasks
 */
void initialize(void) {
	setlocale(LC_ALL, "");

	buffers_avail = 4;
	buffers = malloc(sizeof(buffer_t *) * buffers_avail);

	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	term_width = w.ws_col;
	term_height = w.ws_row;
	set_unbuffered();

	signal(SIGWINCH, SIGWINCH_handler);
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
	env->offset = line - 1;
	env->line_no = line;
	env->col_no  = 1;
	redraw_all();
}


/**
 * UTF-8 parser state
 */
static uint32_t codepoint_r;
static uint32_t state = 0;

/**
 * Processs (part of) a file and add it to a buffer.
 */
void add_buffer(uint8_t * buf, int size) {
	for (int i = 0; i < size; ++i) {
		if (!decode(&state, &codepoint_r, buf[i])) {
			uint32_t c = codepoint_r;
			if (c == '\n') {
				env->lines = add_line(env->lines, env->line_no);
				env->col_no = 1;
				env->line_no += 1;
			} else {
				char_t _c;
				_c.codepoint = (uint32_t)c;
				_c.display_width = codepoint_width((uint16_t)c);
				line_t * line  = env->lines[env->line_no - 1];
				line_t * nline = line_insert(line, _c, env->col_no - 1);
				if (line != nline) {
					env->lines[env->line_no - 1] = nline;
				}
				env->col_no += 1;
			}
		} else if (state == UTF8_REJECT) {
			state = 0;
		}
	}
}

/**
 * Create a new buffer from a file.
 */
void open_file(char * file) {
	env = buffer_new();

	env->file_name = malloc(strlen(file) + 1);
	memcpy(env->file_name, file, strlen(file) + 1);

	setup_buffer(env);

	FILE * f = fopen(file, "r");

	if (!f) {
		char buf[1024];
		sprintf(buf, "Could not open %s", file);
		render_error(buf);
		return;
	}

	size_t length;

	fseek(f, 0, SEEK_END);
	length = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t buf[BLOCK_SIZE];

	state = 0;

	/* XXX: while (!feof()) ? */
	while (length > BLOCK_SIZE) {
		fread(buf, BLOCK_SIZE, 1, f);
		add_buffer(buf, BLOCK_SIZE);
		length -= BLOCK_SIZE;
	}
	if (length > 0) {
		fread(buf, 1, length, f);
		add_buffer((uint8_t *)buf, length);
	}

	if (env->line_no && env->lines[env->line_no-1] && env->lines[env->line_no-1]->actual == 0) {
		/* Remove blank line from end */
		remove_line(env->lines, env->line_no-1);
	}

	update_title();
	goto_line(0);

	fclose(f);
}

/**
 * Clean up the terminal and exit the editor.
 */
void quit(void) {
	set_buffered();
	reset();
	clear_screen();
	printf("Thanks for flying bim!\n");
	exit(0);
}

/**
 * Try to quit, but don't continue if there are
 * modified buffers open.
 */
void try_quit(void) {
	for (int i = 0; i < buffers_len; i++ ) {
		buffer_t * _env = buffers[i];
		if (_env->modified) {
			char msg[100];
			sprintf(msg, "Modifications made to file `%s` in tab %d. Aborting.", _env->file_name, i+1);
			render_error(msg);
			return;
		}
	}
	quit();
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
				redraw_all();
				return;
			} else {
				env = buffers[buffers_len-1];
				redraw_all();
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
				redraw_all();
				return;
			} else {
				/* Wrap around */
				env = buffers[0];
				redraw_all();
				return;
			}
		}
	}
}

/**
 * Write active buffer to file
 */
void write_file(char * file) {
	if (!file) {
		render_error("Need a file to write to.");
		return;
	}

	FILE * f = fopen(file, "w+");

	if (!f) {
		render_error("Failed to open file for writing.");
		return;
	}

	/* Go through each line and convert it back to UTF-8 */
	int i, j;
	for (i = 0; i < env->line_count; ++i) {
		line_t * line = env->lines[i];
		for (j = 0; j < line->actual; j++) {
			char_t c = line->text[j];
			if (c.codepoint == 0) {
				char buf[1] = {0};
				fwrite(buf, 1, 1, f);
			} else {
				char tmp[4];
				int i = to_eight(c.codepoint, tmp);
				fwrite(tmp, i, 1, f);
			}
		}
		fputc('\n', f);
	}
	fclose(f);

	/* Mark it no longer modified */
	env->modified = 0;

	/* If there was no file name set, set one */
	if (!env->file_name) {
		env->file_name = malloc(strlen(file) + 1);
		memcpy(env->file_name, file, strlen(file) + 1);
	}

	redraw_all();
}

/**
 * Close the active buffer
 */
void close_buffer(void) {
	buffer_t * previous_env = env;
	buffer_t * new_env = buffer_close(env);
	if (new_env == previous_env) {
		/* ?? Failed to close buffer */
		render_error("lolwat");
	}
	/* No more buffers, exit */
	if (!new_env) {
		quit();
	}
	/* Clean up the old buffer */
	free(previous_env);

	/* Set the new active buffer */
	env = new_env;

	/* Redraw the screen */
	redraw_all();
}

/**
 * Move the cursor down one line in the text region
 */
void cursor_down(void) {
	/* If this isn't already the last line... */
	if (env->line_no < env->line_count) {

		/* Move the cursor down */
		env->line_no += 1;

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
		if (env->line_no > env->offset + term_height - env->bottom_size - 1) {
			env->offset += 1;

			/* Tell terminal to scroll */
			printf("\033[1S");

			/* A new line appears on screen at the bottom, draw it */
			int l = term_height - env->bottom_size - 1;
			if (env->offset + l < env->line_count) {
				redraw_line(l-1, env->offset + l-1);
			} else {
				place_cursor(1, 2 + l - 1);
				set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("~");
				clear_to_end();
			}

			/* Redraw elements that were moved by scrolling */
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
			return;
		} else if (redraw) {
			/* Otherwise, if we need to redraw because of coffset change, do that */
			redraw_text();
		}

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

		if (env->line_no <= env->offset) {
			env->offset -= 1;

			/* Tell terminal to scroll */
			printf("\033[1T");

			/*
			 * The line at the top of the screen should always be real
			 * so we can just call redraw_line here
			 */
			redraw_line(0,env->offset);

			/* Redraw elements that were moved by scrolling */
			redraw_tabbar();
			redraw_statusbar();
			redraw_commandline();
			place_cursor_actual();
			return;
		} else if (redraw) {
			/* Otherwise, if we need to redraw because of coffset change, do that */
			redraw_text();
		}

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
}

/**
 * Move the cursor to the fron the of the line
 */
void cursor_home(void) {
	env->col_no = 1;

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

	/* Update the status bar */
	redraw_statusbar();

	/* Place the terminal cursor again */
	place_cursor_actual();
}

/**
 * Leave INSERT mode
 *
 * If the cursor is too far right, adjust it.
 * Redraw the command line.
 */
void leave_insert(void) {
	if (env->col_no > env->lines[env->line_no-1]->actual) {
		env->col_no = env->lines[env->line_no-1]->actual;
		if (env->col_no == 0) env->col_no = 1;
	}
	env->mode = MODE_NORMAL;
	redraw_commandline();
}

/**
 * Process a user command.
 */
void process_command(char * cmd) {
	/* Special case ! to run shell commands without parsing tokens */
	if (*cmd == '!') {
		/* Reset and draw some line feeds */
		printf("\033[0m\n\n");

		/* Set buffered for shell application */
		set_buffered();

		/* Call the shell and wait for completion */
		system(&cmd[1]);

		/* Return to the editor, wait for user to press enter. */
		set_unbuffered();
		printf("\n\nPress ENTER to continue.");
		fflush(stdout);
		while (bim_getch() != ENTER_KEY);

		/* Redraw the screen */
		redraw_all();

		/* Done processing command */
		return;
	}

	/* Tokenize argument string on spaces */
	char *p, *argv[512], *last;
	int argc = 0;
	for ((p = strtok_r(cmd, " ", &last)); p;
			(p = strtok_r(NULL, " ", &last)), argc++) {
		if (argc < 511) argv[argc] = p;
	}
	argv[argc] = NULL;

	if (argc < 1) {
		/* no op */
		return;
	}

	if (!strcmp(argv[0], "e")) {
		/* e: edit file */
		if (argc > 1) {
			/* This actually opens a new tab */
			open_file(argv[1]);
		} else {
			/* TODO: Reopen file? */
			render_error("Expected a file to open...");
		}
	} else if (!strcmp(argv[0], "w")) {
		/* w: write file */
		if (argc > 1) {
			write_file(argv[1]);
		} else {
			write_file(env->file_name);
		}
	} else if (!strcmp(argv[0], "wq")) {
		/* wq: write file and close buffer; if there's no file to write to, may do weird things */
		write_file(env->file_name);
		close_buffer();
	} else if (!strcmp(argv[0], "q")) {
		/* close buffer if unmodified */
		if (env->modified) {
			render_error("No write since last change. Use :q! to force exit.");
		} else {
			close_buffer();
		}
	} else if (!strcmp(argv[0], "q!")) {
		/* close buffer without warning if unmodified */
		close_buffer();
	} else if (!strcmp(argv[0], "qa") || !strcmp(argv[0], "qall")) {
		/* Close all */
		try_quit();
	} else if (!strcmp(argv[0], "qa!")) {
		/* Forcefully exit editor */
		quit();
	} else if (!strcmp(argv[0], "tabp")) {
		/* Next tab */
		previous_tab();
	} else if (!strcmp(argv[0], "tabn")) {
		/* Previous tab */
		next_tab();
	} else if (isdigit(*argv[0])) {
		/* Go to line number */
		goto_line(atoi(argv[0]));
	} else {
		/* Unrecognized command */
		char buf[512];
		sprintf(buf, "Not an editor command: %s", argv[0]);
		render_error(buf);
	}
}

/**
 * Command mode
 *
 * Accept a user command and then process it and
 * return to normal mode.
 *
 * TODO: We only have basic line editing here; it might be
 *       nice to add more advanced line editing, like cursor
 *       movement, tab completion, etc. This is easier than
 *       with the shell since we have a lot more control over
 *       where the command input bar is rendered.
 */
void command_mode(void) {
	char c;
	char buffer[1024] = {0};
	int  buffer_len = 0;

	redraw_commandline();
	printf(":");
	show_cursor();

	while ((c = bim_getch())) {
		if (c == -1) {
			/* Time out */
			continue;
		}
		if (c == '\033') {
			/* Escape, cancel command */
			break;
		} else if (c == ENTER_KEY) {
			/* Enter, run command */
			process_command(buffer);
			break;
		} else if (c == BACKSPACE_KEY) {
			/* Backspace, delete last character in command buffer */
			if (buffer_len > 0) {
				buffer_len -= 1;
				buffer[buffer_len] = '\0';
				redraw_commandline();
				printf(":%s", buffer);
			} else {
				/* If backspaced through entire command, cancel command mode */
				redraw_commandline();
				break;
			}
		} else {
			/* Regular character */
			buffer[buffer_len] = c;
			buffer_len++;
			printf("%c", c);
		}
		show_cursor();
	}
}

/**
 * INSERT mode
 *
 * Accept input into the text buffer.
 */
void insert_mode(void) {
	int cin;
	uint32_t c;

	/* Set mode line */
	env->mode = MODE_INSERT;
	redraw_commandline();

	/* Place the cursor in the text area */
	place_cursor_actual();

	int timeout = 0;
	int this_buf[20];
	uint32_t istate = 0;
	while ((cin = bim_getch())) {
		if (cin == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				leave_insert();
				return;
			}
			timeout = 0;
			continue;
		}
		if (!decode(&istate, &c, cin)) {
			switch (c) {
				case '\033':
					if (timeout == 0) {
						this_buf[timeout] = c;
						timeout++;
					} else if (timeout == 1 && this_buf[0] == '\033') {
						leave_insert();
						bim_unget(c);
						return;
					}
					break;
				case BACKSPACE_KEY:
					if (env->col_no > 1) {
						line_delete(env->lines[env->line_no - 1], env->col_no - 1);
						env->col_no -= 1;
						redraw_line(env->line_no - env->offset - 1, env->line_no-1);
						set_modified();
						redraw_statusbar();
						place_cursor_actual();
					} else if (env->line_no > 1) {
						int tmp = env->lines[env->line_no - 2]->actual;
						merge_lines(env->lines, env->line_no - 1);
						env->line_no -= 1;
						env->col_no = tmp+1;
						redraw_text();
						set_modified();
						redraw_statusbar();
						place_cursor_actual();
					}
					break;
				case ENTER_KEY:
					if (env->col_no == env->lines[env->line_no - 1]->actual + 1) {
						env->lines = add_line(env->lines, env->line_no);
					} else {
						/* oh oh god we're all gonna die */
						env->lines = split_line(env->lines, env->line_no, env->col_no - 1);
					}
					env->col_no = 1;
					env->line_no += 1;
					if (env->line_no > env->offset + term_height - env->bottom_size - 1) {
						env->offset += 1;
					}
					redraw_text();
					set_modified();
					redraw_statusbar();
					place_cursor_actual();
					break;
				default:
					{
						if (timeout == 1 && c == '[' && this_buf[0] == '\033') {
							this_buf[1] = c;
							timeout++;
							continue;
						} else if (timeout == 1 && c  != '[') {
							leave_insert();
							bim_unget(c);
							return;
						}
						if (timeout == 2 && this_buf[0] == '\033' && this_buf[1] == '[') {
							switch (c) {
								case 'A': // up
									cursor_up();
									break;
								case 'B': // down
									cursor_down();
									break;
								case 'C': // right
									cursor_right();
									break;
								case 'D': // left
									cursor_left();
									break;
								case 'H': // home
									cursor_home();
									break;
								case 'F': // end
									cursor_end();
									break;
								case '6':
									if (bim_getch() == '~') {
										/* Page down */
										goto_line(env->line_no + term_height - 6);
									}
									break;
								case '5':
									if (bim_getch() == '~') {
										/* Page up */
										goto_line(env->line_no - (term_height - 6));
									}
									break;
								case '3':
									if (bim_getch() == '~') {
										/* Page up */
										if (env->col_no < env->lines[env->line_no - 1]->actual + 1) {
											line_delete(env->lines[env->line_no - 1], env->col_no);
											redraw_line(env->line_no - env->offset - 1, env->line_no-1);
											set_modified();
											redraw_statusbar();
											place_cursor_actual();
										} else if (env->line_no < env->line_count) {
											merge_lines(env->lines, env->line_no);
											redraw_text();
											set_modified();
											redraw_statusbar();
											place_cursor_actual();
										}
									}
									break;
							}
							timeout = 0;
							continue;
						}
						timeout = 0;
						char_t _c;
						_c.codepoint = c;
						_c.display_width = codepoint_width(c);
						line_t * line  = env->lines[env->line_no - 1];
						line_t * nline = line_insert(line, _c, env->col_no - 1);
						if (line != nline) {
							env->lines[env->line_no - 1] = nline;
						}
						redraw_line(env->line_no - env->offset - 1, env->line_no-1);
						env->col_no += 1;
						set_modified();
						redraw_statusbar();
						place_cursor_actual();
					}
					break;
			}
		} else if (istate == UTF8_REJECT) {
			istate = 0;
		}
	}
}

int main(int argc, char * argv[]) {
	initialize();

	if (argc > 1) {
		open_file(argv[1]);
	} else {
		env = buffer_new();
		update_title();
		setup_buffer(env);
	}

	redraw_all();

	while (1) {
		place_cursor_actual();
		char c;
		int timeout = 0;
		int this_buf[20];
		while ((c = bim_getch())) {
			switch (c) {
				case '\033':
					if (timeout == 0) {
						this_buf[timeout] = c;
						timeout++;
					}
					break;
				case BACKSPACE_KEY:
					cursor_left();
					break;
				case ':':
					/* Switch to command mode */
					command_mode();
					break;
				case 'j':
					cursor_down();
					break;
				case 'k':
					cursor_up();
					break;
				case 'h':
					cursor_left();
					break;
				case 'l':
					cursor_right();
					break;
				case 'd':
					remove_line(env->lines, env->line_no-1);
					env->col_no = 1;
					if (env->line_no > env->line_count) {
						env->line_no--;
					}
					redraw_text();
					set_modified();
					place_cursor_actual();
					break;
				case ' ':
					goto_line(env->line_no + term_height - 6);
					break;
				case 'O':
					{
						env->lines = add_line(env->lines, env->line_no-1);
						env->col_no = 1;
						redraw_text();
						set_modified();
						place_cursor_actual();
						goto _insert;
					}
				case 'o':
					{
						env->lines = add_line(env->lines, env->line_no);
						env->col_no = 1;
						env->line_no += 1;
						if (env->line_no > env->offset + term_height - env->bottom_size - 1) {
							env->offset += 1;
						}
						redraw_text();
						set_modified();
						place_cursor_actual();
						goto _insert;
					}
				case 'a':
					if (env->col_no < env->lines[env->line_no-1]->actual + 1) {
						env->col_no += 1;
					}
					goto _insert;
				case '$':
					env->col_no = env->lines[env->line_no-1]->actual+1;
					break;
				case '0':
					env->col_no = 1;
					break;
				case 'i':
_insert:
					insert_mode();
					redraw_statusbar();
					redraw_commandline();
					timeout = 0;
					break;
				default:
					if (timeout == 1 && c == '[' && this_buf[0] == '\033') {
						this_buf[1] = c;
						timeout++;
						continue;
					}
					if (timeout == 2 && this_buf[0] == '\033' && this_buf[1] == '[') {
						switch (c) {
							case 'A': // up
								cursor_up();
								break;
							case 'B': // down
								cursor_down();
								break;
							case 'C': // right
								cursor_right();
								break;
							case 'D': // left
								cursor_left();
								break;
							case 'H': // home
								cursor_home();
								break;
							case 'F': // end
								cursor_end();
								break;
							case '6':
								if (bim_getch() == '~') {
									/* Page down */
									goto_line(env->line_no + term_height - 6);
								}
								break;
							case '5':
								if (bim_getch() == '~') {
									/* Page up */
									goto_line(env->line_no - (term_height - 6));
								}
								break;
						}
						timeout = 0;
						continue;
					}
					timeout = 0;
					break;
			}
			place_cursor_actual();
		}
		printf("%c", c);
	}

	return 0;
}
