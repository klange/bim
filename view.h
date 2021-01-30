#pragma once

#include "kuroko/src/object.h"
#include "buffer.h"
#include "util.h"

extern KrkClass * TextView;
struct TextView {
	KrkInstance inst;
	/* A view has one buffer, but a buffer may have many views. */
	buffer_t * value;

	/* A view is a rectangle somewhere on screen. */
	int x;
	int y;
	int w;
	int h;

	/* A view has a specific line offset into its buffer */
	int offset;
};

#define IS_TEXTVIEW(obj)   (krk_isInstanceOf((obj),TextView))
#define AS_TEXTVIEW(obj)   (((struct TextView*)(AS_OBJECT(obj)))

extern void view_redraw(struct TextView * view);
extern struct TextView * view_from_buffer(buffer_t * buffer);
