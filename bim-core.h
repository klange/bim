#ifndef _BIM_CORE_H
#define _BIM_CORE_H

#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <dirent.h>
#include <poll.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>


#define BIM_VERSION   "1.9.0"
#define BIM_COPYRIGHT "Copyright 2012-2019 K. Lange <\033[3mklange@toaruos.org\033[23m>"

#define BLOCK_SIZE 4096
#define ENTER_KEY     '\r'
#define LINE_FEED     '\n'
#define BACKSPACE_KEY 0x08
#define DELETE_KEY    0x7F

/**
 * Syntax highlighting flags.
 */
#define FLAG_NONE      0
#define FLAG_KEYWORD   1
#define FLAG_STRING    2
#define FLAG_COMMENT   3
#define FLAG_TYPE      4
#define FLAG_PRAGMA    5
#define FLAG_NUMERAL   6
#define FLAG_ERROR     7
#define FLAG_DIFFPLUS  8
#define FLAG_DIFFMINUS 9
#define FLAG_NOTICE    10
#define FLAG_BOLD      11
#define FLAG_LINK      12
#define FLAG_ESCAPE    13

#define FLAG_SELECT    (1 << 5)
#define FLAG_SEARCH    (1 << 6)

/**
 * Line buffer definitions
 *
 * Lines are essentially resizable vectors of char_t structs,
 * which represent single codepoints in the file.
 */
typedef struct {
	uint32_t display_width:4;
	uint32_t flags:7;
	uint32_t codepoint:21;
} __attribute__((packed)) char_t;

/**
 * Lines have available and actual lengths, describing
 * how much space was allocated vs. how much is being
 * used at the moment.
 */
typedef struct {
	int available;
	int actual;
	int istate;
	int is_current;
	int rev_status;
	char_t   text[];
} line_t;

/**
 * Global configuration state
 *
 * At the moment, this is all in a global, but in the future
 * this should be passed around to various functions.
 */
typedef struct {
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
	unsigned int numbers:1;

	int cursor_padding;
	int split_percent;
	int scroll_amount;
	const char * syntax_fallback;
	uint32_t * search;
} global_config_t;

#define HISTORY_SENTINEL     0
#define HISTORY_INSERT       1
#define HISTORY_DELETE       2
#define HISTORY_REPLACE      3
#define HISTORY_REMOVE_LINE  4
#define HISTORY_ADD_LINE     5
#define HISTORY_REPLACE_LINE 6
#define HISTORY_MERGE_LINES  7
#define HISTORY_SPLIT_LINE   8

#define HISTORY_BREAK        10

typedef struct history {
	struct history * previous;
	struct history * next;
	int type;
	int line;
	int col;
	union {
		struct {
			int lineno;
			int offset;
			int codepoint;
			int old_codepoint;
		} insert_delete_replace;

		struct {
			int lineno;
			line_t * contents;
			line_t * old_contents;
		} remove_replace_line;

		struct {
			int lineno;
			int split;
		} add_merge_split_lines;
	} contents;
} history_t;

/**
 * Buffer data
 *
 * A buffer describes a file, and stores
 * its name as well as the editor state
 * (cursor offsets, etc.) and the actual
 * line buffers.
 */
typedef struct _env {
	unsigned int loading:1;
	unsigned int tabs:1;
	unsigned int modified:1;
	unsigned int readonly:1;
	unsigned int indent:1;
	unsigned int checkgitstatusonwrite:1;
	unsigned int crnl:1;

	int highlighting_paren;

	short  mode;
	short  tabstop;

	char * file_name;
	int    offset;
	int    coffset;
	int    line_no;
	int    line_count;
	int    line_avail;
	int    col_no;
	int    preferred_column;
	struct syntax_definition * syntax;
	line_t ** lines;

	history_t * history;
	history_t * last_save_history;

	int width;
	int left;

	int start_line;
	int sel_col;
} buffer_t;

struct theme_def {
	const char * name;
	void (*load)(void);
};

extern struct theme_def * themes;

struct syntax_state {
	line_t * line;
	int line_no;
	int state;
	int i;
};

struct syntax_definition {
	char * name;
	char ** ext;
	int (*calculate)(struct syntax_state *);
	int prefers_spaces;
};

extern struct syntax_definition * syntaxes;

/**
 * Editor mode states
 */
#define MODE_NORMAL 0
#define MODE_INSERT 1
#define MODE_LINE_SELECTION 2
#define MODE_REPLACE 3
#define MODE_CHAR_SELECTION 4
#define MODE_COL_SELECTION 5
#define MODE_COL_INSERT 6

extern global_config_t global_config;

extern const char * COLOR_FG;
extern const char * COLOR_BG;
extern const char * COLOR_ALT_FG;
extern const char * COLOR_ALT_BG;
extern const char * COLOR_NUMBER_FG;
extern const char * COLOR_NUMBER_BG;
extern const char * COLOR_STATUS_FG;
extern const char * COLOR_STATUS_BG;
extern const char * COLOR_TABBAR_BG;
extern const char * COLOR_TAB_BG;
extern const char * COLOR_ERROR_FG;
extern const char * COLOR_ERROR_BG;
extern const char * COLOR_SEARCH_FG;
extern const char * COLOR_SEARCH_BG;
extern const char * COLOR_KEYWORD;
extern const char * COLOR_STRING;
extern const char * COLOR_COMMENT;
extern const char * COLOR_TYPE;
extern const char * COLOR_PRAGMA;
extern const char * COLOR_NUMERAL;
extern const char * COLOR_SELECTFG;
extern const char * COLOR_SELECTBG;
extern const char * COLOR_RED;
extern const char * COLOR_GREEN;
extern const char * COLOR_BOLD;
extern const char * COLOR_LINK;
extern const char * COLOR_ESCAPE;
extern const char * current_theme;

extern buffer_t * env;
extern buffer_t * left_buffer;
extern buffer_t * right_buffer;
#define NAV_BUFFER_MAX 10
extern char nav_buf[NAV_BUFFER_MAX+1];
extern int nav_buffer;
extern int    buffers_len;
extern int    buffers_avail;
extern buffer_t ** buffers;

/**
 * UTF-8 parser state
 */
extern uint32_t state;

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

static inline uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte) {
	static int state_table[32] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xxxxxxx */
		1,1,1,1,1,1,1,1,                 /* 10xxxxxx */
		2,2,2,2,                         /* 110xxxxx */
		3,3,                             /* 1110xxxx */
		4,                               /* 11110xxx */
		1                                /* 11111xxx */
	};

	static int mask_bytes[32] = {
		0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
		0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,0x7F,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x1F,0x1F,0x1F,0x1F,
		0x0F,0x0F,
		0x07,
		0x00
	};

	static int next[5] = {
		0,
		1,
		0,
		2,
		3
	};

	if (*state == UTF8_ACCEPT) {
		*codep = byte & mask_bytes[byte >> 3];
		*state = state_table[byte >> 3];
	} else if (*state > 0) {
		*codep = (byte & 0x3F) | (*codep << 6);
		*state = next[*state];
	}
	return *state;
}

#define HIST_APPEND(e) do { \
		e->col = env->col_no; \
		e->line = env->line_no; \
		if (env->history) { \
			e->previous = env->history; \
			recursive_history_free(env->history); \
			env->history->next = e; \
			e->next = NULL; \
		} \
		env->history = e; \
	} while (0)

#endif /* _BIM_CORE_H */
