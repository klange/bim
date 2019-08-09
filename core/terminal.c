#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

/**
 * Special implementation of getch with a timeout
 */
int _bim_unget = -1;

void bim_unget(int c) {
	_bim_unget = c;
}

int bim_getch_timeout(int timeout) {
	fflush(stdout);
	if (_bim_unget != -1) {
		int out = _bim_unget;
		_bim_unget = -1;
		return out;
	}
	struct pollfd fds[1];
	fds[0].fd = global_config.tty_in;
	fds[0].events = POLLIN;
	int ret = poll(fds,1,timeout);
	if (ret > 0 && fds[0].revents & POLLIN) {
		unsigned char buf[1];
		read(global_config.tty_in, buf, 1);
		return buf[0];
	} else {
		return -1;
	}
}

/**
 * Toggle buffered / unbuffered modes
 */
struct termios old;
void get_initial_termios(void) {
	tcgetattr(STDOUT_FILENO, &old);
}

void set_unbuffered(void) {
	struct termios new = old;
	new.c_iflag &= (~ICRNL);
	new.c_lflag &= (~ICANON) & (~ECHO);
	new.c_cc[VINTR] = 0;
	new.c_cc[VLNEXT] = 0;
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &new);
}

void set_buffered(void) {
	tcsetattr(STDOUT_FILENO, TCSAFLUSH, &old);
}

/**
 * Move the terminal cursor
 */
void place_cursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

/**
 * Set text colors
 *
 * Normally, text colors are just strings, but if they
 * start with @ they will be parsed as integers
 * representing one of the 16 standard colors, suitable
 * for terminals without support for the 256- or 24-bit
 * color modes.
 */
void set_colors(const char * fg, const char * bg) {
	printf("\033[22;23;24;");
	if (*bg == '@') {
		int _bg = atoi(bg+1);
		if (_bg < 10) {
			printf("4%d;", _bg);
		} else {
			printf("10%d;", _bg-10);
		}
	} else {
		printf("48;%s;", bg);
	}
	if (*fg == '@') {
		int _fg = atoi(fg+1);
		if (_fg < 10) {
			printf("3%dm", _fg);
		} else {
			printf("9%dm", _fg-10);
		}
	} else {
		printf("38;%sm", fg);
	}
}

/**
 * Set just the foreground color
 *
 * (See set_colors above)
 */
void set_fg_color(const char * fg) {
	printf("\033[22;23;24;");
	if (*fg == '@') {
		int _fg = atoi(fg+1);
		if (_fg < 10) {
			printf("3%dm", _fg);
		} else {
			printf("9%dm", _fg-10);
		}
	} else {
		printf("38;%sm", fg);
	}
}

/**
 * Clear the rest of this line
 */
void clear_to_end(void) {
	if (global_config.can_bce) {
		printf("\033[K");
	}
}

/**
 * For terminals without bce,
 * prepaint the whole line, so we don't have to track
 * where the cursor is for everything. Inefficient,
 * but effective.
 */
void paint_line(const char * bg) {
	if (!global_config.can_bce) {
		set_colors(COLOR_FG, bg);
		for (int i = 0; i < global_config.term_width; ++i) {
			printf(" ");
		}
		printf("\r");
	}
}

/**
 * Enable bold text display
 */
void set_bold(void) {
	printf("\033[1m");
}

/**
 * Disable bold
 */
void unset_bold(void) {
	printf("\033[22m");
}

/**
 * Enable underlined text display
 */
void set_underline(void) {
	printf("\033[4m");
}

/**
 * Disable underlined text display
 */
void unset_underline(void) {
	printf("\033[24m");
}

/**
 * Reset text display attributes
 */
void reset(void) {
	printf("\033[0m");
}

/**
 * Clear the entire screen
 */
void clear_screen(void) {
	printf("\033[H\033[2J");
}

/**
 * Hide the cursor
 */
void hide_cursor(void) {
	if (global_config.can_hideshow) {
		printf("\033[?25l");
	}
}

/**
 * Show the cursor
 */
void show_cursor(void) {
	if (global_config.can_hideshow) {
		printf("\033[?25h");
	}
}

/**
 * Store the cursor position
 */
void store_cursor(void) {
	printf("\0337");
}

/**
 * Restore the cursor position.
 */
void restore_cursor(void) {
	printf("\0338");
}

/**
 * Request mouse events
 */
void mouse_enable(void) {
	if (global_config.can_mouse) {
		printf("\033[?1000h");
	}
}

/**
 * Stop mouse events
 */
void mouse_disable(void) {
	if (global_config.can_mouse) {
		printf("\033[?1000l");
	}
}

/**
 * Shift the screen up one line
 */
void shift_up(int amount) {
	printf("\033[%dS", amount);
}

/**
 * Shift the screen down one line.
 */
void shift_down(int amount) {
	printf("\033[%dT", amount);
}

/**
 * Switch to the alternate terminal screen.
 */
void set_alternate_screen(void) {
	if (global_config.can_altscreen) {
		printf("\033[?1049h");
	}
}

/**
 * Restore the standard terminal screen.
 */
void unset_alternate_screen(void) {
	if (global_config.can_altscreen) {
		printf("\033[?1049l");
	}
}

/**
 * Update the terminal title bar
 */
void update_title(void) {
	if (!global_config.can_title) return;

	char cwd[1024] = {'/',0};
	getcwd(cwd, 1024);

	for (int i = 1; i < 3; ++i) {
		printf("\033]%d;%s%s (%s) - Bim\007", i, env->file_name ? env->file_name : "[No Name]", env->modified ? " +" : "", cwd);
	}
}

/**
 * Update screen size
 */
void update_screen_size(void) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	global_config.term_width = w.ws_col;
	global_config.term_height = w.ws_row;
	if (env) {
		if (left_buffer) {
			update_split_size();
		} else if (env != left_buffer && env != right_buffer) {
			env->width = w.ws_col;
		}
	}
	for (int i = 0; i < buffers_len; ++i) {
		if (buffers[i] != left_buffer && buffers[i] != right_buffer) {
			buffers[i]->width = w.ws_col;
		}
	}
}

/**
 * Handle terminal size changes
 */
void SIGWINCH_handler(int sig) {
	(void)sig;
	update_screen_size();
	redraw_all();

	signal(SIGWINCH, SIGWINCH_handler);
}

/**
 * Handle suspend
 */
void SIGTSTP_handler(int sig) {
	(void)sig;
	mouse_disable();
	set_buffered();
	reset();
	clear_screen();
	show_cursor();
	unset_alternate_screen();
	fflush(stdout);

	signal(SIGTSTP, SIG_DFL);
	raise(SIGTSTP);
}

void SIGCONT_handler(int sig) {
	(void)sig;
	set_alternate_screen();
	set_unbuffered();
	update_screen_size();
	mouse_enable();
	redraw_all();
	signal(SIGCONT, SIGCONT_handler);
	signal(SIGTSTP, SIGTSTP_handler);
}
