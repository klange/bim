#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

/**
 * Pointer to current active buffer
 */
buffer_t * env = NULL;

buffer_t * left_buffer = NULL;
buffer_t * right_buffer = NULL;

/**
 * A buffer for holding a number (line, repetition count)
 */
char nav_buf[NAV_BUFFER_MAX+1];
int nav_buffer = 0;

/**
 * Available buffers
 */
int    buffers_len = 0;
int    buffers_avail = 0;
buffer_t ** buffers = NULL;

/**
 * Create a new buffer
 */
buffer_t * buffer_new(void) {
	if (buffers_len == buffers_avail) {
		/* If we are out of buffer space, expand the buffers vector */
		buffers_avail *= 2;
		buffers = realloc(buffers, sizeof(buffer_t *) * buffers_avail);
	}

	/* TODO: Support having split buffers with more than two buffers open */
	if (left_buffer) {
		left_buffer->left = 0;
		left_buffer->width = global_config.term_width;
		right_buffer->left = 0;
		right_buffer->width = global_config.term_width;
		left_buffer = NULL;
		right_buffer = NULL;
	}

	/* Allocate a new buffer */
	buffers[buffers_len] = malloc(sizeof(buffer_t));
	memset(buffers[buffers_len], 0x00, sizeof(buffer_t));
	buffers[buffers_len]->left = 0;
	buffers[buffers_len]->width = global_config.term_width;
	buffers[buffers_len]->highlighting_paren = -1;
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

	update_biminfo(buf);

	/* Clean up lines used by old buffer */
	for (int i = 0; i < buf->line_count; ++i) {
		free(buf->lines[i]);
	}

	free(buf->lines);

	if (buf->file_name) {
		free(buf->file_name);
	}

	history_t * h = buf->history;
	while (h->next) {
		h = h->next;
	}
	while (h) {
		history_t * x = h->previous;
		free(h);
		h = x;
	}

	/* Clean up the old buffer */
	free(buf);

	/* Remove the buffer from the vector, moving others up */
	if (i != buffers_len - 1) {
		memmove(&buffers[i], &buffers[i+1], sizeof(*buffers) * (buffers_len - i));
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
	return buffers[i];
}

/**
 * Insert a character into an existing line.
 */
line_t * line_insert(line_t * line, char_t c, int offset, int lineno) {

	if (!env->loading && global_config.history_enabled && lineno != -1) {
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

	if (!env->loading && global_config.history_enabled && lineno != -1) {
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

	if (!env->loading && global_config.history_enabled && lineno != -1) {
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
	env->readonly    = 0;
	env->offset      = 0;
	env->line_avail  = 8; /* Default line buffer capacity */
	env->tabs        = 1; /* Tabs by default */
	env->tabstop     = 4; /* Tab stop width */
	env->indent      = 1; /* Auto-indent by default */
	env->history     = malloc(sizeof(struct history));
	memset(env->history, 0, sizeof(struct history));
	env->last_save_history = env->history;

	/* Allocate line buffer */
	env->lines = malloc(sizeof(line_t *) * env->line_avail);

	/* Initialize the first line */
	env->lines[0] = calloc(sizeof(line_t) + sizeof(char_t) * 32, 1);
	env->lines[0]->available = 32;
}

/**
 * Processs (part of) a file and add it to a buffer.
 */
void add_buffer(uint8_t * buf, int size) {
	static uint32_t codepoint_r = 0;
	for (int i = 0; i < size; ++i) {
		if (!decode(&state, &codepoint_r, buf[i])) {
			uint32_t c = codepoint_r;
			if (c == '\n') {
				if (!env->crnl && env->lines[env->line_no-1]->actual && env->lines[env->line_no-1]->text[env->lines[env->line_no-1]->actual-1].codepoint == '\r') {
					env->lines[env->line_no-1]->actual--;
					env->crnl = 1;
				}
				env->lines = add_line(env->lines, env->line_no);
				env->col_no = 1;
				env->line_no += 1;
			} else if (env->crnl && c == '\r') {
				continue;
			} else {
				char_t _c;
				_c.codepoint = (uint32_t)c;
				_c.flags = 0;
				_c.display_width = codepoint_width((wchar_t)c);
				line_t * line  = env->lines[env->line_no - 1];
				line_t * nline = line_insert(line, _c, env->col_no - 1, env->line_no-1);
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
 * Close the active buffer
 */
void close_buffer(void) {
	buffer_t * previous_env = env;
	buffer_t * new_env = buffer_close(env);
	if (new_env == previous_env) {
		/* ?? Failed to close buffer */
		render_error("lolwat");
	}
	if (left_buffer && env == left_buffer) {
		left_buffer = NULL;
		right_buffer->left = 0;
		right_buffer->width = global_config.term_width;
		right_buffer = NULL;
	} else if (left_buffer && env == right_buffer) {
		right_buffer = NULL;
		left_buffer->left = 0;
		left_buffer->width = global_config.term_width;
		left_buffer = NULL;
	}
	/* No more buffers, exit */
	if (!new_env) {
		quit(NULL);
	}

	/* Set the new active buffer */
	env = new_env;

	/* Redraw the screen */
	redraw_all();
	update_title();
}
