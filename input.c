/**
 * Key input processor.
 *
 * Parses terminal escapes into key codes / codepoints.
 */

#include <termios.h>
#include <poll.h>
#include <unistd.h>
#include "util.h"
#include "input.h"

static int _unget_val = -1;
static void _unget(int c) {
	_unget_val = c;
}

static struct termios old;
static void get_initial_termios(void) {
	tcgetattr(STDOUT_FILENO, &old);
}

static void set_unbuffered(void) {
	struct termios new = old;
	new.c_iflag &= (~ICRNL) & (~IXON);
	new.c_lflag &= (~ICANON) & (~ECHO) & (~ISIG);
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &new);
}

static void set_buffered(void) {
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &old);
}

void input_initialize(void) {
	get_initial_termios();
	set_unbuffered();
}

void input_release(void) {
	set_buffered();
}

static int _get_byte(int timeout) {
	fflush(stdout);

	if (_unget_val != -1) {
		int out = _unget_val;
		_unget_val = -1;
		return out;
	}

	/* TODO poll on other things than just the input */
	struct pollfd fds[1];
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;

	int ret = poll(fds, 1, timeout);

	if (ret > 0 && fds[0].revents & POLLIN) {
		unsigned char buf[1];
		read(STDIN_FILENO, buf, 1);
		return buf[0];
	}

	/* TODO perform background processing? */
	return -1;
}

#define shift_key(i) _shift_key((i), this_buf, &timeout);
static int _shift_key(int i, int this_buf[20], int *timeout) {
	int thing = this_buf[*timeout-1];
	(*timeout) = 0;
	switch (thing) {
		/* There are other combinations we can handle... */
		case '2': return i + 4;
		case '5': return i + 8;
		case '3': return i + 12;
		case '4': return i + 16;
		default: return i;
	}
}

int input_getkey(int read_timeout) {

	int timeout = 0;
	int this_buf[20];

	int cin;
	uint32_t c;
	uint32_t istate = 0;

	while ((cin = _get_byte((timeout == 1) ? 50 : read_timeout))) {
		if (cin == -1) {
			if (timeout && this_buf[timeout-1] == '\033')
				return KEY_ESCAPE;
			return KEY_TIMEOUT;
		}

		if (!codepoint_from_eight(&istate, &c, cin)) {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						continue;
					case KEY_LINEFEED: return KEY_ENTER;
					case KEY_DELETE: return KEY_BACKSPACE;
				}
				return c;
			} else {
				if (timeout >= 1 && this_buf[timeout-1] == '\033' && c == '\033') {
					_unget(c);
					timeout = 0;
					return KEY_ESCAPE;
				}
				if (timeout >= 1 && this_buf[0] == '\033' && c == 'O') {
					this_buf[timeout] = c;
					timeout++;
					continue;
				}
				if (timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == 'O') {
					switch (c) {
						case 'P': return KEY_F1;
						case 'Q': return KEY_F2;
						case 'R': return KEY_F3;
						case 'S': return KEY_F4;
					}
					timeout = 0;
					continue;
				}
				if (timeout >= 1 && this_buf[timeout-1] == '\033' && c != '[') {
					timeout = 0;
					_unget(c);
					return KEY_ESCAPE;
				}
				if (timeout >= 1 && this_buf[timeout-1] == '\033' && c == '[') {
					timeout = 1;
					this_buf[timeout] = c;
					timeout++;
					continue;
				}
				if (timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[' &&
				    (bim_isdigit(c) || (c == ';'))) {
					this_buf[timeout] = c;
					timeout++;
					continue;
				}
				if (timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[') {
					switch (c) {
						case 'M': return KEY_MOUSE;
						case '<': return KEY_MOUSE_SGR;
						case 'A': return shift_key(KEY_UP);
						case 'B': return shift_key(KEY_DOWN);
						case 'C': return shift_key(KEY_RIGHT);
						case 'D': return shift_key(KEY_LEFT);
						case 'H': return KEY_HOME;
						case 'F': return KEY_END;
						case 'I': return KEY_PAGE_UP;
						case 'G': return KEY_PAGE_DOWN;
						case 'Z': return KEY_SHIFT_TAB;
						case '~':
							if (timeout == 3) {
								switch (this_buf[2]) {
									case '1': return KEY_HOME;
									case '3': return KEY_DELETE;
									case '4': return KEY_END;
									case '5': return KEY_PAGE_UP;
									case '6': return KEY_PAGE_DOWN;
								}
							} else if (timeout == 5) {
								if (this_buf[2] == '2' && this_buf[3] == '0' && this_buf[4] == '0') {
									return KEY_PASTE_BEGIN;
								} else if (this_buf[2] == '2' && this_buf[3] == '0' && this_buf[4] == '1') {
									return KEY_PASTE_END;
								}
							} else if (this_buf[2] == '1') {
								switch (this_buf[3]) {
									case '5': return KEY_F5;
									case '7': return KEY_F6;
									case '8': return KEY_F7;
									case '9': return KEY_F8;
								}
							} else if (this_buf[2] == '2') {
								switch (this_buf[3]) {
									case '0': return KEY_F9;
									case '1': return KEY_F10;
									case '3': return KEY_F11;
									case '4': return KEY_F12;
								}
							}
							break;
					}
				}
				timeout = 0;
				continue;
			}
		} else if (istate == UTF8_REJECT) {
			istate = 0;
		}
	}

	return KEY_TIMEOUT;
}

struct key_name_map KeyNames[] = {
	{KEY_TIMEOUT, "[timeout]"},
	{KEY_BACKSPACE, "<backspace>"},
	{KEY_ENTER, "<enter>"},
	{KEY_ESCAPE, "<escape>"},
	{KEY_TAB, "<tab>"},
	{' ', "<space>"},
	/* These are mostly here for markdown output. */
	{'`', "<backtick>"},
	{'|', "<pipe>"},
	{KEY_DELETE, "<del>"},
	{KEY_MOUSE, "<mouse>"},
	{KEY_MOUSE_SGR, "<mouse-sgr>"},
	{KEY_F1, "<f1>"},{KEY_F2, "<f2>"},{KEY_F3, "<f3>"},{KEY_F4, "<f4>"},
	{KEY_F5, "<f5>"},{KEY_F6, "<f6>"},{KEY_F7, "<f7>"},{KEY_F8, "<f8>"},
	{KEY_F9, "<f9>"},{KEY_F10, "<f10>"},{KEY_F11, "<f11>"},{KEY_F12, "<f12>"},
	{KEY_HOME,"<home>"},{KEY_END,"<end>"},{KEY_PAGE_UP,"<page-up>"},{KEY_PAGE_DOWN,"<page-down>"},
	{KEY_UP, "<up>"},{KEY_DOWN, "<down>"},{KEY_RIGHT, "<right>"},{KEY_LEFT, "<left>"},
	{KEY_SHIFT_UP, "<shift-up>"},{KEY_SHIFT_DOWN, "<shift-down>"},{KEY_SHIFT_RIGHT, "<shift-right>"},{KEY_SHIFT_LEFT, "<shift-left>"},
	{KEY_CTRL_UP, "<ctrl-up>"},{KEY_CTRL_DOWN, "<ctrl-down>"},{KEY_CTRL_RIGHT, "<ctrl-right>"},{KEY_CTRL_LEFT, "<ctrl-left>"},
	{KEY_ALT_UP, "<alt-up>"},{KEY_ALT_DOWN, "<alt-down>"},{KEY_ALT_RIGHT, "<alt-right>"},{KEY_ALT_LEFT, "<alt-left>"},
	{KEY_ALT_SHIFT_UP, "<alt-shift-up>"},{KEY_ALT_SHIFT_DOWN, "<alt-shift-down>"},{KEY_ALT_SHIFT_RIGHT, "<alt-shift-right>"},{KEY_ALT_SHIFT_LEFT, "<alt-shift-left>"},
	{KEY_SHIFT_TAB,"<shift-tab>"},
	{KEY_PASTE_BEGIN,"<paste-begin>"},{KEY_PASTE_END,"<paste-end>"},
};

char * name_from_key(enum Key keycode) {
	for (unsigned int i = 0;  i < sizeof(KeyNames)/sizeof(KeyNames[0]); ++i) {
		if (KeyNames[i].keycode == keycode) return KeyNames[i].name;
	}
	static char keyNameTmp[8] = {0};
	if (keycode <= KEY_CTRL_UNDERSCORE) {
		keyNameTmp[0] = '^';
		keyNameTmp[1] = '@' + keycode;
		keyNameTmp[2] = 0;
		return keyNameTmp;
	}
	codepoint_to_eight(keycode, keyNameTmp);
	return keyNameTmp;
}

enum Key key_from_name(char * name) {
	for (unsigned int i = 0;  i < sizeof(KeyNames)/sizeof(KeyNames[0]); ++i) {
		if (!strcmp(KeyNames[i].name, name)) return KeyNames[i].keycode;
	}
	if (name[0] == '^' && name[1] && !name[2]) {
		return name[1] - '@';
	}
	if (!name[1]) return name[0];
	/* Try decoding */
	uint32_t c, state = 0;
	int candidate = -1;
	while (*name) {
		if (!codepoint_from_eight(&state, &c, (unsigned char)*name)) {
			if (candidate == -1) candidate = c;
			else return -1; /* Reject `name` if it is multiple codepoints */
		} else if (state == UTF8_REJECT) {
			return -1;
		}
	}
	return candidate;
}
