/**
 * Core primitive operations on buffers.
 *
 * All other actions action on the internals of buffers, excepting syntax
 * highlighting, must go through these operations.
 */
#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kuroko/src/kuroko.h"
#include "kuroko/src/object.h"
#include "kuroko/src/vm.h"

#include "buffer.h"
#include "util.h"

/**
 * When a new action that produces history happens and there is forward
 * history that can be redone, we need to erase it as our tree has branched.
 * If we wanted, we could actually story things in a tree structure so that
 * the new branch and the old branch can both stick around, and that should
 * probably be explored in the future...
 */
static void history_free(history_t * root) {
	if (!root->next) return;

	/* Find the last entry so we can free backwards */
	history_t * n = root->next;
	while (n->next) {
		n = n->next;
	}

	/* Free everything after the root, including stored content. */
	while (n != root) {
		history_t * p = n->previous;
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
		n = p;
	}

	root->next = NULL;
}

/**
 * This macro is called by primitives to insert history elements for each
 * primitive action when performing in edit modes.
 */
#define HIST_APPEND(e) do { \
		e->col = env->col_no; \
		e->line = env->line_no; \
		if (env->history) { \
			e->previous = env->history; \
			history_free(env->history); \
			env->history->next = e; \
			e->next = NULL; \
		} \
		env->history = e; \
	} while (0)

/**
 * Mark a point where a complete set of actions has ended.
 * Individual history entries include things like "insert one character"
 * but a user action that should be undone is "insert several characters";
 * breaks should be inserted after a series of primitives when there is
 * a clear "end", such as when switching out of insert mode after typing.
 */
void buffer_set_history_break(buffer_t * env) {
	if (!env->history_enabled) return;

	/* Do not produce duplicate breaks, or add breaks if we are at a sentinel. */
	if (env->history->type != HISTORY_BREAK && env->history->type != HISTORY_SENTINEL) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_BREAK;
		HIST_APPEND(e);
	}
}

/**
 * Recalculate tab widths for all tab characters in a line. This is in this
 * section because a buffer is assumed to have a fixed tabstop and we need to
 * recalculate these as we insert and remove characters from a line.
 */
void buffer_recalculate_line(buffer_t * env, int lineno) {
	if (env->loading) return;
	line_t * line = env->lines[lineno];

	int j = 0;
	for (int i = 0; i < line->actual; ++i) {
		if (line->text[i].codepoint == '\t') {
			line->text[i].display_width = env->tabstop - (j % env->tabstop);
		}
		j += line->text[i].display_width;
	}
}

/**
 * (Primitive) Insert a character into an existing line.
 *
 * This is the most basic primitive action: Take a line, and insert a codepoint
 * into it at a given offset. If `lineno` is not -1, the line is assumed to
 * be part of the active buffer. If inserting a character means the line needs
 * to grow, then it will be reallocated.
 */
void buffer_line_insert(buffer_t * env, int lineno, int offset, char_t c) {

	line_t * line = env->lines[lineno];

	if (!env->loading && env->history_enabled && lineno != -1) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_INSERT;
		e->contents.insert_delete_replace.lineno = lineno;
		e->contents.insert_delete_replace.offset = offset;
		e->contents.insert_delete_replace.codepoint = c.codepoint;
		HIST_APPEND(e);
	}

	/* If there is not enough space... */
	if (line->actual == line->available) {
		/* Expand the line buffer */
		if (line->available == 0) {
			line->available = 8;
		} else {
			line->available *= 2;
		}
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

	if (!env->loading) {
		env->modified = 1;
		line->rev_status = 2; /* Modified */
		buffer_recalculate_line(env, lineno);
	}

	env->lines[lineno] = line;
}

/**
 * (Primitive) Delete a character from a line.
 *
 * Remove the character at the given offset. We never shrink lines, so this
 * does not have a return value, and delete should never be called during
 * a loading operation (though it may be called during a history replay).
 */
void buffer_line_delete(buffer_t * env, int lineno, int offset) {
	line_t * line = env->lines[lineno];

	/* Can't delete character before start of line. */
	if (offset == 0) return;
	/* Can't delete past end of line either */
	if (offset > line->actual) return;

	if (!env->loading && env->history_enabled && lineno != -1) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_DELETE;
		e->contents.insert_delete_replace.lineno = lineno;
		e->contents.insert_delete_replace.offset = offset;
		e->contents.insert_delete_replace.old_codepoint = line->text[offset-1].codepoint;
		HIST_APPEND(e);
	}

	/* If this isn't the last character, we need to move all subsequent characters backwards */
	if (offset < line->actual) {
		memmove(&line->text[offset-1], &line->text[offset], sizeof(char_t) * (line->actual - offset));
	}

	/* The line is one character shorter */
	line->actual -= 1;
	line->rev_status = 2;
	env->modified = 1;

	buffer_recalculate_line(env, lineno);
}

/**
 * (Primitive) Replace a character in a line.
 *
 * Replaces the codepoint at the given offset with a new character. Since this
 * does not involve any size changes, it does not have a return value.
 * Since a replacement character may be a tab, we do still need to recalculate
 * character widths for tabs as they may change.
 */
void buffer_line_replace(buffer_t * env, int lineno, int offset, char_t _c) {
	line_t * line = env->lines[lineno];

	if (!env->loading && env->history_enabled && lineno != -1) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_REPLACE;
		e->contents.insert_delete_replace.lineno = lineno;
		e->contents.insert_delete_replace.offset = offset;
		e->contents.insert_delete_replace.codepoint = _c.codepoint;
		e->contents.insert_delete_replace.old_codepoint = line->text[offset].codepoint;
		HIST_APPEND(e);
	}

	line->text[offset] = _c;

	if (!env->loading) {
		line->rev_status = 2; /* Modified */
		buffer_recalculate_line(env, lineno);
		env->modified = 1;
	}
}

/**
 * (Primitive) Remove a line from the active buffer
 *
 * This primitive is only valid for a buffer. Delete a line, or if this is the
 * only line in the buffer, clear it but keep the line around with no
 * characters. We use the `line_delete` primitive to clear that line,
 * otherwise we are our own primitive and produce history entries.
 *
 * While we do not shrink the `lines` array, it is returned here anyway.
 */
void buffer_remove_line(buffer_t * env, int offset) {
	line_t ** lines = env->lines;

	/* If there is only one line, clear it instead of removing it. */
	if (env->line_count == 1) {
		while (lines[offset]->actual > 0) {
			buffer_line_delete(env, offset, lines[offset]->actual);
		}
		return;
	}

	/* When a line is removed, we need to keep its contents so we
	 * can un-remove it on redo... */
	if (!env->loading && env->history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_REMOVE_LINE;
		e->contents.remove_replace_line.lineno = offset;
		e->contents.remove_replace_line.old_contents = malloc(sizeof(line_t) + sizeof(char_t) * lines[offset]->available);
		memcpy(e->contents.remove_replace_line.old_contents, lines[offset], sizeof(line_t) + sizeof(char_t) * lines[offset]->available);
		HIST_APPEND(e);
	}

	/* Otherwise, free the data used by the line */
	free(lines[offset]);

	/* Move other lines up */
	if (offset < env->line_count-1) {
		memmove(&lines[offset], &lines[offset+1], sizeof(line_t *) * (env->line_count - (offset - 1)));
		lines[env->line_count-1] = NULL;
	}

	/* There is one less line */
	env->line_count -= 1;
	env->lines = lines;
}

/**
 * (Primitive) Add a new line to the active buffer.
 *
 * Inserts a new line into a buffer at the given line offset.
 * Since this grows the buffer, it will return the new line array
 * after reallocation if needed.
 */
void buffer_add_line(buffer_t * env, int offset) {
	line_t ** lines = env->lines;

	/* Invalid offset? */
	if (offset > env->line_count) return;

	if (!env->loading && env->history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_ADD_LINE;
		e->contents.add_merge_split_lines.lineno = offset;
		HIST_APPEND(e);
	}

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
	lines[offset] = calloc(sizeof(line_t) + sizeof(char_t) * 32, 1);
	lines[offset]->available = 32;

	/* There is one new line */
	env->line_count += 1;
	env->lines = lines;

	if (!env->loading) {
		lines[offset]->rev_status = 2; /* Modified */
		env->modified = 1;
	}

	if (offset > 0 && !env->loading) {
		buffer_recalculate_line(env, offset-1);
	}

	env->lines = lines;
}

/**
 * (Primitive) Replace a line with data from another line.
 *
 * This is only called when pasting yanks after calling `add_line`,
 * but it allows us to have simpler history for that action.
 */
void buffer_replace_line(buffer_t * env, int offset, line_t * replacement) {
	line_t ** lines = env->lines;

	if (!env->loading && env->history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_REPLACE_LINE;
		e->contents.remove_replace_line.lineno = offset;
		e->contents.remove_replace_line.old_contents = malloc(sizeof(line_t) + sizeof(char_t) * lines[offset]->available);
		memcpy(e->contents.remove_replace_line.old_contents, lines[offset], sizeof(line_t) + sizeof(char_t) * lines[offset]->available);
		e->contents.remove_replace_line.contents = malloc(sizeof(line_t) + sizeof(char_t) * replacement->available);
		memcpy(e->contents.remove_replace_line.contents, replacement, sizeof(line_t) + sizeof(char_t) * replacement->available);
		HIST_APPEND(e);
	}

	if (lines[offset]->available < replacement->actual) {
		lines[offset] = realloc(lines[offset], sizeof(line_t) + sizeof(char_t) * replacement->available);
		lines[offset]->available = replacement->available;
	}
	lines[offset]->actual = replacement->actual;
	memcpy(&lines[offset]->text, &replacement->text, sizeof(char_t) * replacement->actual);

	if (!env->loading) {
		lines[offset]->rev_status = 2;
		env->modified = 1;
		buffer_recalculate_line(env, offset);
	}
}

/**
 * (Primitive) Merge two consecutive lines.
 *
 * Take two lines in a buffer and turn them into one line.
 * `lineb` is the offset of the second line... or the
 * line number of the first line, depending on which indexing
 * system you prefer to think about.
 */
void buffer_merge_lines(buffer_t * env, int lineb) {
	line_t ** lines = env->lines;

	/* linea is the line immediately before lineb */
	int linea = lineb - 1;

	if (!env->loading && env->history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_MERGE_LINES;
		e->contents.add_merge_split_lines.lineno = lineb;
		e->contents.add_merge_split_lines.split = env->lines[linea]->actual;
		HIST_APPEND(e);
	}

	/* If there isn't enough space in linea hold both... */
	if (lines[linea]->available < lines[linea]->actual + lines[lineb]->actual) {
		while (lines[linea]->available < lines[linea]->actual + lines[lineb]->actual) {
			/* ... allocate more space until it fits */
			if (lines[linea]->available == 0) {
				lines[linea]->available = 8;
			} else {
				lines[linea]->available *= 2;
			}
		}
		lines[linea] = realloc(lines[linea], sizeof(line_t) + sizeof(char_t) * lines[linea]->available);
	}

	/* Copy the second line into the first line */
	memcpy(&lines[linea]->text[lines[linea]->actual], &lines[lineb]->text, sizeof(char_t) * lines[lineb]->actual);

	/* The first line is now longer */
	lines[linea]->actual = lines[linea]->actual + lines[lineb]->actual;

	if (!env->loading) {
		lines[linea]->rev_status = 2;
		env->modified = 1;
		buffer_recalculate_line(env, linea);
	}

	/* Remove the second line */
	free(lines[lineb]);

	/* Move other lines up */
	if (lineb < env->line_count) {
		memmove(&lines[lineb], &lines[lineb+1], sizeof(line_t *) * (env->line_count - (lineb - 1)));
		lines[env->line_count-1] = NULL;
	}

	/* There is one less line */
	env->line_count -= 1;
}

/**
 * (Primitive) Split a line into two lines at the given column.
 *
 * Takes one line and makes it two lines. There are some optimizations
 * if you are trying to "split" at the first or last column, which
 * are both just treated as add_line.
 */
void buffer_split_line(buffer_t * env, int line, int split) {
	line_t ** lines = env->lines;

	/* If we're trying to split from the start, just add a new blank line before */
	if (split == 0) {
		buffer_add_line(env, line);
		return;
	}

	if (!env->loading && env->history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_SPLIT_LINE;
		e->contents.add_merge_split_lines.lineno = line;
		e->contents.add_merge_split_lines.split  = split;
		HIST_APPEND(e);
	}

	/* Allocate more space as needed */
	if (env->line_count == env->line_avail) {
		env->line_avail *= 2;
		lines = realloc(lines, sizeof(line_t *) * env->line_avail);
	}

	/* Shift later lines down */
	if (line < env->line_count) {
		memmove(&lines[line+2], &lines[line+1], sizeof(line_t *) * (env->line_count - line));
	}

	int remaining = lines[line]->actual - split;

	/* This is some wacky math to get a good power-of-two */
	int v = remaining;
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	/* Allocate space for the new line */
	lines[line+1] = calloc(sizeof(line_t) + sizeof(char_t) * v, 1);
	lines[line+1]->available = v;
	lines[line+1]->actual = remaining;

	/* Move the data from the old line into the new line */
	memmove(lines[line+1]->text, &lines[line]->text[split], sizeof(char_t) * remaining);
	lines[line]->actual = split;

	if (!env->loading) {
		env->modified = 1;
		lines[line]->rev_status = 2;
		lines[line+1]->rev_status = 2;
		buffer_recalculate_line(env, line);
		buffer_recalculate_line(env, line+1);
	}

	/* There is one new line */
	env->line_count += 1;

	/* We may have reallocated lines */
	env->lines = lines;
}

int buffer_undo(buffer_t * env, int * changedLines, int * changedChars) {
	if (!env->history_enabled) return 0; /* No response */

	env->loading = 1;
	history_t * e = env->history;

	if (e->type == HISTORY_SENTINEL) {
		env->loading = 0;
		return -1; /* "Already at oldest change." */
	}

	int count_chars = 0;
	int count_lines = 0;

	do {
		if (e->type == HISTORY_SENTINEL) break;
		switch (e->type) {
			case HISTORY_INSERT:
				/* Delete */
				buffer_line_delete(env, e->contents.insert_delete_replace.lineno, e->contents.insert_delete_replace.offset + 1);
				count_chars++;
				break;
			case HISTORY_DELETE: {
				char_t _c = {codepoint_width(e->contents.insert_delete_replace.old_codepoint), 0, e->contents.insert_delete_replace.old_codepoint};
				buffer_line_insert(env, e->contents.insert_delete_replace.lineno, e->contents.insert_delete_replace.offset - 1, _c);
				count_chars++;
				break;
			}
			case HISTORY_REPLACE: {
				char_t _o = {codepoint_width(e->contents.insert_delete_replace.old_codepoint),0,e->contents.insert_delete_replace.old_codepoint};
				buffer_line_replace(env, e->contents.insert_delete_replace.lineno,e->contents.insert_delete_replace.offset, _o);
				count_chars++;
				break;
			}
			case HISTORY_REMOVE_LINE:
				buffer_add_line(env, e->contents.remove_replace_line.lineno);
				buffer_replace_line(env, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.old_contents);
				count_lines++;
				break;
			case HISTORY_ADD_LINE:
				buffer_remove_line(env, e->contents.add_merge_split_lines.lineno);
				count_lines++;
				break;
			case HISTORY_REPLACE_LINE:
				buffer_replace_line(env, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.old_contents);
				count_lines++;
				break;
			case HISTORY_SPLIT_LINE:
				buffer_merge_lines(env, e->contents.add_merge_split_lines.lineno + 1);
				count_lines++;
				break;
			case HISTORY_MERGE_LINES:
				buffer_split_line(env, e->contents.add_merge_split_lines.lineno - 1, e->contents.add_merge_split_lines.split);
				count_lines++;
				break;
			case HISTORY_BREAK:
				/* Ignore break */
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
		buffer_recalculate_line(env, i);
	}

	*changedChars = count_chars;
	*changedLines = count_lines;
	return 1;
}

int buffer_redo(buffer_t * env, int * changedLines, int * changedChars) {
	if (!env->history_enabled) return 0;

	env->loading = 1;
	history_t * e = env->history->next;

	if (!e) {
		env->loading = 0;
		return -1; /* "Already at newest change" */
	}

	int count_chars = 0;
	int count_lines = 0;

	while (e) {
		if (e->type == HISTORY_BREAK) {
			env->history = e;
			break;
		}

		switch (e->type) {
			case HISTORY_INSERT: {
				char_t _c = {codepoint_width(e->contents.insert_delete_replace.codepoint),0,e->contents.insert_delete_replace.codepoint};
				buffer_line_insert(env, e->contents.insert_delete_replace.lineno, e->contents.insert_delete_replace.offset, _c);
				count_chars++;
				break;
			}
			case HISTORY_DELETE:
				/* Delete */
				buffer_line_delete(env, e->contents.insert_delete_replace.lineno, e->contents.insert_delete_replace.offset);
				count_chars++;
				break;
			case HISTORY_REPLACE: {
				char_t _o = {codepoint_width(e->contents.insert_delete_replace.codepoint),0,e->contents.insert_delete_replace.codepoint};
				buffer_line_replace(env, e->contents.insert_delete_replace.lineno, e->contents.insert_delete_replace.offset, _o);
				count_chars++;
				break;
			}
			case HISTORY_ADD_LINE:
				buffer_add_line(env, e->contents.remove_replace_line.lineno);
				count_lines++;
				break;
			case HISTORY_REMOVE_LINE:
				buffer_remove_line(env, e->contents.remove_replace_line.lineno);
				count_lines++;
				break;
			case HISTORY_REPLACE_LINE:
				buffer_replace_line(env, e->contents.remove_replace_line.lineno, e->contents.remove_replace_line.contents);
				count_lines++;
				break;
			case HISTORY_MERGE_LINES:
				buffer_merge_lines(env, e->contents.add_merge_split_lines.lineno);
				count_lines++;
				break;
			case HISTORY_SPLIT_LINE:
				buffer_split_line(env, e->contents.add_merge_split_lines.lineno, e->contents.add_merge_split_lines.split);
				count_lines++;
				break;
			case HISTORY_BREAK:
				/* Ignore break */
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
		buffer_recalculate_line(env, i);
	}

	*changedChars = count_chars;
	*changedLines = count_lines;
	return 1;
}

/**
 * Creates a new, empty buffer.
 * @returns buffer_t *
 */
buffer_t * buffer_new(void) {
	buffer_t * env = calloc(sizeof(buffer_t), 1);

	/* Set general defaults. */
	env->line_no = 1; /* TODO replace with views */
	env->col_no  = 1; /* TODO replace with views */
	env->offset  = 0; /* TODO replace with views */
	env->left    = 0; /* TODO replace with views */
	env->width   = 0; /* TODO replace with views */

	env->line_count = 1; /* Buffers always have at least one line. */
	env->line_avail = 8; /* Default buffer line capacity. */
	env->modified   = 0; /* Buffers start in a pristine state. */
	env->readonly   = 0; /* Set by the caller if true. */
	env->tabs       = 1; /* Assume tabs by default; syntax highlighter will fix */
	env->tabstop    = 4; /* General default */
	env->indent     = 1; /* Enable indentation by default */
	env->numbers    = 1;
	env->history_enabled = 1;
	env->history    = calloc(1, sizeof(struct history));
	env->last_save_history = env->history;
	env->lines      = malloc(sizeof(line_t *) * env->line_avail);
	env->lines[0]   = calloc(sizeof(line_t) + sizeof(char_t *) * 32, 1);
	env->lines[0]->available = 32;

	return env;
}

/**
 * Destroy and free resources used by a buffer.
 * Caller must make sure there are no dangling references
 * to this buffer (eg. from Kuroko).
 */
void buffer_release(buffer_t * env) {
	/* TODO biminfo */
	/* TODO background tasks associated with this buffer */

	/* free lines */
	for (int i = 0; i < env->line_count; ++i) {
		free(env->lines[i]);
	}

	free(env->lines);

	history_t * h = env->history;
	while (h->next) {
		h = h->next;
	}
	while (h) {
		history_t * p = h->previous;
		free(h);
		h = p;
	}

	if (env->file_name) free(env->file_name);

	free(env);
}

/**
 * (private) Add raw UTF8 to a buffer being loaded
 */
static uint32_t _buffer_add(buffer_t * env, uint8_t * buf, size_t size, uint32_t decoderState) {
	uint32_t codepoint_r = 0;
	for (size_t i = 0; i < size; ++i) {
		if (!codepoint_from_eight(&decoderState, &codepoint_r, buf[i])) {
			uint32_t c = codepoint_r;
			if (c == '\n') {
				buffer_add_line(env, env->line_count);
			} else {
				char_t _c = {codepoint_width((wchar_t)c), 0, c};
				buffer_line_insert(env, env->line_count - 1, env->lines[env->line_count-1]->actual, _c);
			}
		} else if (decoderState == UTF8_REJECT) {
			decoderState = 0;
		}
	}
	return decoderState;
}

/**
 * Creates a new buffer from a file.
 * @param   fileName Name of the file, as provided by the user.
 * @returns buffer_t *
 */
buffer_t * buffer_from_file(char * fileName) {
	buffer_t * env = buffer_new();

	env->loading = 1; /* We are loading this file, so prevent certain internal actions. */

	FILE * f = NULL;
	env->file_name = strdup(fileName);

	if (!strcmp(fileName, "-")) {
		if (isatty(STDIN_FILENO)) {
			buffer_release(env);
			/* TODO krk_runtimeError ? */
			return NULL;
		}

		f = stdin;
		env->modified = 1;
	} else {
		char * _file = fileName;
		if (fileName[0] == '~') {
			char * home = getenv("HOME");
			if (home) {
				_file = malloc(strlen(fileName) + strlen(home) + 4);
				sprintf(_file, "%s%s", home, fileName + 1);
			}
		}

		/* TODO check biminfo */
		/* TODO directories */

		f = fopen(_file, "r");
		if (fileName != _file) free(_file);
	}

	if (!f) {
		/* New file */
		env->loading = 0;
		return env;
	}

#define BLOCK_SIZE 4096
	uint8_t buf[BLOCK_SIZE];
	uint32_t decoderState = 0;

	while (!feof(f) && !ferror(f)) {
		size_t r = fread(buf, 1, BLOCK_SIZE, f);
		decoderState = _buffer_add(env, buf, r, decoderState);
	}

	if (env->line_count && env->lines[env->line_count-1] && env->lines[env->line_no-1]->actual == 0) {
		buffer_remove_line(env, env->line_count - 1);
	}

	/* TODO call syntax highlighters */

	env->loading = 0;

	/* TODO other bindings */

	for (int i = 0; i < env->line_count; ++i) {
		buffer_recalculate_line(env, i);
	}

	/* TODO go to line */

	/* TODO update biminfo */

	fclose(f);

	/* TODO after-load functions? */

	return env;
}

/** Kuroko bindings for all of the above */
KrkClass * TextBuffer = NULL;
KrkClass * TextChar = NULL;
KrkClass * TextLine = NULL;

#define CURRENT_PTR

#define CURRENT_TYPE  buffer_t *
#define CURRENT_NAME  env
KRK_FUNC(TextBuffer,set_history_break, { buffer_set_history_break(env); })
KRK_FUNC(TextBuffer,line_insert, {
	CHECK_ARG(1,INTEGER,int,lineno);
	CHECK_ARG(2,INTEGER,int,offset);
	CHECK_ARG(3,TEXTCHAR,char_t,c);
	buffer_line_insert(env, lineno, offset, c);
})
KRK_FUNC(TextBuffer,line_delete, {
	CHECK_ARG(1,INTEGER,int,lineno);
	CHECK_ARG(2,INTEGER,int,offset);
	buffer_line_delete(env,lineno,offset);
})
KRK_FUNC(TextBuffer,line_replace, {
	CHECK_ARG(1,INTEGER,int,lineno);
	CHECK_ARG(2,INTEGER,int,offset);
	CHECK_ARG(3,TEXTCHAR,char_t,c);
	buffer_line_replace(env,lineno,offset,c);
})
KRK_FUNC(TextBuffer,remove_line, {
	CHECK_ARG(1,INTEGER,int,offset);
	buffer_remove_line(env,offset);
})
KRK_FUNC(TextBuffer,add_line, {
	CHECK_ARG(1,INTEGER,int,offset);
	buffer_add_line(env,offset);
})
KRK_FUNC(TextBuffer,replace_line, {
	CHECK_ARG(1,INTEGER,int,offset);
	CHECK_ARG(2,TEXTLINE,line_t*,replacement);
	buffer_replace_line(env,offset,replacement);
})
KRK_FUNC(TextBuffer,merge_lines, {
	CHECK_ARG(1,INTEGER,int,lineb);
	buffer_merge_lines(env,lineb);
})
KRK_FUNC(TextBuffer,split_lines, {
	CHECK_ARG(1,INTEGER,int,line);
	CHECK_ARG(2,INTEGER,int,split);
	buffer_split_line(env,line,split);
})
KRK_FUNC(TextBuffer,undo, {
	KrkValue outTuple = OBJECT_VAL(krk_newTuple(2));
	int lines;
	int chars;
	int result = buffer_undo(env, &lines, &chars);
	if (result == 0) return BOOLEAN_VAL(0);
	if (result == -1) return NONE_VAL();
	AS_TUPLE(outTuple)->values.values[0] = INTEGER_VAL(lines);
	AS_TUPLE(outTuple)->values.values[1] = INTEGER_VAL(chars);
	AS_TUPLE(outTuple)->values.count = 2;
	return outTuple;
})
KRK_FUNC(TextBuffer,redo, {
	KrkValue outTuple = OBJECT_VAL(krk_newTuple(2));
	int lines;
	int chars;
	int result = buffer_redo(env, &lines, &chars);
	if (result == 0) return BOOLEAN_VAL(0);
	if (result == -1) return NONE_VAL();
	AS_TUPLE(outTuple)->values.values[0] = INTEGER_VAL(lines);
	AS_TUPLE(outTuple)->values.values[1] = INTEGER_VAL(chars);
	AS_TUPLE(outTuple)->values.count = 2;
	return outTuple;
})
KRK_FUNC(TextBuffer,__get__, {
	CHECK_ARG(1,INTEGER,int,lineno);
	if (lineno < 0 || lineno >= env->line_count) return krk_runtimeError(vm.exceptions.indexError, "line number out of range");
	struct TextLine * line = (void*)krk_newInstance(TextLine);
	line->value = env->lines[lineno];
	return OBJECT_VAL(line);
})
KRK_FUNC(TextBuffer,__len__, {
	return INTEGER_VAL(env->line_count);
})
KRK_FUNC(TextBuffer,filename, {
	if (env->file_name)
		return OBJECT_VAL(krk_copyString(env->file_name,strlen(env->file_name)));
})
KRK_FUNC(TextBuffer,modified, {
	return BOOLEAN_VAL(env->modified);
})
KRK_FUNC(TextBuffer,__repr__, {
	char out[1000];
	size_t len = sprintf(out, "TextBuffer('%s',line_count=%d)", env->file_name ? env->file_name : "<No name>", env->line_count);
	return OBJECT_VAL(krk_copyString(out,len));
})
#undef CURRENT_TYPE
#undef CURRENT_NAME

#define CURRENT_TYPE  line_t *
#define CURRENT_NAME  line
KRK_FUNC(TextLine,__get__, {
	CHECK_ARG(1,INTEGER,int,charno);
	if (charno < 0 || charno >= line->actual) return krk_runtimeError(vm.exceptions.indexError, "character offset out of range");
	struct TextChar * c = (void*)krk_newInstance(TextChar);
	c->value = line->text[charno];
	return OBJECT_VAL(c);
})
KRK_FUNC(TextLine,__len__, {
	return INTEGER_VAL(line->actual);
})
KRK_FUNC(TextLine,__repr__, {
	char out[1000];
	char asStr[100] = {'\0'};
	char * c = asStr;
	for (int i = 0; i < line->actual && i < 10; ++i) {
		c += codepoint_to_eight(line->text[i].codepoint, c);
	}
	size_t len = sprintf(out, "TextLine('%s...',actual=%d)", asStr, line->actual);
	return OBJECT_VAL(krk_copyString(out,len));
})
#undef CURRENT_TYPE
#undef CURRENT_NAME
#undef CURRENT_PTR

#define CURRENT_PTR &
#define CURRENT_TYPE char_t *
#define CURRENT_NAME c
KRK_FUNC(TextChar,__init__, {
	CHECK_ARG(1,STRING,KrkString*,str);
	if (str->codesLength != 1) return krk_runtimeError(vm.exceptions.valueError, "TextChar must be initialized from single codepoint");
	krk_unicodeString(str);
	c->codepoint = krk_unicodeCodepoint(str,0);
	c->flags = 0;
	c->display_width = codepoint_width(c->codepoint);
	return argv[0];
})
KRK_FUNC(TextChar,__int__, {
	return INTEGER_VAL(c->codepoint);
})
KRK_FUNC(TextChar,__str__, {
	char tmp[8];
	size_t len = codepoint_to_eight(c->codepoint, tmp);
	return OBJECT_VAL(krk_copyString(tmp,len));
})
KRK_FUNC(TextChar,__repr__, {
	char out[100];
	char tmp[8];
	codepoint_to_eight(c->codepoint, tmp);
	size_t outLen = sprintf(out,"TextChar('%s',display_width=%d,flags=%d)", tmp, c->display_width, c->flags);
	return OBJECT_VAL(krk_copyString(out,outLen));
})
KRK_FUNC(TextChar,display_width, {
	if (argc > 1) {
		return krk_runtimeError(vm.exceptions.valueError, "TextChar.display_width is not mutable");
	}
	return INTEGER_VAL(c->display_width);
})
KRK_FUNC(TextChar,flags, {
	if (argc > 1) {
		CHECK_ARG(1,INTEGER,int,flags);
		c->flags = flags;
	}
	return INTEGER_VAL(c->flags);
})
#undef CURRENT_TYPE
#undef CURRENT_NAME

void kuroko_bind_buffer(KrkInstance * module) {
	MAKE_CLASS(TextBuffer);
	BIND_METHOD(TextBuffer, set_history_break);
	BIND_METHOD(TextBuffer, line_insert);
	BIND_METHOD(TextBuffer, line_delete);
	BIND_METHOD(TextBuffer, line_replace);
	BIND_METHOD(TextBuffer, remove_line);
	BIND_METHOD(TextBuffer, add_line);
	BIND_METHOD(TextBuffer, replace_line);
	BIND_METHOD(TextBuffer, merge_lines);
	BIND_METHOD(TextBuffer, split_lines);
	BIND_METHOD(TextBuffer, undo);
	BIND_METHOD(TextBuffer, redo);
	BIND_METHOD(TextBuffer, __get__);
	BIND_METHOD(TextBuffer, __len__);
	BIND_METHOD(TextBuffer, __repr__);
	BIND_PROP(TextBuffer, filename);
	BIND_PROP(TextBuffer, modified);
	krk_finalizeClass(TextBuffer);

	MAKE_CLASS(TextLine);
	BIND_METHOD(TextLine, __repr__);
	BIND_METHOD(TextLine, __get__);
	BIND_METHOD(TextLine, __len__);
	krk_finalizeClass(TextLine);

	MAKE_CLASS(TextChar);
	BIND_METHOD(TextChar, __init__);
	BIND_METHOD(TextChar, __int__);
	BIND_METHOD(TextChar, __str__);
	BIND_METHOD(TextChar, __repr__);
	BIND_PROP(TextChar, display_width);
	BIND_PROP(TextChar, flags);
	krk_finalizeClass(TextChar);
}

KrkValue kuroko_TextBuffer_from_buffer(buffer_t * env) {
	struct TextBuffer * self = (void*)krk_newInstance(TextBuffer);
	self->value = env;
	return OBJECT_VAL(self);
}
