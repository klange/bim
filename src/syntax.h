#ifndef _BIM_SYNTAX_H
#define _BIM_SYNTAX_H

#include "line.h"

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

extern const char * flag_to_color(int _flag);

struct syntax_state {
	line_t * line;
	int line_no;
	int state;
	int i;
};

#define paint(length, flag) do { for (int i = 0; i < (length) && state->i < state->line->actual; i++, state->i++) { state->line->text[state->i].flags = (flag); } } while (0)
#define charat() (state->i < state->line->actual ? state->line->text[(state->i)].codepoint : -1)
#define nextchar() (state->i + 1 < state->line->actual ? state->line->text[(state->i+1)].codepoint : -1)
#define lastchar() (state->i - 1 >= 0 ? state->line->text[(state->i-1)].codepoint : -1)
#define skip() (state->i++)
#define charrel(x) (state->i + (x) < state->line->actual ? state->line->text[(state->i+(x))].codepoint : -1)

extern int c_keyword_qualifier(int c);

struct syntax_definition {
	char * name;
	char ** ext;
	int (*calculate)(struct syntax_state *);
	int prefers_spaces;
};

extern struct syntax_definition syntaxes[];
extern void recalculate_syntax(line_t * line, int line_no);
extern void recalculate_tabs(line_t * line);

extern struct syntax_definition * match_syntax(char * file);
extern void set_syntax_by_name(const char * name);

#endif // _BIM_SYNTAX_H
