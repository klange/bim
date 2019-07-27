#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include "history.h"
#include "buffer.h"
#include "config.h"

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
