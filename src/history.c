#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include "buffer.h"
#include "config.h"
#include "display.h"
#include "history.h"
#include "navigation.h"
#include "terminal.h"
#include "utf8.h"

/**
 * TODO:
 *
 * The line editing functions should probably take a buffer_t *
 * so that they can act on buffers other than the active one.
 */

void recursive_history_free(history_t * root) {
	if (!root->next) return;

	history_t * n = root->next;
	recursive_history_free(n);

	switch (n->type) {
		case HISTORY_REPLACE_LINE:
			free(n->contents.remove_replace_line.contents);
			/* fall-through */
		case HISTORY_REMOVE_LINE:
			free(n->contents.remove_replace_line.old_contents);
			break;
		default:
			/* Nothing extra to free */
			break;
	}

	free(n);
	root->next = NULL;
}

/**
 * Mark a point where a complete set of actions has ended.
 */
void set_history_break(void) {
	if (!global_config.history_enabled) return;

	if (env->history->type != HISTORY_BREAK && env->history->type != HISTORY_SENTINEL) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_BREAK;
		HIST_APPEND(e);
	}
}

/**
 * Undo a history entry.
 */
void undo_history(void) {
	if (!global_config.history_enabled) return;

	env->loading = 1;
	history_t * e = env->history;

	if (e->type == HISTORY_SENTINEL) {
		env->loading = 0;
		render_commandline_message("Already at oldest change");
		return;
	}

	int count_chars = 0;
	int count_lines = 0;

	do {
		if (e->type == HISTORY_SENTINEL) break;

		switch (e->type) {
			case HISTORY_INSERT:
				/* Delete */
				line_delete(
						env->lines[e->contents.insert_delete_replace.lineno],
						e->contents.insert_delete_replace.offset+1,
						e->contents.insert_delete_replace.lineno
				);
				count_chars++;
				break;
			case HISTORY_DELETE:
				{
					char_t _c = {codepoint_width(e->contents.insert_delete_replace.old_codepoint),0,e->contents.insert_delete_replace.old_codepoint};
					env->lines[e->contents.insert_delete_replace.lineno] = line_insert(
							env->lines[e->contents.insert_delete_replace.lineno],
							_c,
							e->contents.insert_delete_replace.offset-1,
							e->contents.insert_delete_replace.lineno
					);
				}
				count_chars++;
				break;
			case HISTORY_REPLACE:
				{
					char_t _o = {codepoint_width(e->contents.insert_delete_replace.old_codepoint),0,e->contents.insert_delete_replace.old_codepoint};
					line_replace(
							env->lines[e->contents.insert_delete_replace.lineno],
							_o,
							e->contents.insert_delete_replace.offset,
							e->contents.insert_delete_replace.lineno
					);
				}
				count_chars++;
				break;
			case HISTORY_REMOVE_LINE:
				env->lines = add_line(env->lines, e->contents.remove_replace_line.lineno);
				replace_line(env->lines, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.old_contents);
				count_lines++;
				break;
			case HISTORY_ADD_LINE:
				env->lines = remove_line(env->lines, e->contents.add_merge_split_lines.lineno);
				count_lines++;
				break;
			case HISTORY_REPLACE_LINE:
				replace_line(env->lines, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.old_contents);
				count_lines++;
				break;
			case HISTORY_SPLIT_LINE:
				env->lines = merge_lines(env->lines, e->contents.add_merge_split_lines.lineno+1);
				count_lines++;
				break;
			case HISTORY_MERGE_LINES:
				env->lines = split_line(env->lines, e->contents.add_merge_split_lines.lineno-1, e->contents.add_merge_split_lines.split);
				count_lines++;
				break;
			case HISTORY_BREAK:
				/* Ignore break */
				break;
			default:
				render_error("Unknown type %d!\n", e->type);
				break;
		}

		env->line_no = env->history->line;
		env->col_no = env->history->col;

		env->history = e->previous;
		e = env->history;
	} while (e->type != HISTORY_BREAK);

	if (env->line_no > env->line_count) env->line_no = env->line_count;
	if (env->line_no < 1) env->line_no = 1;
	if (env->col_no > env->lines[env->line_no-1]->actual) env->col_no = env->lines[env->line_no-1]->actual;
	if (env->col_no < 1) env->col_no = 1;

	env->modified = (env->history != env->last_save_history);

	env->loading = 0;

	for (int i = 0; i < env->line_count; ++i) {
		env->lines[i]->istate = 0;
		recalculate_tabs(env->lines[i]);
	}
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_syntax(env->lines[i],i);
	}
	place_cursor_actual();
	update_title();
	redraw_all();
	render_commandline_message("%d character%s, %d line%s changed",
			count_chars, (count_chars == 1) ? "" : "s",
			count_lines, (count_lines == 1) ? "" : "s");
}

/**
 * Replay a history entry.
 */
void redo_history(void) {
	if (!global_config.history_enabled) return;

	env->loading = 1;
	history_t * e = env->history->next;

	if (!e) {
		env->loading = 0;
		render_commandline_message("Already at newest change");
		return;
	}

	int count_chars = 0;
	int count_lines = 0;

	while (e) {
		if (e->type == HISTORY_BREAK) {
			env->history = e;
			break;
		}

		switch (e->type) {
			case HISTORY_INSERT:
				{
					char_t _c = {codepoint_width(e->contents.insert_delete_replace.codepoint),0,e->contents.insert_delete_replace.codepoint};
					env->lines[e->contents.insert_delete_replace.lineno] = line_insert(
							env->lines[e->contents.insert_delete_replace.lineno],
							_c,
							e->contents.insert_delete_replace.offset,
							e->contents.insert_delete_replace.lineno
					);
				}
				count_chars++;
				break;
			case HISTORY_DELETE:
				/* Delete */
				line_delete(
						env->lines[e->contents.insert_delete_replace.lineno],
						e->contents.insert_delete_replace.offset,
						e->contents.insert_delete_replace.lineno
				);
				count_chars++;
				break;
			case HISTORY_REPLACE:
				{
					char_t _o = {codepoint_width(e->contents.insert_delete_replace.codepoint),0,e->contents.insert_delete_replace.codepoint};
					line_replace(
							env->lines[e->contents.insert_delete_replace.lineno],
							_o,
							e->contents.insert_delete_replace.offset,
							e->contents.insert_delete_replace.lineno
					);
				}
				count_chars++;
				break;
			case HISTORY_ADD_LINE:
				env->lines = add_line(env->lines, e->contents.remove_replace_line.lineno);
				count_lines++;
				break;
			case HISTORY_REMOVE_LINE:
				env->lines = remove_line(env->lines, e->contents.remove_replace_line.lineno);
				count_lines++;
				break;
			case HISTORY_REPLACE_LINE:
				replace_line(env->lines, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.contents);
				count_lines++;
				break;
			case HISTORY_MERGE_LINES:
				env->lines = merge_lines(env->lines, e->contents.add_merge_split_lines.lineno);
				count_lines++;
				break;
			case HISTORY_SPLIT_LINE:
				env->lines = split_line(env->lines, e->contents.add_merge_split_lines.lineno, e->contents.add_merge_split_lines.split);
				count_lines++;
				break;
			case HISTORY_BREAK:
				/* Ignore break */
				break;
			default:
				render_error("Unknown type %d!\n", e->type);
				break;
		}

		env->history = e;
		e = e->next;
	}

	env->line_no = env->history->line;
	env->col_no = env->history->col;

	if (env->line_no > env->line_count) env->line_no = env->line_count;
	if (env->line_no < 1) env->line_no = 1;
	if (env->col_no > env->lines[env->line_no-1]->actual) env->col_no = env->lines[env->line_no-1]->actual;
	if (env->col_no < 1) env->col_no = 1;

	env->modified = (env->history != env->last_save_history);

	env->loading = 0;

	for (int i = 0; i < env->line_count; ++i) {
		env->lines[i]->istate = 0;
		recalculate_tabs(env->lines[i]);
	}
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_syntax(env->lines[i],i);
	}
	place_cursor_actual();
	update_title();
	redraw_all();
	render_commandline_message("%d character%s, %d line%s changed",
			count_chars, (count_chars == 1) ? "" : "s",
			count_lines, (count_lines == 1) ? "" : "s");
}
