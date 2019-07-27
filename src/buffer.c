#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "config.h"
#include "buffer.h"
#include "biminfo.h"
#include "history.h"
#include "line.h"
#include "display.h"
#include "terminal.h"
#include "util.h"
#include "utf8.h"
#include "navigation.h"

/**
 * Pointer to current active buffer
 */
buffer_t * env = NULL;
buffer_t * left_buffer = NULL;
buffer_t * right_buffer = NULL;
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
 * Mark this buffer as modified and
 * redraw the status and tabbar if needed.
 */
void set_modified(void) {
	/* If it was already marked modified, no need to do anything */
	if (env->modified) return;

	/* Mark as modified */
	env->modified = 1;

	/* Redraw some things */
	update_title();
	redraw_tabbar();
	redraw_statusbar();
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

/**
 * Write active buffer to file
 */
void write_file(char * file) {
	if (!file) {
		render_error("Need a file to write to.");
		return;
	}

	FILE * f = fopen(file, "w+");

	if (!f) {
		render_error("Failed to open file for writing.");
		return;
	}

	/* Go through each line and convert it back to UTF-8 */
	int i, j;
	for (i = 0; i < env->line_count; ++i) {
		line_t * line = env->lines[i];
		line->rev_status = 0;
		for (j = 0; j < line->actual; j++) {
			char_t c = line->text[j];
			if (c.codepoint == 0) {
				char buf[1] = {0};
				fwrite(buf, 1, 1, f);
			} else {
				char tmp[8] = {0};
				int i = to_eight(c.codepoint, tmp);
				fwrite(tmp, i, 1, f);
			}
		}
		fputc('\n', f);
	}
	fclose(f);

	/* Mark it no longer modified */
	env->modified = 0;
	env->last_save_history = env->history;

	/* If there was no file name set, set one */
	if (!env->file_name) {
		env->file_name = malloc(strlen(file) + 1);
		memcpy(env->file_name, file, strlen(file) + 1);
	}

	if (env->checkgitstatusonwrite) {
		git_examine(file);
	}

	update_title();
	redraw_all();
}

/**
 * Processs (part of) a file and add it to a buffer.
 */
void add_buffer(uint8_t * buf, int size) {
	for (int i = 0; i < size; ++i) {
		if (!decode(&state, &codepoint_r, buf[i])) {
			uint32_t c = codepoint_r;
			if (c == '\n') {
				env->lines = add_line(env->lines, env->line_no);
				env->col_no = 1;
				env->line_no += 1;
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
 * Create a new buffer from a file.
 */
void open_file(char * file) {
	env = buffer_new();
	env->width = global_config.term_width;
	env->left = 0;
	env->loading = 1;

	setup_buffer(env);

	FILE * f;
	int init_line = -1;

	if (!strcmp(file,"-")) {
		/**
		 * Read file from stdin. stderr provides terminal input.
		 */
		if (isatty(STDIN_FILENO)) {
			if (buffers_len == 1) {
				quit("stdin is a terminal and you tried to open -; not letting you do that");
			}
			close_buffer();
			render_error("stdin is a terminal and you tried to open -; not letting you do that");
			return;
		}
		f = stdin;
		env->modified = 1;
	} else {
		char * l = strrchr(file, ':');
		if (l && is_all_numbers(l+1)) {
			*l = '\0';
			l++;
			init_line = atoi(l);
		}

		struct stat statbuf;
		if (!stat(file, &statbuf) && S_ISDIR(statbuf.st_mode)) {
			DIR * dirp = opendir(file);
			if (!dirp) {
				env->loading = 0;
				return;
			}
			struct dirent * ent = readdir(dirp);
			while (ent) {
				add_buffer((unsigned char*)ent->d_name, strlen(ent->d_name));
				add_buffer((unsigned char*)"\n",1);
				ent = readdir(dirp);
			}
			closedir(dirp);
			env->file_name = strdup(file);
			env->readonly = 1;
			env->loading = 0;
			return;
		}
		f = fopen(file, "r");
		env->file_name = strdup(file);
	}

	if (!f) {
		if (global_config.highlight_on_open) {
			env->syntax = match_syntax(file);
		}
		env->loading = 0;
		if (global_config.go_to_line) {
			goto_line(1);
		}
		if (env->syntax && env->syntax->prefers_spaces) {
			env->tabs = 0;
		}
		return;
	}

	uint8_t buf[BLOCK_SIZE];

	state = 0;

	while (!feof(f) && !ferror(f)) {
		size_t r = fread(buf, 1, BLOCK_SIZE, f);
		add_buffer(buf, r);
	}

	if (ferror(f)) {
		env->loading = 0;
		return;
	}

	if (env->line_no && env->lines[env->line_no-1] && env->lines[env->line_no-1]->actual == 0) {
		/* Remove blank line from end */
		env->lines = remove_line(env->lines, env->line_no-1);
	}

	if (global_config.highlight_on_open) {
		env->syntax = match_syntax(file);
		if (!env->syntax && global_config.syntax_fallback) {
			set_syntax_by_name(global_config.syntax_fallback);
		}
		for (int i = 0; i < env->line_count; ++i) {
			recalculate_syntax(env->lines[i],i);
		}
	}

	/* Try to automatically figure out tabs vs. spaces */
	int tabs = 0, spaces = 0;
	for (int i = 0; i < env->line_count; ++i) {
		if (env->lines[i]->actual > 1) { /* Make sure line has at least some text on it */
			if (env->lines[i]->text[0].codepoint == '\t') tabs++;
			if (env->lines[i]->text[0].codepoint == ' ' &&
				env->lines[i]->text[1].codepoint == ' ') /* Ignore spaces at the start of asterisky C comments */
				spaces++;
		}
	}
	if (spaces > tabs) {
		env->tabs = 0;
	}

	/* TODO figure out tabstop for spaces? */

	env->loading = 0;

	if (global_config.check_git) {
		env->checkgitstatusonwrite = 1;
		git_examine(file);
	}

	for (int i = 0; i < env->line_count; ++i) {
		recalculate_tabs(env->lines[i]);
	}

	if (global_config.go_to_line) {
		if (init_line != -1) {
			goto_line(init_line);
		} else {
			env->line_no = 1;
			env->col_no = 1;
			fetch_from_biminfo(env);
			place_cursor_actual();
			redraw_all();
			set_preferred_column();
		}
	}

	fclose(f);
}

/**
 * Clean up the terminal and exit the editor.
 */
void quit(const char * message) {
	mouse_disable();
	set_buffered();
	reset();
	clear_screen();
	show_cursor();
	unset_alternate_screen();
	if (message) {
		fprintf(stdout, "%s\n", message);
	}
	exit(0);
}

/**
 * Try to quit, but don't continue if there are
 * modified buffers open.
 */
void try_quit(void) {
	for (int i = 0; i < buffers_len; i++ ) {
		buffer_t * _env = buffers[i];
		if (_env->modified) {
			if (_env->file_name) {
				render_error("Modifications made to file `%s` in tab %d. Aborting.", _env->file_name, i+1);
			} else {
				render_error("Unsaved new file in tab %d. Aborting.", i+1);
			}
			return;
		}
	}
	/* Close all buffers */
	while (buffers_len) {
		buffer_close(buffers[0]);
	}
	quit(NULL);
}

