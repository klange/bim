#ifndef _BIM_HISTORY_H
#define _BIM_HISTORY_H

#include "line.h"

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

extern void recursive_history_free(history_t * root);
extern void set_history_break(void);
extern void undo_history(void);
extern void redo_history(void);

#endif // _BIM_HISTORY_H
