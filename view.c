#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "buffer.h"
#include "syntax.h"
#include "util.h"
#include "view.h"

KrkClass * TextView = NULL;

const char * COLOR_FG        = "2;230;230;230";
const char * COLOR_BG        = "2;31;31;31";
const char * COLOR_ALT_FG    = "2;122;122;122";
const char * COLOR_ALT_BG    = "2;46;43;46";
const char * COLOR_NUMBER_FG = "2;150;139;57";
const char * COLOR_NUMBER_BG = "2;0;0;0";
const char * COLOR_STATUS_FG = "2;230;230;230";
const char * COLOR_STATUS_BG = "2;71;64;58";
const char * COLOR_STATUS_ALT= "2;122;122;122";
const char * COLOR_TABBAR_BG = "2;71;64;58";
const char * COLOR_TAB_BG    = "2;71;64;58";
const char * COLOR_ERROR_FG  = "5;15";
const char * COLOR_ERROR_BG  = "5;196";
const char * COLOR_SEARCH_FG = "5;234";
const char * COLOR_SEARCH_BG = "5;226";
const char * COLOR_KEYWORD   = "2;51;162;230";
const char * COLOR_STRING    = "2;72;176;72";
const char * COLOR_COMMENT   = "2;158;153;129;3";
const char * COLOR_TYPE      = "2;230;206;110";
const char * COLOR_PRAGMA    = "2;194;70;54";
const char * COLOR_NUMERAL   = "2;230;43;127";
const char * COLOR_SELECTFG  = "2;0;43;54";
const char * COLOR_SELECTBG  = "2;147;161;161";
const char * COLOR_RED       = "2;222;53;53";
const char * COLOR_GREEN     = "2;55;167;0";
const char * COLOR_BOLD      = "2;2;230;230;230;1";
const char * COLOR_LINK      = "2;51;162;230;4";
const char * COLOR_ESCAPE    = "2;113;203;173";

const char * tab_indicator = "»";
const char * space_indicator = "·";

const char * flag_to_color(int _flag) {
	int flag = _flag & 0xF;
	switch (flag) {
		case FLAG_KEYWORD:
			return COLOR_KEYWORD;
		case FLAG_STRING:
			return COLOR_STRING;
		case FLAG_COMMENT:
			return COLOR_COMMENT;
		case FLAG_TYPE:
			return COLOR_TYPE;
		case FLAG_NUMERAL:
			return COLOR_NUMERAL;
		case FLAG_PRAGMA:
			return COLOR_PRAGMA;
		case FLAG_DIFFPLUS:
			return COLOR_GREEN;
		case FLAG_DIFFMINUS:
			return COLOR_RED;
		case FLAG_SELECT:
			return COLOR_FG;
		case FLAG_BOLD:
			return COLOR_BOLD;
		case FLAG_LINK:
			return COLOR_LINK;
		case FLAG_ESCAPE:
			return COLOR_ESCAPE;
		default:
			return COLOR_FG;
	}
}

/**
 * Given two color codes from a theme, convert them to an escape
 * sequence to set the foreground and background color. This allows
 * us to support basic 16, xterm 256, and true color in themes.
 */
char * color_string(const char * fg, const char * bg) {
	static char output[100];
	char * t = output;
	t += sprintf(t,"\033[22;23;24;");
	if (*bg == '@') {
		int _bg = atoi(bg+1);
		if (_bg < 10) {
			t += sprintf(t, "4%d;", _bg);
		} else {
			t += sprintf(t, "10%d;", _bg-10);
		}
	} else {
		t += sprintf(t, "48;%s;", bg);
	}
	if (*fg == '@') {
		int _fg = atoi(fg+1);
		if (_fg < 10) {
			t += sprintf(t, "3%dm", _fg);
		} else {
			t += sprintf(t, "9%dm", _fg-10);
		}
	} else {
		t += sprintf(t, "38;%sm", fg);
	}
	return output;
}

/**
 * Set text colors
 *
 * Normally, text colors are just strings, but if they
 * start with @ they will be parsed as integers
 * representing one of the 16 standard colors, suitable
 * for terminals without support for the 256- or 24-bit
 * color modes.
 */
void set_colors(const char * fg, const char * bg) {
	printf("%s", color_string(fg, bg));
}

/**
 * Set just the foreground color
 *
 * (See set_colors above)
 */
void set_fg_color(const char * fg) {
	printf("\033[22;23;24;");
	if (*fg == '@') {
		int _fg = atoi(fg+1);
		if (_fg < 10) {
			printf("3%dm", _fg);
		} else {
			printf("9%dm", _fg-10);
		}
	} else {
		printf("38;%sm", fg);
	}
}

int buffer_line_render(buffer_t * env, int line_no, int width, int offset) {
	line_t * line = env->lines[line_no];
	int i = 0; /* Offset in char_t line data entries */
	int j = 0; /* Offset in terminal cells */

	const char * last_color = NULL;
	int was_selecting = 0, was_searching = 0;

	/* Set default text colors */
	set_colors(COLOR_FG, line->is_current ? COLOR_ALT_BG : COLOR_BG);

	/*
	 * When we are rendering in the middle of a wide character,
	 * we render -'s to fill the remaining amount of the character's width.
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
				set_colors(COLOR_FG, line->is_current ? COLOR_ALT_BG : COLOR_BG);
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
				int out = j;
				/* If it's wide, draw ---> as needed */
				while (j - offset < width - 1) {
					printf(" ");
					j++;
				}

				/* End the line with a > to show it overflows */
				printf("…");
				return out;
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

#define _set_colors(fg,bg) \
	if (!(c.flags & FLAG_SELECT) && !(c.flags & FLAG_SEARCH) && !(was_selecting)) { \
		set_colors(fg,(line->is_current && bg == COLOR_BG) ? COLOR_ALT_BG : bg); \
	}

			/* Render special characters */
			if (c.codepoint == '\t') {
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("%s", tab_indicator);
				for (int i = 1; i < c.display_width; ++i) {
					printf("%s", space_indicator);
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
				/* TODO can_unicode */
				printf("▏");
				_set_colors(last_color ? last_color : COLOR_FG, COLOR_BG);
			} else if (c.codepoint == ' ' && i == line->actual - 1) {
				/* Special case: space at end of line */
				_set_colors(COLOR_ALT_FG, COLOR_ALT_BG);
				printf("%s", space_indicator);
				_set_colors(COLOR_FG, COLOR_BG);
			} else {
				/* Normal characters get output */
				char tmp[7]; /* Max six bytes, use 7 to ensure last is always nil */
				codepoint_to_eight(c.codepoint, tmp);
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

	/* Paint the rest of the line */
	if (j < offset) j = offset;
	for (; j < width + offset; ++j) {
		printf(" ");
	}

	return 0;
}

void place_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

int log_base_10(unsigned int v) {
	int r = (v >= 1000000000) ? 9 : (v >= 100000000) ? 8 : (v >= 10000000) ? 7 :
		(v >= 1000000) ? 6 : (v >= 100000) ? 5 : (v >= 10000) ? 4 :
		(v >= 1000) ? 3 : (v >= 100) ? 2 : (v >= 10) ? 1 : 0;
	return r;
}

int num_width(struct TextView * view) {
	if (!view->value->numbers) return 0;
	int w = log_base_10(view->value->line_count) + 3;
	if (w < 4) return 4;
	return w;
}

void draw_line_number(struct TextView * view, int x) {
	if (!view->value->numbers) return;
	/* Draw the line number */
	if (view->value->lines[x]->is_current) {
		set_colors(COLOR_NUMBER_BG, COLOR_NUMBER_FG);
	} else {
		set_colors(COLOR_NUMBER_FG, COLOR_NUMBER_BG);
	}
	int num_size = num_width(view) - 2; /* Padding */
	for (int y = 0; y < num_size - log_base_10(x + 1); ++y) {
		printf(" ");
	}
	printf("%d%c", x + 1, ' ');
}

void draw_line_continues(struct TextView * view, int x) {
	if (!view->value->numbers) return;
	/* Draw the line number */
	if (view->value->lines[x]->is_current) {
		set_colors(COLOR_NUMBER_BG, COLOR_NUMBER_FG);
	} else {
		set_colors(COLOR_NUMBER_FG, COLOR_NUMBER_BG);
	}
	int num_size = num_width(view);
	for (int y = 0; y < num_size; ++y) {
		printf(" ");
	}
}

void view_redraw(struct TextView * view) {
	int line = view->offset;
	int into = 0;

	for (size_t i = 0; i < (size_t)view->h; ++i) {
		place_cursor(view->x, view->y + i);
		if (line < view->value->line_count) {
			if (!into) {
				draw_line_number(view, line);
			} else {
				draw_line_continues(view, line);
			}
			int spot = buffer_line_render(view->value, line, view->w - num_width(view), into);
			if (!spot) {
				line++;
				into = 0;
			} else {
				into = spot;
			}
		} else {
			printf("~");
		}
	}
}

struct TextView * view_from_buffer(buffer_t * buffer) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	struct TextView * out = (void*)krk_newInstance(TextView);
	out->value = buffer;
	out->x = 1;
	out->y = 1;
	out->w = w.ws_col;
	out->h = w.ws_row;
	out->offset = 0;

	return out;
}

#define CURRENT_PTR
#define CURRENT_TYPE  __attribute__((unused)) buffer_t *
#define CURRENT_NAME  env

KRK_FUNC(TextView, __init__, {
	CHECK_ARG(1,TEXTBUFFER,buffer_t *,buffer);
	CHECK_ARG(2,INTEGER,int,x);
	CHECK_ARG(3,INTEGER,int,y);
	CHECK_ARG(4,INTEGER,int,w);
	CHECK_ARG(5,INTEGER,int,h);
	CHECK_ARG(6,INTEGER,int,offset);
	self->value = buffer;
	self->x = x;
	self->y = y;
	self->w = w;
	self->h = h;
	self->offset = offset;
	return argv[0];
})

KRK_FUNC(TextView, redraw, {
	view_redraw(self);
})

KRK_FUNC(TextView,x, { return INTEGER_VAL(self->x); })
KRK_FUNC(TextView,y, { return INTEGER_VAL(self->y); })
KRK_FUNC(TextView,w, { return INTEGER_VAL(self->w); })
KRK_FUNC(TextView,h, { return INTEGER_VAL(self->h); })
KRK_FUNC(TextView,offset, { return INTEGER_VAL(self->offset); })

void kuroko_bind_view(KrkInstance * module) {
	MAKE_CLASS(TextView);
	BIND_METHOD(TextView,__init__);
	BIND_METHOD(TextView,redraw);

	/* Temporary */
	BIND_FIELD(TextView,x);
	BIND_FIELD(TextView,y);
	BIND_FIELD(TextView,w);
	BIND_FIELD(TextView,h);
	BIND_FIELD(TextView,offset);

	krk_finalizeClass(TextView);

}
