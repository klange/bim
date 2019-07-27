#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "buffer.h"
#include "config.h"
#include "display.h"
#include "history.h"
#include "line.h"
#include "navigation.h"
#include "util.h"
#include "utf8.h"

/**
 * Insert a character into an existing line.
 */
line_t * line_insert(line_t * line, char_t c, int offset, int lineno) {

	if (!env->loading && global_config.history_enabled) {
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
		line->rev_status = 2; /* Modified */
		recalculate_tabs(line);
		recalculate_syntax(line, lineno);
	}

	return line;
}

/**
 * Delete a character from a line
 */
void line_delete(line_t * line, int offset, int lineno) {

	/* Can't delete character before start of line. */
	if (offset == 0) return;

	if (!env->loading && global_config.history_enabled) {
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

	recalculate_tabs(line);
	recalculate_syntax(line, lineno);
}

/**
 * Replace a character in a line
 */
void line_replace(line_t * line, char_t _c, int offset, int lineno) {

	if (!env->loading && global_config.history_enabled) {
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
		recalculate_tabs(line);
		recalculate_syntax(line, lineno);
	}
}

/**
 * Remove a line from the active buffer
 */
line_t ** remove_line(line_t ** lines, int offset) {

	/* If there is only one line, clear it instead of removing it. */
	if (env->line_count == 1) {
		while (lines[offset]->actual > 0) {
			line_delete(lines[offset], lines[offset]->actual, offset);
		}
		return lines;
	}

	if (!env->loading && global_config.history_enabled) {
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
	return lines;
}

/**
 * Add a new line to the active buffer.
 */
line_t ** add_line(line_t ** lines, int offset) {

	/* Invalid offset? */
	if (offset > env->line_count) return lines;

	if (!env->loading && global_config.history_enabled) {
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
	}

	if (offset > 0 && !env->loading) {
		recalculate_syntax(lines[offset-1],offset-1);
	}
	return lines;
}

/**
 * Replace a line with data from another line (used by paste to paste yanked lines)
 */
void replace_line(line_t ** lines, int offset, line_t * replacement) {

	if (!env->loading && global_config.history_enabled) {
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
		recalculate_syntax(lines[offset],offset);
	}
}

/**
 * Merge two consecutive lines.
 * lineb is the offset of the second line.
 */
line_t ** merge_lines(line_t ** lines, int lineb) {

	/* linea is the line immediately before lineb */
	int linea = lineb - 1;

	if (!env->loading && global_config.history_enabled) {
		history_t * e = malloc(sizeof(history_t));
		e->type = HISTORY_MERGE_LINES;
		e->contents.add_merge_split_lines.lineno = lineb;
		e->contents.add_merge_split_lines.split = env->lines[linea]->actual;
		HIST_APPEND(e);
	}

	/* If there isn't enough space in linea hold both... */
	while (lines[linea]->available < lines[linea]->actual + lines[lineb]->actual) {
		/* ... allocate more space until it fits */
		if (lines[linea]->available == 0) {
			lines[linea]->available = 8;
		} else {
			lines[linea]->available *= 2;
		}
		/* XXX why not just do this once after calculating appropriate size */
		lines[linea] = realloc(lines[linea], sizeof(line_t) + sizeof(char_t) * lines[linea]->available);
	}

	/* Copy the second line into the first line */
	memcpy(&lines[linea]->text[lines[linea]->actual], &lines[lineb]->text, sizeof(char_t) * lines[lineb]->actual);

	/* The first line is now longer */
	lines[linea]->actual = lines[linea]->actual + lines[lineb]->actual;

	if (!env->loading) {
		lines[linea]->rev_status = 2;
		recalculate_tabs(lines[linea]);
		recalculate_syntax(lines[linea], linea);
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
	return lines;
}

/**
 * Split a line into two lines at the given column
 */
line_t ** split_line(line_t ** lines, int line, int split) {

	/* If we're trying to split from the start, just add a new blank line before */
	if (split == 0) {
		return add_line(lines, line);
	}

	if (!env->loading && global_config.history_enabled) {
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

	/* I have no idea what this is doing */
	int remaining = lines[line]->actual - split;

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
		lines[line]->rev_status = 2;
		lines[line+1]->rev_status = 2;
		recalculate_tabs(lines[line]);
		recalculate_tabs(lines[line+1]);
		recalculate_syntax(lines[line], line);
		recalculate_syntax(lines[line+1], line+1);
	}

	/* There is one new line */
	env->line_count += 1;

	/* We may have reallocated lines */
	return lines;
}

/**
 * Understand spaces and comments and check if the previous line
 * ended with a brace or a colon.
 */
int line_ends_with_brace(line_t * line) {
	int i = line->actual-1;
	while (i >= 0) {
		if ((line->text[i].flags & 0x1F) == FLAG_COMMENT || line->text[i].codepoint == ' ') {
			i--;
		} else {
			break;
		}
	}
	if (i < 0) return 0;
	return (line->text[i].codepoint == '{' || line->text[i].codepoint == ':');
}

int line_is_comment(line_t * line) {
	if (!env->syntax) return 0;

	if (!strcmp(env->syntax->name,"c")) {
		if (line->istate == 1) return 1;
	} else if (!strcmp(env->syntax->name,"java")) {
		if (line->istate == 1) return 1;
	} else if (!strcmp(env->syntax->name,"rust")) {
		if (line->istate > 0) return 1;
	}

	return 0;
}

/**
 * Add indentation from the previous (temporally) line
 */
void add_indent(int new_line, int old_line, int ignore_brace) {
	if (env->indent) {
		int changed = 0;
		if (old_line < new_line && line_is_comment(env->lines[new_line])) {
			for (int i = 0; i < env->lines[old_line]->actual; ++i) {
				if (env->lines[old_line]->text[i].codepoint == '/') {
					if (env->lines[old_line]->text[i+1].codepoint == '*') {
						/* Insert ' * ' */
						char_t space = {1,FLAG_COMMENT,' '};
						char_t asterisk = {1,FLAG_COMMENT,'*'};
						env->lines[new_line] = line_insert(env->lines[new_line],space,i,new_line);
						env->lines[new_line] = line_insert(env->lines[new_line],asterisk,i+1,new_line);
						env->lines[new_line] = line_insert(env->lines[new_line],space,i+2,new_line);
						env->col_no += 3;
					}
					break;
				} else if (env->lines[old_line]->text[i].codepoint == ' ' && env->lines[old_line]->text[i+1].codepoint == '*') {
					/* Insert ' * ' */
					char_t space = {1,FLAG_COMMENT,' '};
					char_t asterisk = {1,FLAG_COMMENT,'*'};
					env->lines[new_line] = line_insert(env->lines[new_line],space,i,new_line);
					env->lines[new_line] = line_insert(env->lines[new_line],asterisk,i+1,new_line);
					env->lines[new_line] = line_insert(env->lines[new_line],space,i+2,new_line);
					env->col_no += 3;
					break;
				} else if (env->lines[old_line]->text[i].codepoint == ' ' ||
					env->lines[old_line]->text[i].codepoint == '\t' ||
					env->lines[old_line]->text[i].codepoint == '*') {
					env->lines[new_line] = line_insert(env->lines[new_line],env->lines[old_line]->text[i],i,new_line);
					env->col_no++;
					changed = 1;
				} else {
					break;
				}
			}
		} else {
			for (int i = 0; i < env->lines[old_line]->actual; ++i) {
				if (old_line < new_line && i == env->lines[old_line]->actual - 3 &&
					env->lines[old_line]->text[i].codepoint == ' ' &&
					env->lines[old_line]->text[i+1].codepoint == '*' &&
					env->lines[old_line]->text[i+2].codepoint == '/') {
					break;
				} else if (env->lines[old_line]->text[i].codepoint == ' ' ||
					env->lines[old_line]->text[i].codepoint == '\t') {
					env->lines[new_line] = line_insert(env->lines[new_line],env->lines[old_line]->text[i],i,new_line);
					env->col_no++;
					changed = 1;
				} else {
					break;
				}
			}
		}
		if (old_line < new_line && !ignore_brace && line_ends_with_brace(env->lines[old_line])) {
			if (env->tabs) {
				char_t c;
				c.codepoint = '\t';
				c.display_width = env->tabstop;
				env->lines[new_line] = line_insert(env->lines[new_line], c, env->col_no-1, new_line);
				env->col_no++;
				changed = 1;
			} else {
				for (int j = 0; j < env->tabstop; ++j) {
					char_t c;
					c.codepoint = ' ';
					c.display_width = 1;
					c.flags = FLAG_SELECT;
					env->lines[new_line] = line_insert(env->lines[new_line], c, env->col_no-1, new_line);
					env->col_no++;
				}
				changed = 1;
			}
		}
		int was_whitespace = 1;
		for (int i = 0; i < env->lines[old_line]->actual; ++i) {
			if (env->lines[old_line]->text[i].codepoint != ' ' &&
				env->lines[old_line]->text[i].codepoint != '\t') {
				was_whitespace = 0;
				break;
			}
		}
		if (was_whitespace) {
			while (env->lines[old_line]->actual) {
				line_delete(env->lines[old_line], env->lines[old_line]->actual, old_line);
			}
		}
		if (changed) {
			recalculate_syntax(env->lines[new_line],new_line);
		}
	}
}

/**
 * Append a character at the current cursor point.
 */
void insert_char(unsigned int c) {
	if (!c) {
		render_error("Inserted nil byte?");
		return;
	}
	char_t _c;
	_c.codepoint = c;
	_c.flags = 0;
	_c.display_width = codepoint_width(c);
	line_t * line  = env->lines[env->line_no - 1];
	line_t * nline = line_insert(line, _c, env->col_no - 1, env->line_no - 1);
	if (line != nline) {
		env->lines[env->line_no - 1] = nline;
	}
	env->col_no += 1;
	set_modified();
}

/**
 * Replace a single character at the current cursor point
 */
void replace_char(unsigned int c) {
	if (env->col_no < 1 || env->col_no > env->lines[env->line_no-1]->actual) return;

	char_t _c;
	_c.codepoint = c;
	_c.flags = 0;
	_c.display_width = codepoint_width(c);

	line_replace(env->lines[env->line_no-1], _c, env->col_no-1, env->line_no-1);

	redraw_line(env->line_no - env->offset - 1, env->line_no-1);
	set_modified();
}

/**
 * Backspace from the current cursor position.
 */
void delete_at_cursor(void) {
	if (env->col_no > 1) {
		line_delete(env->lines[env->line_no - 1], env->col_no - 1, env->line_no - 1);
		env->col_no -= 1;
		if (env->coffset > 0) env->coffset--;
		redraw_line(env->line_no - env->offset - 1, env->line_no-1);
		set_modified();
		redraw_statusbar();
		place_cursor_actual();
	} else if (env->line_no > 1) {
		int tmp = env->lines[env->line_no - 2]->actual;
		merge_lines(env->lines, env->line_no - 1);
		env->line_no -= 1;
		env->col_no = tmp+1;
		set_preferred_column();
		redraw_text();
		set_modified();
		redraw_statusbar();
		place_cursor_actual();
	}
}

/**
 * Delete a "word"; the logic here is a bit complex, but it attempts to do
 * what vim does when you hit ^W (and it's what we bind ^W to as well)
 */
void delete_word(void) {
	if (!env->lines[env->line_no-1]) return;
	if (env->col_no > 1) {

		/* Start by deleting whitespace */
		while (env->col_no > 1 && is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint)) {
			line_delete(env->lines[env->line_no - 1], env->col_no - 1, env->line_no - 1);
			env->col_no -= 1;
			if (env->coffset > 0) env->coffset--;
		}

		int (*inverse_comparator)(int) = is_special;
		if (env->col_no > 1 && is_special(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint)) {
			inverse_comparator = is_normal;
		}

		do {
			if (env->col_no > 1) {
				line_delete(env->lines[env->line_no - 1], env->col_no - 1, env->line_no - 1);
				env->col_no -= 1;
				if (env->coffset > 0) env->coffset--;
			}
		} while (env->col_no > 1 && !is_whitespace(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint) && !inverse_comparator(env->lines[env->line_no - 1]->text[env->col_no - 2].codepoint));

		set_preferred_column();
		redraw_text();
		set_modified();
		redraw_statusbar();
		place_cursor_actual();
	}
}

/**
 * Break the current line in two at the current cursor position.
 */
void insert_line_feed(void) {
	if (env->col_no == env->lines[env->line_no - 1]->actual + 1) {
		env->lines = add_line(env->lines, env->line_no);
	} else {
		env->lines = split_line(env->lines, env->line_no-1, env->col_no - 1);
	}
	env->coffset = 0;
	env->col_no = 1;
	env->line_no += 1;
	set_preferred_column();
	add_indent(env->line_no-1,env->line_no-2,0);
	if (env->line_no > env->offset + global_config.term_height - global_config.bottom_size - 1) {
		env->offset += 1;
	}
	set_modified();
}

/**
 * Yank lines between line start and line end (which may be in either order)
 */
void yank_lines(int start, int end) {
	if (global_config.yanks) {
		for (unsigned int i = 0; i < global_config.yank_count; ++i) {
			free(global_config.yanks[i]);
		}
		free(global_config.yanks);
	}
	int lines_to_yank;
	int start_point;
	if (start <= end) {
		lines_to_yank = end - start + 1;
		start_point = start - 1;
	} else {
		lines_to_yank = start - end + 1;
		start_point = end - 1;
	}
	global_config.yanks = malloc(sizeof(line_t *) * lines_to_yank);
	global_config.yank_count = lines_to_yank;
	global_config.yank_is_full_lines = 1;
	for (int i = 0; i < lines_to_yank; ++i) {
		global_config.yanks[i] = malloc(sizeof(line_t) + sizeof(char_t) * (env->lines[start_point+i]->available));
		global_config.yanks[i]->available = env->lines[start_point+i]->available;
		global_config.yanks[i]->actual = env->lines[start_point+i]->actual;
		global_config.yanks[i]->istate = 0;
		memcpy(&global_config.yanks[i]->text, &env->lines[start_point+i]->text, sizeof(char_t) * (env->lines[start_point+i]->actual));

		for (int j = 0; j < global_config.yanks[i]->actual; ++j) {
			global_config.yanks[i]->text[j].flags = 0;
		}
	}
}

/**
 * Helper to yank part of a line into a new yank line.
 */
void yank_partial_line(int yank_no, int line_no, int start_off, int count) {
	global_config.yanks[yank_no] = malloc(sizeof(line_t) + sizeof(char_t) * (count + 1));
	global_config.yanks[yank_no]->available = count + 1; /* ensure extra space */
	global_config.yanks[yank_no]->actual = count;
	global_config.yanks[yank_no]->istate = 0;
	memcpy(&global_config.yanks[yank_no]->text, &env->lines[line_no]->text[start_off], sizeof(char_t) * count);
	for (int i = 0; i < count; ++i) {
		global_config.yanks[yank_no]->text[i].flags = 0;
	}
}

/**
 * Yank text...
 */
void yank_text(int start_line, int start_col, int end_line, int end_col) {
	if (global_config.yanks) {
		for (unsigned int i = 0; i < global_config.yank_count; ++i) {
			free(global_config.yanks[i]);
		}
		free(global_config.yanks);
	}
	int lines_to_yank = end_line - start_line + 1;
	int start_point = start_line - 1;
	global_config.yanks = malloc(sizeof(line_t *) * lines_to_yank);
	global_config.yank_count = lines_to_yank;
	global_config.yank_is_full_lines = 0;
	if (lines_to_yank == 1) {
		yank_partial_line(0, start_point, start_col - 1, (end_col - start_col + 1));
	} else {
		yank_partial_line(0, start_point, start_col - 1, (env->lines[start_point]->actual - start_col + 1));
		/* Yank middle lines */
		for (int i = 1; i < lines_to_yank - 1; ++i) {
			global_config.yanks[i] = malloc(sizeof(line_t) + sizeof(char_t) * (env->lines[start_point+i]->available));
			global_config.yanks[i]->available = env->lines[start_point+i]->available;
			global_config.yanks[i]->actual = env->lines[start_point+i]->actual;
			global_config.yanks[i]->istate = 0;
			memcpy(&global_config.yanks[i]->text, &env->lines[start_point+i]->text, sizeof(char_t) * (env->lines[start_point+i]->actual));

			for (int j = 0; j < global_config.yanks[i]->actual; ++j) {
				global_config.yanks[i]->text[j].flags = 0;
			}
		}
		/* Yank end line */
		yank_partial_line(lines_to_yank-1, end_line - 1, 0, end_col);
	}
}

