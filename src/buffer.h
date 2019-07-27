#ifndef _BIM_BUFFER_H
#define _BIM_BUFFER_H

#include <stdint.h>
#include "line.h"
#include "syntax.h"
#include "history.h"
#include "buffer.h"

/**
 * Buffer data
 *
 * A buffer describes a file, and stores
 * its name as well as the editor state
 * (cursor offsets, etc.) and the actual
 * line buffers.
 */
typedef struct _env {
	unsigned short loading:1;
	unsigned short tabs:1;
	unsigned short modified:1;
	unsigned short readonly:1;
	unsigned short indent:1;
	unsigned short highlighting_paren:1;
	unsigned short checkgitstatusonwrite:1;

	short  mode;
	short  tabstop;

	char * file_name;
	int    offset;
	int    coffset;
	int    line_no;
	int    line_count;
	int    line_avail;
	int    col_no;
	int    preferred_column;
	struct syntax_definition * syntax;
	line_t ** lines;

	history_t * history;
	history_t * last_save_history;

	int width;
	int left;

	int start_line;
	int sel_col;
} buffer_t;

extern buffer_t * buffer_new(void);
extern void setup_buffer(buffer_t * env);

extern buffer_t * env;
extern buffer_t * left_buffer;
extern buffer_t * right_buffer;
extern int    buffers_len;
extern int    buffers_avail;
extern buffer_t ** buffers;

extern buffer_t * buffer_new(void);
extern buffer_t * buffer_close(buffer_t * buf);
extern void setup_buffer(buffer_t * env);

#endif // _BIM_BUFFER_H
