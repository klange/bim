#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "config.h"
#include "util.h"

/**
 * Special implementation of getch with a timeout
 */
int _bim_unget = -1;

void bim_unget(int c) {
	_bim_unget = c;
}

#define bim_getch() bim_getch_timeout(200)
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
 * Get the name of just a file from a full path.
 * Returns a pointer within the original string.
 */
char * file_basename(char * file) {
	char * c = strrchr(file, '/');
	if (!c) return file;
	return (c+1);
}

char nav_buf[NAV_BUFFER_MAX+1] = {0};
int nav_buffer = 0;
