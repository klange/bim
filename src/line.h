#ifndef _BIM_LINE_H
#define _BIM_LINE_H

#include <stdint.h>

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

extern line_t * line_insert(line_t * line, char_t c, int offset, int lineno);
extern void line_delete(line_t * line, int offset, int lineno);
extern void line_replace(line_t * line, char_t _c, int offset, int lineno);
extern line_t ** remove_line(line_t ** lines, int offset);
extern line_t ** add_line(line_t ** lines, int offset);
extern void replace_line(line_t ** lines, int offset, line_t * replacement);
extern line_t ** merge_lines(line_t ** lines, int lineb);
extern line_t ** split_line(line_t ** lines, int line, int split);
extern int line_ends_with_brace(line_t * line);
extern int line_is_comment(line_t * line);
extern void add_indent(int new_line, int old_line, int ignore_brace);
extern void insert_char(unsigned int c);
extern void replace_char(unsigned int c);
void delete_at_cursor(void);
void delete_word(void);
void insert_line_feed(void);
void yank_lines(int start, int end);
void yank_partial_line(int yank_no, int line_no, int start_off, int count);
void yank_text(int start_line, int start_col, int end_line, int end_col);

#endif // _BIM_LINE_H
