#pragma once

#define ENTER_KEY     '\r'
#define LINE_FEED     '\n'
#define BACKSPACE_KEY 0x08
#define DELETE_KEY    0x7F
#define DEFAULT_KEY_WAIT (-1)

enum Key {
	KEY_TIMEOUT = -1,
	KEY_CTRL_AT = 0, /* Base */
	KEY_CTRL_A, KEY_CTRL_B, KEY_CTRL_C, KEY_CTRL_D, KEY_CTRL_E, KEY_CTRL_F, KEY_CTRL_G, KEY_CTRL_H,
	KEY_CTRL_I, KEY_CTRL_J, KEY_CTRL_K, KEY_CTRL_L, KEY_CTRL_M, KEY_CTRL_N, KEY_CTRL_O, KEY_CTRL_P,
	KEY_CTRL_Q, KEY_CTRL_R, KEY_CTRL_S, KEY_CTRL_T, KEY_CTRL_U, KEY_CTRL_V, KEY_CTRL_W, KEY_CTRL_X,
	KEY_CTRL_Y, KEY_CTRL_Z, /* Note we keep ctrl-z mapped in termios as suspend */
	KEY_CTRL_OPEN, KEY_CTRL_BACKSLASH, KEY_CTRL_CLOSE, KEY_CTRL_CARAT, KEY_CTRL_UNDERSCORE,
	/* Space... */
	/* Some of these are equivalent to things above */
	KEY_BACKSPACE = 0x08,
	KEY_LINEFEED = '\n',
	KEY_ENTER = '\r',
	KEY_TAB = '\t',
	/* Basic printable characters go here. */
	/* Delete is special */
	KEY_DELETE = 0x7F,
	/* Unicode codepoints go here */
	KEY_ESCAPE = 0x400000, /* Escape would normally be 27, but is special because reasons */
	KEY_F1, KEY_F2, KEY_F3, KEY_F4,
	KEY_F5, KEY_F6, KEY_F7, KEY_F8,
	KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	/* TODO ALT, SHIFT, etc., for F keys */
	KEY_MOUSE, /* Must be followed with a 3-byte mouse read */
	KEY_MOUSE_SGR, /* Followed by an SGR-style sequence of mouse data */
	KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN,
	KEY_UP, KEY_DOWN, KEY_RIGHT, KEY_LEFT,
	KEY_SHIFT_UP, KEY_SHIFT_DOWN, KEY_SHIFT_RIGHT, KEY_SHIFT_LEFT,
	KEY_CTRL_UP, KEY_CTRL_DOWN, KEY_CTRL_RIGHT, KEY_CTRL_LEFT,
	KEY_ALT_UP, KEY_ALT_DOWN, KEY_ALT_RIGHT, KEY_ALT_LEFT,
	KEY_ALT_SHIFT_UP, KEY_ALT_SHIFT_DOWN, KEY_ALT_SHIFT_RIGHT, KEY_ALT_SHIFT_LEFT,
	KEY_SHIFT_TAB,
	/* Special signals for paste start, paste end */
	KEY_PASTE_BEGIN, KEY_PASTE_END,
};

struct key_name_map {
	enum Key keycode;
	char * name;
};

extern struct key_name_map KeyNames[];
extern char * name_from_key(enum Key keycode);
extern enum Key key_from_name(char * name);

extern void input_initialize(void);
extern void input_release(void);
extern int input_getkey(int read_timeout);
