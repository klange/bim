#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "buffer.h"
#include "biminfo.h"
#include "history.h"
#include "line.h"

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

