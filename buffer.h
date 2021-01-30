#pragma once

#include <stdint.h>
#include "kuroko/src/object.h"

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
 * One step of history.
 */
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
 * History entry types.
 */
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
	unsigned int numbers:1;
	unsigned int gutter:1;
	unsigned int slowop:1;
	unsigned int history_enabled:1;

	int highlighting_paren;
	int maxcolumn;

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
	int start_col;
	int prev_line;
} buffer_t;

extern void buffer_set_history_break(buffer_t * env);
extern void buffer_line_insert(buffer_t * env, int lineno, int offset, char_t c);
extern void buffer_line_delete(buffer_t * env, int lineno, int offset);
extern void buffer_line_replace(buffer_t * env, int lineno, int offset, char_t _c);
extern void buffer_remove_line(buffer_t * env, int offset);
extern void buffer_add_line(buffer_t * env, int offset);
extern void buffer_replace_line(buffer_t * env, int offset, line_t * replacement);
extern void buffer_merge_lines(buffer_t * env, int lineb);
extern void buffer_split_line(buffer_t * env, int line, int split);
extern int buffer_undo(buffer_t * env, int * changedLines, int * changedChars);
extern int buffer_redo(buffer_t * env, int * changedLines, int * changedChars);

extern void buffer_release(buffer_t * env);
extern buffer_t * buffer_new(void);
extern buffer_t * buffer_from_file(char * fileName);

extern KrkClass * TextBuffer;
struct TextBuffer {
	KrkInstance inst;
	buffer_t * value;
};
#define IS_TEXTBUFFER(obj) (krk_isInstanceOf((obj),TextBuffer))
#define AS_TEXTBUFFER(obj) (((struct TextBuffer*)(AS_OBJECT(obj)))->value)

extern KrkClass * TextChar;
struct TextChar {
	KrkInstance inst;
	char_t value;
};
#define IS_TEXTCHAR(obj) (krk_isInstanceOf((obj),TextChar))
#define AS_TEXTCHAR(obj) (((struct TextChar*)(AS_OBJECT(obj)))->value)

extern KrkClass * TextLine;
struct TextLine {
	KrkInstance inst;
	line_t * value;
};
#define IS_TEXTLINE(obj) (krk_isInstanceOf((obj),TextLine))
#define AS_TEXTLINE(obj) (((struct TextLine*)(AS_OBJECT(obj)))->value)
