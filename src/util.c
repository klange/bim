#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <ctype.h>
#include "config.h"
#include "util.h"
#include "display.h"
#include "utf8.h"

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

/**
 * Handler for 'r'; takes in input to replace a single
 * character in the document. Handles Unicode, so we
 * can replace a character with complex input. Also
 * handles ^V so we can replace with escape sequences
 * we would otherwise gobble up.
 *
 * Does not actually do the replacement; returns -1
 * or the codepoint to replace with.
 */
int read_one_character(char * message) {
	/* Read one character and replace */
	render_commandline_message(message);
	uint32_t state = 0;
	int cin;
	uint32_t c = -1;
	while ((cin = bim_getch())) {
		if (cin == -1) continue;
		if (!decode(&state, &c, cin)) {
			if (c == '\033') {
				c = -1;
				goto _done;
			} else if (c == 22) { /* ctrl-v */
				render_commandline_message(message);
				printf(" ^V");
				fflush(stdout);
				while ((cin = bim_getch()) == -1);
				c = cin;
				goto _done;
			} else {
				goto _done;
			}
		}
	}

_done:
	redraw_commandline();
	return c;
}

/**
 * Check if a string is all numbers.
 */
int is_all_numbers(const char * c) {
	while (*c) {
		if (!isdigit(*c)) return 0;
		c++;
	}
	return 1;
}

/**
 * Check for modified lines in a file by examining `git diff` output.
 * This can be enabled globally in bimrc or per environment with the 'git' option.
 */
int git_examine(char * filename) {
	if (env->modified) return 1;
#ifndef __toaru__
	int fds[2];
	pipe(fds);
	int child = fork();
	if (child == 0) {
		FILE * dev_null = fopen("/dev/null","w");
		close(fds[0]);
		dup2(fds[1], STDOUT_FILENO);
		dup2(fileno(dev_null), STDERR_FILENO);
		char * args[] = {"git","--no-pager","diff","-U0","--no-color","--",filename,NULL};
		exit(execvp("git",args));
	} else if (child < 0) {
		return 1;
	}

	close(fds[1]);
	FILE * f = fdopen(fds[0],"r");

	int line_offset = 0;
	while (!feof(f)) {
		int c = fgetc(f);
		if (c < 0) break;

		if (c == '@' && line_offset == 0) {
			/* Read line offset, count */
			if (fgetc(f) == '@' && fgetc(f) == ' ' && fgetc(f) == '-') {
				/* This algorithm is borrowed from Kakoune and only requires us to parse the @@ line */
				int from_line = 0;
				int from_count = 0;
				int to_line = 0;
				int to_count = 0;
				fscanf(f,"%d",&from_line);
				if (fgetc(f) == ',') {
					fscanf(f,"%d",&from_count);
				} else {
					from_count = 1;
				}
				fscanf(f,"%d",&to_line);
				if (fgetc(f) == ',') {
					fscanf(f,"%d",&to_count);
				} else {
					to_count = 1;
				}

				if (to_line > env->line_count) continue;

				if (from_count == 0 && to_count > 0) {
					/* No -, all + means all of to_count is green */
					for (int i = 0; i < to_count; ++i) {
						env->lines[to_line+i-1]->rev_status = 1; /* Green */
					}
				} else if (from_count > 0 && to_count == 0) {
					/*
					 * No +, all - means we have a deletion. We mark the next line such that it has a red bar at the top
					 * Note that to_line is one lower than the affacted line, so we don't need to mes with indexes.
					 */
					if (to_line >= env->line_count) continue;
					env->lines[to_line]->rev_status = 4; /* Red */
				} else if (from_count > 0 && from_count == to_count) {
					/* from = to, all modified */
					for (int i = 0; i < to_count; ++i) {
						env->lines[to_line+i-1]->rev_status = 3; /* Blue */
					}
				} else if (from_count > 0 && from_count < to_count) {
					/* from < to, some modified, some added */
					for (int i = 0; i < from_count; ++i) {
						env->lines[to_line+i-1]->rev_status = 3; /* Blue */
					}
					for (int i = from_count; i < to_count; ++i) {
						env->lines[to_line+i-1]->rev_status = 1; /* Green */
					}
				} else if (to_count > 0 && from_count > to_count) {
					/* from > to, we deleted but also modified some lines */
					env->lines[to_line-1]->rev_status = 5; /* Red + Blue */
					for (int i = 1; i < to_count-1; ++i) {
						env->lines[to_line+i-1]->rev_status = 3; /* Blue */
					}
				}
			}
		}

		if (c == '\n') {
			line_offset = 0;
			continue;
		}

		line_offset++;
	}

	fclose(f);
#endif
	return 0;
}

int is_whitespace(int codepoint) {
	return codepoint == ' ' || codepoint == '\t';
}

int is_normal(int codepoint) {
	return isalnum(codepoint) || codepoint == '_';
}

int is_special(int codepoint) {
	return !is_normal(codepoint) && !is_whitespace(codepoint);
}

