/* Bim - A Text Editor
 *
 * Copyright (C) 2012-2019 K. Lange
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define BIM_VERSION   "1.8.0"
#define BIM_COPYRIGHT "Copyright 2012-2019 K. Lange <\033[3mklange@toaruos.org\033[23m>"

#include "themes.h"
#include "line.h"
#include "syntax.h"
#include "buffer.h"
#include "history.h"
#include "biminfo.h"
#include "util.h"
#include "syntax.h"
#include "line.h"
#include "config.h"
#include "utf8.h"
#include "display.h"
#include "terminal.h"
#include "display.h"
#include "navigation.h"
#include "search.h"

/**
 * Leave INSERT mode
 *
 * If the cursor is too far right, adjust it.
 * Redraw the command line.
 */
void leave_insert(void) {
	if (env->col_no > env->lines[env->line_no-1]->actual) {
		env->col_no = env->lines[env->line_no-1]->actual;
		if (env->col_no == 0) env->col_no = 1;
		set_preferred_column();
	}
	set_history_break();
	env->mode = MODE_NORMAL;
	redraw_commandline();
}

#define COMMAND_HISTORY_MAX 255
char * command_history[COMMAND_HISTORY_MAX] = {NULL};

/**
 * Add a command to the history. If that command was
 * already in history, it is moved to the front of the list;
 * otherwise, the whole list is shifted backwards and
 * overflow is freed up.
 */
void insert_command_history(char * cmd) {
	/* See if this is already in the history. */
	size_t amount_to_shift = COMMAND_HISTORY_MAX - 1;
	for (int i = 0; i < COMMAND_HISTORY_MAX && command_history[i]; ++i) {
		if (!strcmp(command_history[i], cmd)) {
			free(command_history[i]);
			amount_to_shift = i;
			break;
		}
	}

	/* Remove last entry that will roll off the stack */
	if (amount_to_shift == COMMAND_HISTORY_MAX - 1) {
		if (command_history[COMMAND_HISTORY_MAX-1]) free(command_history[COMMAND_HISTORY_MAX-1]);
	}

	/* Roll the history */
	memmove(&command_history[1], &command_history[0], sizeof(char *) * (amount_to_shift));

	command_history[0] = strdup(cmd);
}

/**
 * Add a raw string to a buffer. Convenience wrapper
 * for add_buffer for nil-terminated strings.
 */
static void add_string(char * string) {
	add_buffer((uint8_t*)string,strlen(string));
}

static uint32_t term_colors[] = {
 0x000000, 0xcc0000, 0x3e9a06, 0xc4a000, 0x3465a4, 0x75507b, 0x06989a, 0xeeeeec, 0x555753, 0xef2929, 0x8ae234, 0xfce94f, 0x729fcf, 0xad7fa8, 0x34e2e2,
 0xFFFFFF, 0x000000, 0x00005f, 0x000087, 0x0000af, 0x0000d7, 0x0000ff, 0x005f00, 0x005f5f, 0x005f87, 0x005faf, 0x005fd7, 0x005fff, 0x008700, 0x00875f,
 0x008787, 0x0087af, 0x0087d7, 0x0087ff, 0x00af00, 0x00af5f, 0x00af87, 0x00afaf, 0x00afd7, 0x00afff, 0x00d700, 0x00d75f, 0x00d787, 0x00d7af, 0x00d7d7,
 0x00d7ff, 0x00ff00, 0x00ff5f, 0x00ff87, 0x00ffaf, 0x00ffd7, 0x00ffff, 0x5f0000, 0x5f005f, 0x5f0087, 0x5f00af, 0x5f00d7, 0x5f00ff, 0x5f5f00, 0x5f5f5f,
 0x5f5f87, 0x5f5faf, 0x5f5fd7, 0x5f5fff, 0x5f8700, 0x5f875f, 0x5f8787, 0x5f87af, 0x5f87d7, 0x5f87ff, 0x5faf00, 0x5faf5f, 0x5faf87, 0x5fafaf, 0x5fafd7,
 0x5fafff, 0x5fd700, 0x5fd75f, 0x5fd787, 0x5fd7af, 0x5fd7d7, 0x5fd7ff, 0x5fff00, 0x5fff5f, 0x5fff87, 0x5fffaf, 0x5fffd7, 0x5fffff, 0x870000, 0x87005f,
 0x870087, 0x8700af, 0x8700d7, 0x8700ff, 0x875f00, 0x875f5f, 0x875f87, 0x875faf, 0x875fd7, 0x875fff, 0x878700, 0x87875f, 0x878787, 0x8787af, 0x8787d7,
 0x8787ff, 0x87af00, 0x87af5f, 0x87af87, 0x87afaf, 0x87afd7, 0x87afff, 0x87d700, 0x87d75f, 0x87d787, 0x87d7af, 0x87d7d7, 0x87d7ff, 0x87ff00, 0x87ff5f,
 0x87ff87, 0x87ffaf, 0x87ffd7, 0x87ffff, 0xaf0000, 0xaf005f, 0xaf0087, 0xaf00af, 0xaf00d7, 0xaf00ff, 0xaf5f00, 0xaf5f5f, 0xaf5f87, 0xaf5faf, 0xaf5fd7,
 0xaf5fff, 0xaf8700, 0xaf875f, 0xaf8787, 0xaf87af, 0xaf87d7, 0xaf87ff, 0xafaf00, 0xafaf5f, 0xafaf87, 0xafafaf, 0xafafd7, 0xafafff, 0xafd700, 0xafd75f,
 0xafd787, 0xafd7af, 0xafd7d7, 0xafd7ff, 0xafff00, 0xafff5f, 0xafff87, 0xafffaf, 0xafffd7, 0xafffff, 0xd70000, 0xd7005f, 0xd70087, 0xd700af, 0xd700d7,
 0xd700ff, 0xd75f00, 0xd75f5f, 0xd75f87, 0xd75faf, 0xd75fd7, 0xd75fff, 0xd78700, 0xd7875f, 0xd78787, 0xd787af, 0xd787d7, 0xd787ff, 0xd7af00, 0xd7af5f,
 0xd7af87, 0xd7afaf, 0xd7afd7, 0xd7afff, 0xd7d700, 0xd7d75f, 0xd7d787, 0xd7d7af, 0xd7d7d7, 0xd7d7ff, 0xd7ff00, 0xd7ff5f, 0xd7ff87, 0xd7ffaf, 0xd7ffd7,
 0xd7ffff, 0xff0000, 0xff005f, 0xff0087, 0xff00af, 0xff00d7, 0xff00ff, 0xff5f00, 0xff5f5f, 0xff5f87, 0xff5faf, 0xff5fd7, 0xff5fff, 0xff8700, 0xff875f,
 0xff8787, 0xff87af, 0xff87d7, 0xff87ff, 0xffaf00, 0xffaf5f, 0xffaf87, 0xffafaf, 0xffafd7, 0xffafff, 0xffd700, 0xffd75f, 0xffd787, 0xffd7af, 0xffd7d7,
 0xffd7ff, 0xffff00, 0xffff5f, 0xffff87, 0xffffaf, 0xffffd7, 0xffffff, 0x080808, 0x121212, 0x1c1c1c, 0x262626, 0x303030, 0x3a3a3a, 0x444444, 0x4e4e4e,
 0x585858, 0x626262, 0x6c6c6c, 0x767676, 0x808080, 0x8a8a8a, 0x949494, 0x9e9e9e, 0xa8a8a8, 0xb2b2b2, 0xbcbcbc, 0xc6c6c6, 0xd0d0d0, 0xdadada, 0xe4e4e4,
 0xeeeeee,
};

/**
 * Convert a color setting from terminal format
 * to a hexadecimal color code and add it to the current
 * buffer. This is used for HTML conversion, but could
 * possibly be used for other purposes.
 */
static void html_convert_color(const char * color_string) {
	char tmp[100];
	if (!strncmp(color_string,"2;",2)) {
		/* 24-bit color */
		int red, green, blue;
		sscanf(color_string+2,"%d;%d;%d",&red,&green,&blue);
		sprintf(tmp, "#%02x%02x%02x;", red,green,blue);
	} else if (!strncmp(color_string,"5;",2)) {
		/* 256 colors; needs lookup table */
		int index;
		sscanf(color_string+2,"%d",&index);
		sprintf(tmp,"#%06x;", (unsigned int)term_colors[(index >= 0 && index <= 255) ? index : 0]);
	} else {
		/* 16 colors; needs lookup table */
		int index;
		uint32_t color;
		sscanf(color_string+1,"%d",&index);
		if (index >= 10) {
			index -= 10;
			color = term_colors[index+8];
		} else if (index == 9) {
			color = term_colors[0];
		} else {
			color = term_colors[index];
		}
		sprintf(tmp,"#%06x;", (unsigned int)color);
	}
	add_string(tmp);
	char * italic = strstr(color_string,";3");
	if (italic && italic[2] == '\0') {
		add_string(" font-style: oblique;");
	}
	char * bold = strstr(color_string,";1");
	if (bold && bold[2] == '\0') {
		add_string(" font-weight: bold;");
	}
	char * underline = strstr(color_string,";4");
	if (underline && underline[2] == '\0') {
		add_string(" font-decoration: underline;");
	}
}

/**
 * Based on vim's :TOhtml
 * Convert syntax-highlighted buffer contents to HTML.
 */
void convert_to_html(void) {
	buffer_t * old = env;
	env = buffer_new();
	setup_buffer(env);
	env->loading = 1;

	add_string("<!doctype html>\n");
	add_string("<html>\n");
	add_string("	<head>\n");
	add_string("		<meta charset=\"UTF-8\">\n");
	if (old->file_name) {
		add_string("		<title>");
		add_string(file_basename(old->file_name));
		add_string("</title>\n");
	}
	add_string("		<style>\n");
	add_string("			body {\n");
	add_string("				margin: 0;\n");
	add_string("				-webkit-text-size-adjust: none;\n");
	add_string("				counter-reset: line-no;\n");
	add_string("				background-color: ");
	/* Convert color */
	html_convert_color(COLOR_BG);
	add_string("\n");
	add_string("			}\n");
	for (int i = 0; i < 15; ++i) {
		/* For each of the relevant flags... */
		char tmp[20];
		sprintf(tmp,"			.s%d { color: ", i);
		add_string(tmp);
		/* These are special */
		if (i == FLAG_NOTICE) {
			html_convert_color(COLOR_SEARCH_FG);
			add_string(" background-color: ");
			html_convert_color(COLOR_SEARCH_BG);
		} else if (i == FLAG_ERROR) {
			html_convert_color(COLOR_ERROR_FG);
			add_string(" background-color: ");
			html_convert_color(COLOR_ERROR_BG);
		} else {
			html_convert_color(flag_to_color(i));
		}
		add_string("}\n");
	}
	add_string("			pre {\n");
	add_string("				margin: 0;\n");
	add_string("				white-space: pre-wrap;\n");
	add_string("				font-family: \"DejaVu Sans Mono\", Courier, monospace;\n");
	add_string("				font-size: 10pt;\n");
	add_string("			}\n");
	add_string("			pre>span {\n");
	add_string("				display: inline-block;\n");
	add_string("				width: 100%;\n");
	add_string("			}\n");
	add_string("			pre>span>a::before {\n");
	add_string("				counter-increment: line-no;\n");
	add_string("				content: counter(line-no);\n");
	add_string("				padding-right: 1em;\n");
	add_string("				width: 3em;\n");
	add_string("				display: inline-block;\n");
	add_string("				text-align: right;\n");
	add_string("				background-color: ");
	html_convert_color(COLOR_NUMBER_BG);
	add_string("\n");
	add_string("				color: ");
	html_convert_color(COLOR_NUMBER_FG);
	add_string("\n");
	add_string("			}\n");
	add_string("			pre>span:target {\n");
	add_string("				background-color: ");
	html_convert_color(COLOR_ALT_BG);
	add_string("\n");
	add_string("			}\n");
	add_string("			pre>span:target>a::before {\n");
	add_string("				background-color: ");
	html_convert_color(COLOR_NUMBER_FG);
	add_string("\n");
	add_string("				color: ");
	html_convert_color(COLOR_NUMBER_BG);
	add_string("\n");
	add_string("			}\n");
	for (int i = 1; i <= env->tabstop; ++i) {
		char tmp[10];
		sprintf(tmp, ".tab%d", i);
		add_string("			");
		add_string(tmp);
		add_string(">span {\n");
		add_string("				display: inline-block;\n");
		add_string("				overflow: hidden;\n");
		add_string("				width: 0;\n");
		add_string("				height: 0;\n");
		add_string("			}\n");
		add_string("			");
		add_string(tmp);
		add_string("::after {\n");
		add_string("				content: '»");
		for (int j = 1; j < i; ++j) {
			add_string("·");
		}
		add_string("';\n");
		add_string("				background-color: ");
		html_convert_color(COLOR_ALT_BG);
		add_string("\n");
		add_string("				color: ");
		html_convert_color(COLOR_ALT_FG);
		add_string("\n");
		add_string("			}\n");
	}
	add_string("			.space {\n");
	add_string("				border-left: 1px solid ");
	html_convert_color(COLOR_ALT_FG);
	add_string("\n");
	add_string("				margin-left: -1px;\n");
	add_string("			}\n");
	add_string("		</style>\n");
	add_string("	</head>\n");
	add_string("	<body><pre>\n");

	for (int i = 0; i < old->line_count; ++i) {
		char tmp[100];
		sprintf(tmp, "<span id=\"L%d\"><a href=\"#L%d\"></a>", i+1, i+1);
		add_string(tmp);
		int last_flag = -1;
		int opened = 0;
		int all_spaces = 1;
		for (int j = 0; j < old->lines[i]->actual; ++j) {
			char_t c = old->lines[i]->text[j];

			if (c.codepoint != ' ') all_spaces = 0;

			if (last_flag == -1 || last_flag != (c.flags & 0x1F)) {
				if (opened) add_string("</span>");
				opened = 1;
				char tmp[100];
				sprintf(tmp, "<span class=\"s%d\">", c.flags & 0x1F);
				add_string(tmp);
				last_flag = (c.flags & 0x1F);
			}

			if (c.codepoint == '<') {
				add_string("&lt;");
			} else if (c.codepoint == '>') {
				add_string("&gt;");
			} else if (c.codepoint == '&') {
				add_string("&amp;");
			} else if (c.codepoint == '\t') {
				char tmp[100];
				sprintf(tmp, "<span class=\"tab%d\"><span>	</span></span>",c.display_width);
				add_string(tmp);
			} else if (j > 0 && c.codepoint == ' ' && all_spaces && !(j % old->tabstop)) {
				add_string("<span class=\"space\"> </span>");
			} else {
				char tmp[7] = {0}; /* Max six bytes, use 7 to ensure last is always nil */
				to_eight(c.codepoint, tmp);
				add_string(tmp);
			}
		}
		if (opened) {
			add_string("</span>");
		} else {
			add_string("<wbr>");
		}
		add_string("</span>\n");
	}

	add_string("</pre></body>\n");
	add_string("</html>\n");

	env->loading = 0;
	env->modified = 1;
	if (old->file_name) {
		char * base = file_basename(old->file_name);
		char * tmp = malloc(strlen(base) + 5);
		*tmp = '\0';
		strcat(tmp, base);
		strcat(tmp, ".htm");
		env->file_name = tmp;
	}
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_tabs(env->lines[i]);
	}
	env->syntax = match_syntax(".htm");
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_syntax(env->lines[i],i);
	}
	redraw_all();
}

/**
 * Process a user command.
 */
void process_command(char * cmd) {
	/* Special case ! to run shell commands without parsing tokens */
	int c;

	/* Add command to history */
	insert_command_history(cmd);

	if (*cmd == '!') {
		if (env->mode == MODE_LINE_SELECTION) {
			int range_top, range_bot;
			range_top = env->start_line < env->line_no ? env->start_line : env->line_no;
			range_bot = env->start_line < env->line_no ? env->line_no : env->start_line;

			int in[2];
			pipe(in);
			int out[2];
			pipe(out);
			int child = fork();

			/* Open child process and set up pipes */
			if (child == 0) {
				FILE * dev_null = fopen("/dev/null","w"); /* for stderr */
				close(out[0]);
				close(in[1]);
				dup2(out[1], STDOUT_FILENO);
				dup2(in[0], STDIN_FILENO);
				dup2(fileno(dev_null), STDERR_FILENO);
				system(&cmd[1]); /* Yes we can just do this */
				exit(1);
			} else if (child < 0) {
				render_error("Failed to fork");
				return;
			}
			close(out[1]);
			close(in[0]);

			/* Write lines to child process */
			FILE * f = fdopen(in[1],"w");
			for (int i = range_top; i <= range_bot; ++i) {
				line_t * line = env->lines[i-1];
				for (int j = 0; j < line->actual; j++) {
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
			close(in[1]);

			/* Read results from child process into a new buffer */
			FILE * result = fdopen(out[0],"r");
			buffer_t * old = env;
			env = buffer_new();
			setup_buffer(env);
			env->loading = 1;
			uint8_t buf[BLOCK_SIZE];
			state = 0;
			while (!feof(result) && !ferror(result)) {
				size_t r = fread(buf, 1, BLOCK_SIZE, result);
				add_buffer(buf, r);
			}
			if (env->line_no && env->lines[env->line_no-1] && env->lines[env->line_no-1]->actual == 0) {
				env->lines = remove_line(env->lines, env->line_no-1);
			}
			fclose(result);
			env->loading = 0;

			/* Return to the original buffer and replace the selected lines with the output */
			buffer_t * new = env;
			env = old;
			for (int i = range_top; i <= range_bot; ++i) {
				/* Remove the existing lines */
				env->lines = remove_line(env->lines, range_top-1);
			}
			for (int i = 0; i < new->line_count; ++i) {
				/* Add the new lines */
				env->lines = add_line(env->lines, range_top + i - 1);
				replace_line(env->lines, range_top + i - 1, new->lines[i]);
				recalculate_tabs(env->lines[range_top+i-1]);
			}

			env->modified = 1;

			/* Close the temporary buffer */
			buffer_close(new);
		} else {
			/* Reset and draw some line feeds */
			reset();
			printf("\n\n");

			/* Set buffered for shell application */
			set_buffered();

			/* Call the shell and wait for completion */
			system(&cmd[1]);

			/* Return to the editor, wait for user to press enter. */
			set_unbuffered();
			printf("\n\nPress ENTER to continue.");
			while ((c = bim_getch(), c != ENTER_KEY && c != LINE_FEED));

			/* Redraw the screen */
			redraw_all();
		}

		/* Done processing command */
		return;
	}

	/* Arguments aren't really tokenized, but the first command before a space is extracted */
	char *argv[3]; /* If a specific command wants to tokenize further, it can do that later. */
	int argc = 0;
	if (*cmd) argc++;

	int i = 0;
	char tmp[512] = {0};

	/* Collect up until first space for argv[0] */
	for (char * c = cmd; *c; ++c) {
		if (i == 511) break;
		if (*c == ' ') {
			tmp[i] = '\0';
			argv[1] = c+1;
			if (*argv[1]) argc++;
			break;
		} else {
			tmp[i] = *c;
		}
		i++;
	}
	argv[0] = tmp;
	argv[argc] = NULL;

	if (argc < 1) {
		/* no op */
		return;
	}

	int all_lines = 0;

	if (argv[0][0] == '%') {
		all_lines = 1;
		argv[0]++;
	}

	if (!strcmp(argv[0], "e")) {
		/* e: edit file */
		if (argc > 1) {
			/* This actually opens a new tab */
			open_file(argv[1]);
			update_title();
		} else {
			if (env->modified) {
				render_error("File is modified, can not reload.");
				return;
			}

			buffer_t * old_env = env;
			open_file(env->file_name);
			buffer_t * new_env = env;
			env = old_env;

#define SWAP(T,a,b) do { T x = a; a = b; b = x; } while (0)
			SWAP(line_t **, env->lines, new_env->lines);
			SWAP(int, env->line_count, new_env->line_count);
			SWAP(int, env->line_avail, new_env->line_avail);

			buffer_close(new_env); /* Should probably also free, this needs editing. */
			redraw_all();
		}
	} else if (argv[0][0] == 's' && !isalpha(argv[0][1])) {
		if (!argv[0][1]) {
			render_error("expected substitution argument");
			return;
		}
		/* Substitution */
		int range_top, range_bot;
		if (env->mode == MODE_LINE_SELECTION) {
			range_top = env->start_line < env->line_no ? env->start_line : env->line_no;
			range_bot = env->start_line < env->line_no ? env->line_no : env->start_line;
		} else if (all_lines) {
			range_top = 1;
			range_bot = env->line_count;
		} else {
			range_top = env->line_no;
			range_bot = env->line_no;
		}

		/* Determine replacement parameters */
		char divider = cmd[1];

		char * needle = &cmd[2];
		char * c = needle;
		char * replacement = NULL;
		char * options = "";

		while (*c) {
			if (*c == divider) {
				*c = '\0';
				replacement = c + 1;
				break;
			}
			c++;
		}

		if (!replacement) {
			render_error("nothing to replace with");
			return;
		}

		c = replacement;
		while (*c) {
			if (*c == divider) {
				*c = '\0';
				options = c + 1;
				break;
			}
			c++;
		}

		int global = 0;
		int case_insensitive = 0;

		/* Parse options */
		while (*options) {
			switch (*options) {
				case 'g':
					global = 1;
					break;
				case 'i':
					case_insensitive = 1;
					break;
			}
			options++;
		}

		uint32_t * needle_c = malloc(sizeof(uint32_t) * (strlen(needle) + 1));
		uint32_t * replacement_c = malloc(sizeof(uint32_t) * (strlen(replacement) + 1));

		{
			int i = 0;
			uint32_t c, state = 0;
			for (char * cin = needle; *cin; cin++) {
				if (!decode(&state, &c, *cin)) {
					needle_c[i] = c;
					i++;
				} else if (state == UTF8_REJECT) {
					state = 0;
				}
			}
			needle_c[i] = 0;
			i = 0;
			c = 0;
			state = 0;
			for (char * cin = replacement; *cin; cin++) {
				if (!decode(&state, &c, *cin)) {
					replacement_c[i] = c;
					i++;
				} else if (state == UTF8_REJECT) {
					state = 0;
				}
			}
			replacement_c[i] = 0;
		}

		int replacements = 0;
		for (int line = range_top; line <= range_bot; ++line) {
			int col = 0;
			while (col != -1) {
				perform_replacement(line, needle_c, replacement_c, col, case_insensitive, &col);
				if (col != -1) replacements++;
				if (!global) break;
			}
		}
		free(needle_c);
		free(replacement_c);
		if (replacements) {
			render_status_message("replaced %d instance%s of %s", replacements, replacements == 1 ? "" : "s", needle);
			set_history_break();
			redraw_text();
		} else {
			render_error("Pattern not found: %s", needle);
		}
	} else if (!strcmp(argv[0], "tabnew")) {
		if (argc > 1) {
			open_file(argv[1]);
			update_title();
		} else {
			env = buffer_new();
			setup_buffer(env);
			redraw_all();
			update_title();
		}
	} else if (!strcmp(argv[0], "w")) {
		/* w: write file */
		if (argc > 1) {
			write_file(argv[1]);
		} else {
			write_file(env->file_name);
		}
	} else if (!strcmp(argv[0], "history")) {
		render_commandline_message(""); /* To clear command line */
		for (int i = COMMAND_HISTORY_MAX; i > 1; --i) {
			if (command_history[i-1]) render_commandline_message("%d:%s\n", i-1, command_history[i-1]);
		}
		render_commandline_message("\n");
		redraw_tabbar();
		redraw_commandline();
		int c;
		while ((c = bim_getch())== -1);
		bim_unget(c);
		redraw_all();
	} else if (!strcmp(argv[0], "wq")) {
		/* wq: write file and close buffer; if there's no file to write to, may do weird things */
		write_file(env->file_name);
		close_buffer();
	} else if (!strcmp(argv[0], "q")) {
		/* close buffer if unmodified */
		if (left_buffer && left_buffer == right_buffer) {
			unsplit();
			return;
		}
		if (env->modified) {
			render_error("No write since last change. Use :q! to force exit.");
		} else {
			close_buffer();
		}
		update_title();
	} else if (!strcmp(argv[0], "q!")) {
		/* close buffer without warning if unmodified */
		close_buffer();
		update_title();
	} else if (!strcmp(argv[0], "qa") || !strcmp(argv[0], "qall")) {
		/* Close all */
		try_quit();
	} else if (!strcmp(argv[0], "qa!")) {
		/* Forcefully exit editor */
		while (buffers_len) {
			buffer_close(buffers[0]);
		}
		quit(NULL);
	} else if (!strcmp(argv[0], "tabp")) {
		/* Next tab */
		previous_tab();
		update_title();
	} else if (!strcmp(argv[0], "tabn")) {
		/* Previous tab */
		next_tab();
		update_title();
	} else if (!strcmp(argv[0], "git")) {
		if (argc < 2) {
			render_status_message("git=%d", env->checkgitstatusonwrite);
		} else {
			env->checkgitstatusonwrite = !!atoi(argv[1]);
			if (env->checkgitstatusonwrite && !env->modified && env->file_name) {
				git_examine(env->file_name);
				redraw_text();
			}
		}
	} else if (!strcmp(argv[0], "colorgutter")) {
		if (argc < 2) {
			render_status_message("colorgutter=%d", global_config.color_gutter);
		} else {
			global_config.color_gutter = !!atoi(argv[1]);
			redraw_text();
		}
	} else if (!strcmp(argv[0], "indent")) {
		env->indent = 1;
		redraw_statusbar();
	} else if (!strcmp(argv[0], "noindent")) {
		env->indent = 0;
		redraw_statusbar();
	} else if (!strcmp(argv[0], "cursorcolumn")) {
		render_status_message("cursorcolumn=%d", env->preferred_column);
	} else if (!strcmp(argv[0], "noh")) {
		if (global_config.search) {
			free(global_config.search);
			global_config.search = NULL;
			for (int i = 0; i < env->line_count; ++i) {
				recalculate_syntax(env->lines[i],i);
			}
			redraw_text();
		}
	} else if (!strcmp(argv[0], "help")) {
		/*
		 * The repeated calls to redraw_commandline here make use
		 * of scrolling to draw this multiline help message on
		 * the same background as the command line.
		 */
		render_commandline_message(""); /* To clear command line */
		render_commandline_message("\n");
		#ifdef __toaru__
		render_commandline_message(" \033[1mbim - The standard ToaruOS Text Editor\033[22m\n");
		#else
		render_commandline_message(" \033[1mbim - a text editor \033[22m\n");
		#endif
		render_commandline_message("\n");
		render_commandline_message(" Available commands:\n");
		render_commandline_message("   Quit with \033[3m:q\033[23m, \033[3m:qa\033[23m, \033[3m:q!\033[23m, \033[3m:qa!\033[23m\n");
		render_commandline_message("   Write out with \033[3m:w \033[4mfile\033[24;23m\n");
		render_commandline_message("   Set syntax with \033[3m:syntax \033[4mlanguage\033[24;23m\n");
		render_commandline_message("   Open a new tab with \033[3m:e \033[4mpath/to/file\033[24;23m\n");
		render_commandline_message("   \033[3m:tabn\033[23m and \033[3m:tabp\033[23m can be used to switch tabs\n");
		render_commandline_message("   Set the color scheme with \033[3m:theme \033[4mtheme\033[24;23m\n");
		render_commandline_message("   Set the behavior of the tab key with \033[3m:tabs\033[23m or \033[3m:spaces\033[23m\n");
		render_commandline_message("   Set tabstop with \033[3m:tabstop \033[4mwidth\033[24;23m\n");
		render_commandline_message("\n");
		render_commandline_message(" %s\n", BIM_COPYRIGHT);
		render_commandline_message("\n");
		/* Redrawing the tabbar makes it look like we just shifted the whole view up */
		redraw_tabbar();
		redraw_commandline();
		/* Wait for a character so we can redraw the screen before continuing */
		int c;
		while ((c = bim_getch())== -1);
		/* Make sure that key press actually gets used */
		bim_unget(c);
		/*
		 * Redraw everything to hide the help message and get the
		 * upper few lines of text on screen again
		 */
		redraw_all();
	} else if (!strcmp(argv[0], "theme") || !strcmp(argv[0],"colorscheme")) {
		if (argc < 2) {
			render_status_message("theme=%s", current_theme);
			return;
		}
		for (struct theme_def * d = themes; d->name; ++d) {
			if (!strcmp(argv[1], d->name)) {
				d->load();
				redraw_all();
				return;
			}
		}
	} else if (!strcmp(argv[0], "splitpercent")) {
		if (argc < 2) {
			render_status_message("splitpercent=%d", global_config.split_percent);
			return;
		} else {
			global_config.split_percent = atoi(argv[1]);
			if (left_buffer) {
				update_split_size();
				redraw_all();
			}
		}
	} else if (!strcmp(argv[0], "split")) {
		/* Force split the current buffer; will become unsplit under certain circumstances */
		buffer_t * original = env;
		if (argc > 1) {
			int is_not_number = 0;
			for (char * c = argv[1]; *c; ++c) is_not_number |= !isdigit(*c);
			if (is_not_number) {
				open_file(argv[1]);
				right_buffer = buffers[buffers_len-1];
			} else {
				int other = atoi(argv[1]);
				if (other >= buffers_len || other < 0) {
					render_error("Invalid buffer number: %d", other);
					return;
				}
				right_buffer = buffers[other];
			}
		} else {
			right_buffer = original;
		}
		left_buffer = original;
		update_split_size();
		redraw_all();
	} else if (!strcmp(argv[0], "unsplit")) {
		unsplit();
	} else if (!strcmp(argv[0], "syntax")) {
		if (argc < 2) {
			render_status_message("syntax=%s", env->syntax ? env->syntax->name : "none");
			return;
		}
		set_syntax_by_name(argv[1]);
	} else if (!strcmp(argv[0], "recalc")) {
		for (int i = 0; i < env->line_count; ++i) {
			env->lines[i]->istate = 0;
		}
		env->loading = 1;
		for (int i = 0; i < env->line_count; ++i) {
			recalculate_syntax(env->lines[i],i);
		}
		env->loading = 0;
		redraw_all();
	} else if (!strcmp(argv[0], "tabs")) {
		env->tabs = 1;
		redraw_statusbar();
	} else if (!strcmp(argv[0], "spaces")) {
		env->tabs = 0;
		redraw_statusbar();
	} else if (!strcmp(argv[0], "tabstop")) {
		if (argc < 2) {
			render_status_message("tabstop=%d", env->tabstop);
		} else {
			int t = atoi(argv[1]);
			if (t > 0 && t < 32) {
				env->tabstop = t;
				for (int i = 0; i < env->line_count; ++i) {
					recalculate_tabs(env->lines[i]);
				}
				redraw_all();
			} else {
				render_error("Invalid tabstop: %s", argv[1]);
			}
		}
	} else if (!strcmp(argv[0], "clearyank")) {
		if (global_config.yanks) {
			for (unsigned int i = 0; i < global_config.yank_count; ++i) {
				free(global_config.yanks[i]);
			}
			free(global_config.yanks);
			global_config.yanks = NULL;
			global_config.yank_count = 0;
			redraw_statusbar();
		}
	} else if (!strcmp(argv[0], "padding")) {
		if (argc < 2) {
			render_status_message("padding=%d", global_config.cursor_padding);
		} else {
			global_config.cursor_padding = atoi(argv[1]);
			place_cursor_actual();
		}
	} else if (!strcmp(argv[0], "smartcase")) {
		if (argc < 2) {
			render_status_message("smartcase=%d", global_config.smart_case);
		} else {
			global_config.smart_case = atoi(argv[1]);
			place_cursor_actual();
		}
	} else if (!strcmp(argv[0], "hlparen")) {
		if (argc < 2) {
			render_status_message("hlparen=%d", global_config.highlight_parens);
		} else {
			global_config.highlight_parens = atoi(argv[1]);
			for (int i = 0; i < env->line_count; ++i) {
				recalculate_syntax(env->lines[i],i);
			}
			redraw_text();
			place_cursor_actual();
		}
	} else if (!strcmp(argv[0], "hlcurrent")) {
		if (argc < 2) {
			render_status_message("hlcurrent=%d", global_config.highlight_current_line);
		} else {
			global_config.highlight_current_line = atoi(argv[1]);
			if (!global_config.highlight_current_line) {
				for (int i = 0; i < env->line_count; ++i) {
					env->lines[i]->is_current = 0;
				}
			}
			redraw_text();
			place_cursor_actual();
		}
	} else if (!strcmp(argv[0], "relativenumber")) {
		if (argc < 2) {
			render_status_message("relativenumber=%d", global_config.relative_lines);
		} else {
			global_config.relative_lines = atoi(argv[1]);
			if (!global_config.relative_lines) {
				for (int i = 0; i < env->line_count; ++i) {
					env->lines[i]->is_current = 0;
				}
			}
			redraw_text();
			place_cursor_actual();
		}
	} else if (!strcmp(argv[0],"TOhtml") || !strcmp(argv[0],"tohtml")) { /* TOhtml is for vim compatibility */
		convert_to_html();
	} else if (!strcmp(argv[0],"buffers")) {
		for (int i = 0; i < buffers_len; ++i) {
			render_commandline_message("%d: %s\n", i, buffers[i]->file_name ? buffers[i]->file_name : "(no name)");
		}
		redraw_tabbar();
		redraw_commandline();
		int c;
		while ((c = bim_getch())== -1);
		bim_unget(c);
		redraw_all();
	} else if (argv[0][0] == '-' && isdigit(argv[0][1])) {
		global_config.break_from_selection = 1;
		goto_line(env->line_no-atoi(&argv[0][1]));
	} else if (argv[0][0] == '+' && isdigit(argv[0][1])) {
		global_config.break_from_selection = 1;
		goto_line(env->line_no+atoi(&argv[0][1]));
	} else if (isdigit(*argv[0])) {
		/* Go to line number */
		global_config.break_from_selection = 1;
		goto_line(atoi(argv[0]));
	} else {
		/* Unrecognized command */
		render_error("Not an editor command: %s", argv[0]);
	}
}

/**
 * Tab completion for command mode.
 */
void command_tab_complete(char * buffer) {
	/* Figure out which argument this is and where it starts */
	int arg = 0;
	char * buf = strdup(buffer);
	char * b = buf;

	char * args[32];

	int candidate_count= 0;
	int candidate_space = 4;
	char ** candidates = malloc(sizeof(char*)*candidate_space);

	/* Accept whitespace before first argument */
	while (*b == ' ') b++;
	char * start = b;
	args[0] = start;
	while (*b && *b != ' ') b++;
	while (*b) {
		while (*b == ' ') {
			*b = '\0';
			b++;
		}
		start = b;
		arg++;
		if (arg < 32) {
			args[arg] = start;
		}
		while (*b && *b != ' ') b++;
	}

	/**
	 * Check a possible candidate and add it to the
	 * candidates list, expanding as necessary,
	 * if it matches for the current argument.
	 */
#define add_candidate(candidate) \
	do { \
		char * _arg = args[arg]; \
		int r = strncmp(_arg, candidate, strlen(_arg)); \
		if (!r) { \
			if (candidate_count == candidate_space) { \
				candidate_space *= 2; \
				candidates = realloc(candidates,sizeof(char *) * candidate_space); \
			} \
			candidates[candidate_count] = strdup(candidate); \
			candidate_count++; \
		} \
	} while (0)

	if (arg == 0) {
		/* Complete command names */
		add_candidate("help");
		add_candidate("recalc");
		add_candidate("syntax");
		add_candidate("tabn");
		add_candidate("tabp");
		add_candidate("tabnew");
		add_candidate("theme");
		add_candidate("colorscheme");
		add_candidate("tabs");
		add_candidate("tabstop");
		add_candidate("spaces");
		add_candidate("noh");
		add_candidate("clearyank");
		add_candidate("indent");
		add_candidate("noindent");
		add_candidate("padding");
		add_candidate("hlparen");
		add_candidate("hlcurrent");
		add_candidate("relativenumber");
		add_candidate("cursorcolumn");
		add_candidate("smartcase");
		add_candidate("split");
		add_candidate("splitpercent");
		add_candidate("unsplit");
		add_candidate("git");
		add_candidate("colorgutter");
		add_candidate("tohtml");
		add_candidate("buffers");
		add_candidate("s/");
		goto _accept_candidate;
	}

	if (arg == 1 && !strcmp(args[0], "syntax")) {
		/* Complete syntax options */
		add_candidate("none");
		for (struct syntax_definition * s = syntaxes; s->name; ++s) {
			add_candidate(s->name);
		}
		goto _accept_candidate;
	}

	if (arg == 1 && (!strcmp(args[0], "theme") || !strcmp(args[0], "colorscheme"))) {
		/* Complete color theme names */
		for (struct theme_def * s = themes; s->name; ++s) {
			add_candidate(s->name);
		}
		goto _accept_candidate;
	}

	if (arg == 1 && (!strcmp(args[0], "e") || !strcmp(args[0], "tabnew") || !strcmp(args[0],"split") || !strcmp(args[0],"w"))) {
		/* Complete file paths */

		/* First, find the deepest directory match */
		char * tmp = strdup(args[arg]);
		char * last_slash = strrchr(tmp, '/');
		DIR * dirp;
		if (last_slash) {
			*last_slash = '\0';
			if (last_slash == tmp) {
				/* Started with slash, and it was the only slash */
				dirp = opendir("/");
			} else {
				dirp = opendir(tmp);
			}
		} else {
			/* No directory match, completing from current directory */
			dirp = opendir(".");
			tmp[0] = '\0';
		}

		if (!dirp) {
			/* Directory match doesn't exist, no candidates to populate */
			free(tmp);
			goto done;
		}

		struct dirent * ent = readdir(dirp);
		while (ent != NULL) {
			if (ent->d_name[0] != '.' || (last_slash ? (last_slash[1] == '.') : (tmp[0] == '.'))) {
				struct stat statbuf;
				/* Figure out if this file is a directory */
				if (last_slash) {
					char * x = malloc(strlen(tmp) + 1 + strlen(ent->d_name) + 1);
					snprintf(x, strlen(tmp) + 1 + strlen(ent->d_name) + 1, "%s/%s",tmp,ent->d_name);
					stat(x, &statbuf);
					free(x);
				} else {
					stat(ent->d_name, &statbuf);
				}

				/* Build the complete argument name to tab complete */
				char s[1024] = {0};
				if (last_slash == tmp) {
					strcat(s,"/");
				} else if (*tmp) {
					strcat(s,tmp);
					strcat(s,"/");
				}
				strcat(s,ent->d_name);
				/*
				 * If it is a directory, add a / to the end so the next completion
				 * attempt will complete the directory's contents.
				 */
				if (S_ISDIR(statbuf.st_mode)) {
					strcat(s,"/");
				}
				add_candidate(s);
			}
			ent = readdir(dirp);
		}
		closedir(dirp);
		free(tmp);
		goto _accept_candidate;
	}

_accept_candidate:
	if (candidate_count == 0) {
		redraw_statusbar();
		goto done;
	}

	if (candidate_count == 1) {
		/* Only one completion possibility */
		redraw_statusbar();

		/* Fill out the rest of the command */
		char * cstart = (buffer) + (start - buf);
		for (unsigned int i = 0; i < strlen(candidates[0]); ++i) {
			*cstart = candidates[0][i];
			cstart++;
		}
		*cstart = '\0';
	} else {
		/* Print candidates in status bar */
		char * tmp = malloc(global_config.term_width+1);
		memset(tmp, 0, global_config.term_width+1);
		int offset = 0;
		for (int i = 0; i < candidate_count; ++i) {
			if (offset + 1 + (signed)strlen(candidates[i]) > global_config.term_width - 5) {
				strcat(tmp, "...");
				break;
			}
			if (offset > 0) {
				strcat(tmp, " ");
				offset++;
			}
			strcat(tmp, candidates[i]);
			offset += strlen(candidates[i]);
		}
		render_status_message("%s", tmp);
		free(tmp);

		/* Complete to longest common substring */
		char * cstart = (buffer) + (start - buf);
		for (int i = 0; i < 1023 /* max length of command */; i++) {
			for (int j = 1; j < candidate_count; ++j) {
				if (candidates[0][i] != candidates[j][i]) goto _reject;
			}
			*cstart = candidates[0][i];
			cstart++;
		}
		/* End of longest common substring */
_reject:
		*cstart = '\0';
		/* Just make sure the buffer doesn't end on an incomplete multibyte sequence */
		if (start > buf) { /* Start point needs to be something other than first byte */
			char * tmp = cstart - 1;
			if ((*tmp & 0xC0) == 0x80) {
				/* Count back until we find the start byte and make sure we have the right number */
				int count = 1;
				int x = 0;
				while (tmp >= start) {
					x++;
					tmp--;
					if ((*tmp & 0xC0) == 0x80) {
						count++;
					} else if ((*tmp & 0xC0) == 0xC0) {
						/* How many should we have? */
						int i = 1;
						int j = *tmp;
						while (j & 0x20) {
							i += 1;
							j <<= 1;
						}
						if (count != i) {
							*tmp = '\0';
							break;
						}
						break;
					} else {
						/* This isn't right, we had a bad multibyte sequence? Or someone is typing Latin-1. */
						tmp++;
						*tmp = '\0';
						break;
					}
				}
			} else if ((*tmp & 0xC0) == 0xC0) {
				*tmp = '\0';
			}
		}
	}

	/* Free candidates */
	for (int i = 0; i < candidate_count; ++i) {
		free(candidates[i]);
	}

	/* Redraw command line */
done:
	redraw_commandline();
	printf(":%s", buffer);

	free(candidates);
	free(buf);
}

/**
 * Handle complex keyboard escapes when taking in a command.
 * This allows us to not muck up the command input and also
 * handle things like up/down arrow keys to go through history.
 */
int handle_command_escape(int * this_buf, int * timeout, int c) {
	if (*timeout >=  1 && this_buf[*timeout-1] == '\033' && c == '\033') {
		this_buf[*timeout] = c;
		(*timeout)++;
		return 1;
	}
	if (*timeout >= 1 && this_buf[*timeout-1] == '\033' && c != '[') {
		*timeout = 0;
		bim_unget(c);
		return 1;
	}
	if (*timeout >= 1 && this_buf[*timeout-1] == '\033' && c == '[') {
		*timeout = 1;
		this_buf[*timeout] = c;
		(*timeout)++;
		return 0;
	}
	if (*timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[' &&
			(isdigit(c) || c == ';')) {
		this_buf[*timeout] = c;
		(*timeout)++;
		return 0;
	}
	if (*timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[') {
		int out = 0;
		switch (c) {
			case 'M':
				out = 2;
				break;

			case 'A': // up
			case 'B': // down
			case 'C': // right
			case 'D': // left
			case 'H': // home
			case 'F': // end
				out = c;
				break;
		}
		*timeout = 0;
		return out;
	}

	*timeout = 0;
	return 0;

}


/**
 * Command mode
 *
 * Accept a user command and then process it and
 * return to normal mode.
 *
 * TODO: We only have basic line editing here; it might be
 *       nice to add more advanced line editing, like cursor
 *       movement, tab completion, etc. This is easier than
 *       with the shell since we have a lot more control over
 *       where the command input bar is rendered.
 */
void command_mode(void) {
	int c;
	char buffer[1024] = {0};
	int  buffer_len = 0;
	int _redraw_on_byte = 0;

	int timeout = 0;
	int this_buf[20];

	redraw_commandline();
	printf(":");
	show_cursor();

	int history_point = -1;

	while ((c = bim_getch())) {
		if (c == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				return;
			}
			timeout = 0;
			continue;
		} else {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						/* Escape, cancel command */
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case 3: /* ^C */
						return;
					case ENTER_KEY:
					case LINE_FEED:
						/* Enter, run command */
						process_command(buffer);
						return;
					case '\t':
						/* Handle tab completion */
						command_tab_complete(buffer);
						buffer_len = strlen(buffer);
						break;
					case BACKSPACE_KEY:
					case DELETE_KEY:
						/* Backspace, delete last character in command buffer */
						if (buffer_len > 0) {
							do {
								buffer_len--;
								if ((buffer[buffer_len] & 0xC0) != 0x80) {
									buffer[buffer_len] = '\0';
									break;
								}
							} while (1);
							goto _redraw_buffer;
						} else {
							/* If backspaced through entire command, cancel command mode */
							redraw_commandline();
							return;
						}
						break;
					case 23:
						while (buffer_len >0 &&
								(buffer[buffer_len-1] == ' ' ||
								 buffer[buffer_len-1] == '/')) {
							buffer_len--;
							buffer[buffer_len] = '\0';
						}
						while (buffer_len > 0 &&
								buffer[buffer_len-1] != ' ' &&
								buffer[buffer_len-1] != '/') {
							buffer_len--;
							buffer[buffer_len] = '\0';
						}
						goto _redraw_buffer;
					default:
						/* Regular character */
						buffer[buffer_len] = c;
						buffer_len++;
						if ((c & 0xC0) == 0xC0) {
							/* This is the start of a UTF-8 character; how many bytes do we need? */
							int i = 1;
							int j = c;
							while (j & 0x20) {
								i += 1;
								j <<= 1;
							}
							_redraw_on_byte = i;
						} else if ((c & 0xC0) == 0x80 && _redraw_on_byte) {
							_redraw_on_byte--;
							if (_redraw_on_byte == 0) {
								goto _redraw_buffer;
							}
						} else {
							printf("%c", c);
						}
						break;
				}
			} else {
				switch (handle_command_escape(this_buf,&timeout,c)) {
					case 1:
						bim_unget(c);
						return;
					case 'A':
						/* Load from history */
						if (command_history[history_point+1]) {
							memcpy(buffer, command_history[history_point+1], strlen(command_history[history_point+1])+1);
							history_point++;
							buffer_len = strlen(buffer);
						}
						goto _redraw_buffer;
					case 'B':
						if (history_point > 0) {
							history_point--;
							memcpy(buffer, command_history[history_point], strlen(command_history[history_point])+1);
						} else {
							history_point = -1;
							memset(buffer, 0, 1000);
						}
						buffer_len = strlen(buffer);
						goto _redraw_buffer;
					case 'C':
					case 'D':
					case 'H':
					case 'F':
						render_status_message("line editing not supported in command mode (sorry)");
						goto _redraw_buffer;
				}
			}

			show_cursor();
			continue;

_redraw_buffer:
			redraw_commandline();
			printf(":%s", buffer);
			show_cursor();
		}
	}
}

/**
 * Handle shared escape keys (mostly navigation)
 */
int handle_escape(int * this_buf, int * timeout, int c) {
	if (*timeout >=  1 && this_buf[*timeout-1] == '\033' && c == '\033') {
		this_buf[*timeout] = c;
		(*timeout)++;
		return 1;
	}
	if (*timeout >= 1 && this_buf[*timeout-1] == '\033' && c != '[') {
		*timeout = 0;
		bim_unget(c);
		return 1;
	}
	if (*timeout >= 1 && this_buf[*timeout-1] == '\033' && c == '[') {
		*timeout = 1;
		this_buf[*timeout] = c;
		(*timeout)++;
		return 0;
	}
	if (*timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[' &&
			(isdigit(c) || c == ';')) {
		this_buf[*timeout] = c;
		(*timeout)++;
		return 0;
	}
	if (*timeout >= 2 && this_buf[0] == '\033' && this_buf[1] == '[') {
		switch (c) {
			case 'M':
				handle_mouse();
				break;
			case 'A': // up
				cursor_up();
				break;
			case 'B': // down
				cursor_down();
				break;
			case 'C': // right
				if (this_buf[*timeout-1] == '5') { // ctrl
					big_word_right();
				} else if (this_buf[*timeout-1] == '2') { //shift
					word_right();
				} else if (this_buf[*timeout-1] == '3') { //alt
					global_config.split_percent += 1;
					update_split_size();
					redraw_all();
				} else if (this_buf[*timeout-1] == '4') { // alt shift
					use_right_buffer();
					redraw_all();
				} else {
					cursor_right();
				}
				break;
			case 'D': // left
				if (this_buf[*timeout-1] == '5') { // ctrl
					big_word_left();
				} else if (this_buf[*timeout-1] == '2') { // shift
					word_left();
				} else if (this_buf[*timeout-1] == '3') { // alt
					global_config.split_percent -= 1;
					update_split_size();
					redraw_all();
				} else if (this_buf[*timeout-1] == '4') { // alt-shift
					use_left_buffer();
					redraw_all();
				} else {
					cursor_left();
				}
				break;
			case 'H': // home
				cursor_home();
				break;
			case 'F': // end
				cursor_end();
				break;
			case 'I':
				goto_line(env->line_no - (global_config.term_height - 6));
				break;
			case 'G':
				goto_line(env->line_no + global_config.term_height - 6);
				break;
			case 'Z':
				/* Shift tab */
				if (env->mode == MODE_LINE_SELECTION) {
					*timeout = 0;
					return 'Z';
				}
				break;
			case '~':
				switch (this_buf[*timeout-1]) {
					case '1':
						cursor_home();
						break;
					case '3':
						if (env->mode == MODE_INSERT || env->mode == MODE_REPLACE) {
							if (env->col_no < env->lines[env->line_no - 1]->actual + 1) {
								line_delete(env->lines[env->line_no - 1], env->col_no, env->line_no - 1);
								redraw_line(env->line_no - env->offset - 1, env->line_no-1);
								set_modified();
								redraw_statusbar();
								place_cursor_actual();
							} else if (env->line_no < env->line_count) {
								merge_lines(env->lines, env->line_no);
								redraw_text();
								set_modified();
								redraw_statusbar();
								place_cursor_actual();
							}
						}
						break;
					case '4':
						cursor_end();
						break;
					case '5':
						goto_line(env->line_no - (global_config.term_height - 6));
						break;
					case '6':
						goto_line(env->line_no + global_config.term_height - 6);
						break;
				}
				break;
			default:
				render_error("Unrecognized escape sequence identifier: %c", c);
				break;
		}
		*timeout = 0;
		return 0;
	}

	*timeout = 0;
	return 0;
}

/**
 * Search for the word under the cursor
 */
void search_under_cursor(void) {
	/* Figure out size */
	int c_before = 0;
	int c_after = 0;
	int i = env->col_no;
	while (i > 0) {
		if (!c_keyword_qualifier(env->lines[env->line_no-1]->text[i-1].codepoint)) break;
		c_before++;
		i--;
	}
	i = env->col_no+1;
	while (i < env->lines[env->line_no-1]->actual+1) {
		if (!c_keyword_qualifier(env->lines[env->line_no-1]->text[i-1].codepoint)) break;
		c_after++;
		i++;
	}
	if (!c_before && !c_after) return;

	/* Populate with characters */
	if (global_config.search) free(global_config.search);
	global_config.search = malloc(sizeof(uint32_t) * (c_before+c_after+1));
	int j = 0;
	while (c_before) {
		global_config.search[j] = env->lines[env->line_no-1]->text[env->col_no-c_before].codepoint;
		c_before--;
		j++;
	}
	int x = 0;
	while (c_after) {
		global_config.search[j] = env->lines[env->line_no-1]->text[env->col_no+x].codepoint;
		j++;
		x++;
		c_after--;
	}
	global_config.search[j] = 0;

	/* Find it */
	search_next();
}

/**
 * Handler for f,F,t,T
 * Find the selected character based on the search requirement:
 * f: forward, stop on character
 * F: backward, stop on character
 * t: forward, stop before character
 * T: backward, stop after character
 */
void find_character(int type, int c) {
	if (type == 'f' || type == 't') {
		for (int i = env->col_no+1; i <= env->lines[env->line_no-1]->actual; ++i) {
			if (env->lines[env->line_no-1]->text[i-1].codepoint == c) {
				env->col_no = i - !!(type == 't');
				place_cursor_actual();
				set_preferred_column();
				return;
			}
		}
	} else if (type == 'F' || type == 'T') {
		for (int i = env->col_no; i >= 1; --i) {
			if (env->lines[env->line_no-1]->text[i-1].codepoint == c) {
				env->col_no = i + !!(type == 'T');
				place_cursor_actual();
				set_preferred_column();
				return;
			}
		}
	}
}

/**
 * Clear the navigation number buffer
 */
void reset_nav_buffer(int c) {
	if (nav_buffer && (c < '0' || c > '9')) {
		nav_buffer = 0;
		redraw_commandline();
	}
}

/**
 * Performs action with repitions if nav_buffer is set;
 * otherwise once. With reps, set loading so that actions
 * don't redraw screen several times.
 */
#define with_reps(stuff) \
	if (reps) { \
		env->loading = 1; \
		for (int i = 0; i < reps; ++i) { \
			stuff; \
		} \
		env->loading = 0; \
		redraw_all(); \
	} else { \
		stuff; \
	}

/**
 * Standard navigation shared by normal, line, and char selection.
 */
void handle_navigation(int c) {
	int reps = (nav_buffer) ? atoi(nav_buf) : 1;
	switch (c) {
		case 2: /* ctrl-b = page up */
			with_reps(goto_line(env->line_no - (global_config.term_height - 6)));
			break;
		case 6: /* ctrl-f = page down */
			with_reps(goto_line(env->line_no + global_config.term_height - 6));
			break;
		case ':': /* Switch to command mode */
			command_mode();
			break;
		case '/': /* Switch to search mode */
			search_mode(1);
			break;
		case '?': /* Switch to search mode */
			search_mode(0);
			break;
		case 'n': /* Jump to next search result */
			with_reps(search_next());
			break;
		case 'N': /* Jump backwards to previous search result */
			with_reps(search_prev());
			break;
		case 'j': /* Move cursor down */
			with_reps(cursor_down());
			break;
		case 'k': /* Move cursor up */
			with_reps(cursor_up());
			break;
		case 'h': /* Move cursor left */
			with_reps(cursor_left());
			break;
		case 'l': /* Move cursor right*/
			with_reps(cursor_right());
			break;
		case 'b': /* Move cursor one word left */
			with_reps(word_left());
			break;
		case 'w': /* Move cursor one word right */
			with_reps(word_right());
			break;
		case 'B': /* Move cursor one WORD left */
			with_reps(big_word_left());
			break;
		case 'W': /* Move cursor one WORD right */
			with_reps(big_word_right());
			break;
		case 'f': /* Find character forward */
		case 'F': /* ... backward */
		case 't': /* ... forward but stop before */
		case 'T': /* ... backwards but stop before */
			{
				char tmp[2] = {c,'\0'};
				int cin = read_one_character(tmp);
				if (cin != -1) {
					with_reps(find_character(c, cin));
				}
				redraw_commandline();
			}
			break;
		case 'G': /* Go to line or end of file */
			if (nav_buffer) {
				goto_line(atoi(nav_buf));
				reset_nav_buffer(0);
			} else {
				goto_line(env->line_count);
			}
			break;
		case '*': /* Search for word under cursor */
			search_under_cursor();
			break;
		case ' ': /* Jump forward several lines */
			with_reps(goto_line(env->line_no + global_config.term_height - 6));
			break;
		case '%': /* Jump to matching brace/bracket */
			if (env->mode == MODE_LINE_SELECTION || env->mode == MODE_CHAR_SELECTION) {
				/* These modes need to recalculate syntax as find_matching_brace uses it to find appropriate match */
				for (int i = 0; i < env->line_count; ++i) {
					recalculate_syntax(env->lines[i],i);
				}
			}
			{
				int paren_line = -1, paren_col = -1;
				find_matching_paren(&paren_line, &paren_col, 1);
				if (paren_line != -1) {
					env->line_no = paren_line;
					env->col_no = paren_col;
					set_preferred_column();
					place_cursor_actual();
					redraw_statusbar();
				}
			}
			break;
		case '{': /* Jump to previous blank line */
			env->col_no = 1;
			if (env->line_no == 1) break;
			do {
				env->line_no--;
				if (env->lines[env->line_no-1]->actual == 0) break;
			} while (env->line_no > 1);
			set_preferred_column();
			redraw_statusbar();
			break;
		case '}': /* Jump to next blank line */
			env->col_no = 1;
			if (env->line_no == env->line_count) break;
			do {
				env->line_no++;
				if (env->lines[env->line_no-1]->actual == 0) break;
			} while (env->line_no < env->line_count);
			set_preferred_column();
			redraw_statusbar();
			break;
		case '$': /* Move cursor to end of line */
			cursor_end();
			break;
		case '|':
		case '0': /* Move cursor to beginning of line */
			if (c == '0' && nav_buffer) break;
			cursor_home();
			break;
		case '\r': /* first non-whitespace of next line */
			if (env->line_no < env->line_count) {
				env->line_no++;
				env->col_no = 1;
			} else {
				break;
			}
			/* fall through */
		case '^': /* first non-whitespace */
			for (int i = 0; i < env->lines[env->line_no-1]->actual; ++i) {
				if (!is_whitespace(env->lines[env->line_no-1]->text[i].codepoint)) {
					env->col_no = i + 1;
					break;
				}
			}
			set_preferred_column();
			redraw_statusbar();
			break;
	}

	/* Otherwise, numbers go into the number buffer */
	if ((c >= '1' && c <= '9') || (c == '0' && nav_buffer)) {
		if (nav_buffer < NAV_BUFFER_MAX) {
			/* Up to NAV_BUFFER_MAX=10 characters; that should be enough for most tasks */
			nav_buf[nav_buffer] = c;
			nav_buf[nav_buffer+1] = 0;
			nav_buffer++;
			/* Print the number buffer */
			redraw_commandline();
		}
	}
}

/**
 * Macro for redrawing selected lines with appropriate highlighting.
 */
#define _redraw_line(line, force_start_line) \
	do { \
		if (!(force_start_line) && (line) == env->start_line) break; \
		if ((line) > env->line_count + 1) { \
			if ((line) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 1) { \
				draw_excess_line((line) - env->offset - 1); \
			} \
			break; \
		} \
		if ((env->line_no < env->start_line  && ((line) < env->line_no || (line) > env->start_line)) || \
			(env->line_no > env->start_line  && ((line) > env->line_no || (line) < env->start_line)) || \
			(env->line_no == env->start_line && (line) != env->start_line)) { \
			recalculate_syntax(env->lines[(line)-1],(line)-1); \
		} else { \
			for (int j = 0; j < env->lines[(line)-1]->actual; ++j) { \
				env->lines[(line)-1]->text[j].flags |= FLAG_SELECT; \
			} \
		} \
		if ((line) - env->offset + 1 > 1 && \
			(line) - env->offset - 1< global_config.term_height - global_config.bottom_size - 1) { \
			redraw_line((line) - env->offset - 1, (line)-1); \
		} \
	} while (0)

/**
 * Adjust indentation on selected lines.
 */
void adjust_indent(int start_line, int direction) {
	int lines_to_cover = 0;
	int start_point = 0;
	if (start_line <= env->line_no) {
		start_point = start_line - 1;
		lines_to_cover = env->line_no - start_line + 1;
	} else {
		start_point = env->line_no - 1;
		lines_to_cover = start_line - env->line_no + 1;
	}
	for (int i = 0; i < lines_to_cover; ++i) {
		if (env->lines[start_point + i]->actual < 1) continue;
		if (direction == -1) {
			if (env->tabs) {
				if (env->lines[start_point + i]->text[0].codepoint == '\t') {
					line_delete(env->lines[start_point + i],1,start_point+i);
					_redraw_line(start_point+i+1,1);
				}
			} else {
				for (int j = 0; j < env->tabstop; ++j) {
					if (env->lines[start_point + i]->text[0].codepoint == ' ') {
						line_delete(env->lines[start_point + i],1,start_point+i);
					}
				}
				_redraw_line(start_point+i+1,1);
			}
		} else if (direction == 1) {
			if (env->tabs) {
				char_t c;
				c.codepoint = '\t';
				c.display_width = env->tabstop;
				c.flags = FLAG_SELECT;
				env->lines[start_point + i] = line_insert(env->lines[start_point + i], c, 0, start_point + i);
			} else {
				for (int j = 0; j < env->tabstop; ++j) {
					char_t c;
					c.codepoint = ' ';
					c.display_width = 1;
					c.flags = FLAG_SELECT;
					env->lines[start_point + i] = line_insert(env->lines[start_point + i], c, 0, start_point + i);
				}
			}
			_redraw_line(start_point+i+1,1);
		}
	}
	if (env->col_no > env->lines[env->line_no-1]->actual) {
		env->col_no = env->lines[env->line_no-1]->actual;
	}
	set_preferred_column();
	set_modified();
}

/**
 * LINE SELECTION mode
 *
 * Equivalent to visual line in vim; selects lines of texts.
 */
void line_selection_mode(void) {
	env->start_line = env->line_no;
	int prev_line  = env->start_line;

	env->mode = MODE_LINE_SELECTION;
	redraw_commandline();

	int c;
	int timeout = 0;
	int this_buf[20];

	for (int j = 0; j < env->lines[env->line_no-1]->actual; ++j) {
		env->lines[env->line_no-1]->text[j].flags |= FLAG_SELECT;
	}
	redraw_line(env->line_no - env->offset - 1, env->line_no-1);

	while ((c = bim_getch())) {
		if (c == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				goto _leave_select_line;
			}
			timeout = 0;
			continue;
		} else {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						cursor_left();
						break;
					case '\t':
						if (env->readonly) goto _readonly;
						adjust_indent(env->start_line, 1);
						break;
					case 'V':
						goto _leave_select_line;
					case 'y':
						yank_lines(env->start_line, env->line_no);
						goto _leave_select_line;
					case 'D':
					case 'd':
					case 'x':
					case 's':
						if (env->readonly) goto _readonly;
						yank_lines(env->start_line, env->line_no);
						if (env->start_line <= env->line_no) {
							int lines_to_delete = env->line_no - env->start_line + 1;
							for (int i = 0; i < lines_to_delete; ++i) {
								env->lines = remove_line(env->lines, env->start_line-1);
							}
							env->line_no = env->start_line;
						} else {
							int lines_to_delete = env->start_line - env->line_no + 1;
							for (int i = 0; i < lines_to_delete; ++i) {
								env->lines = remove_line(env->lines, env->line_no-1);
							}
						}
						if (env->line_no > env->line_count) {
							env->line_no = env->line_count;
						}
						if (env->col_no > env->lines[env->line_no-1]->actual) {
							env->col_no = env->lines[env->line_no-1]->actual;
						}
						set_preferred_column();
						set_modified();
						if (c == 's') {
							env->lines = add_line(env->lines, env->line_no-1);
							redraw_text();
							env->mode = MODE_INSERT;
							return;
						}
						goto _leave_select_line;
					case 'r':
						{
							int c = read_one_character("LINE SELECTION r");
							if (c == -1) break;
							char_t _c = {codepoint_width(c), 0, c};
							int start_point = env->start_line < env->line_no ? env->start_line : env->line_no;
							int end_point = env->start_line < env->line_no ? env->line_no : env->start_line;
							for (int line = start_point; line <= end_point; ++line) {
								for (int i = 0; i < env->lines[line-1]->actual; ++i) {
									line_replace(env->lines[line-1], _c, i, line-1);
								}
							}
						}
						goto _leave_select_line;
					case ':': /* Handle command mode specially for redraw */
						global_config.break_from_selection = 0;
						command_mode();
						if (global_config.break_from_selection) break;
						goto _leave_select_line;
					default:
						handle_navigation(c);
						break;
				}
			} else {
				switch (handle_escape(this_buf,&timeout,c)) {
					case 1:
						bim_unget(c);
						goto _leave_select_line;
					case 'Z':
						/* Unindent */
						if (env->readonly) goto _readonly;
						adjust_indent(env->start_line, -1);
						break;
				}
			}

			reset_nav_buffer(c);

			/* Mark current line */
			_redraw_line(env->line_no,0);
			_redraw_line(env->start_line,1);

			/* Properly mark everything in the span we just moved through */
			if (prev_line < env->line_no) {
				for (int i = prev_line; i < env->line_no; ++i) {
					_redraw_line(i,0);
				}
				prev_line = env->line_no;
			} else if (prev_line > env->line_no) {
				for (int i = env->line_no + 1; i <= prev_line; ++i) {
					_redraw_line(i,0);
				}
				prev_line = env->line_no;
			}
			redraw_commandline();
			place_cursor_actual();
			continue;
_readonly:
			render_error("Buffer is read-only");
		}
	}

_leave_select_line:
	set_history_break();
	env->mode = MODE_NORMAL;
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_syntax(env->lines[i],i);
	}
	redraw_all();
}

#define _redraw_line_col(line, force_start_line) \
	do {\
		if (!(force_start_line) && (line) == env->start_line) break; \
		if ((line) > env->line_count + 1) { \
			if ((line) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 1) { \
				draw_excess_line((line) - env->offset - 1); \
			} \
			break; \
		} \
		if ((line) - env->offset + 1 > 1 && \
			(line) - env->offset - 1< global_config.term_height - global_config.bottom_size - 1) { \
			redraw_line((line) - env->offset - 1, (line)-1); \
		} \
	} while (0)

/**
 * COL INSERT MODE
 *
 * Allows entering text on multiple lines simultaneously.
 * A full multi-cursor insert mode would be way cooler, but
 * this is all we need for my general use case of vim's BLOCK modes.
 */
void col_insert_mode(void) {
	if (env->start_line < env->line_no) {
		/* swap */
		int tmp = env->line_no;
		env->line_no = env->start_line;
		env->start_line = tmp;
	}

	/* Set column to preferred_column */
	env->mode = MODE_COL_INSERT;
	redraw_commandline();
	place_cursor_actual();

	int cin;
	uint32_t c;

	int timeout = 0;
	int this_buf[20];
	uint32_t istate = 0;
	int redraw = 0;
	while ((cin = bim_getch_timeout((redraw ? 10 : 200)))) {
		if (cin == -1) {
			if (redraw) {
				if (redraw & 2) {
					redraw_text();
				} else {
					redraw_line(env->line_no - env->offset - 1, env->line_no-1);
				}
				redraw_statusbar();
				place_cursor_actual();
				redraw = 0;
			}
			if (timeout && this_buf[timeout-1] == '\033') {
				return;
			}
			timeout = 0;
			continue;
		}
		if (!decode(&istate, &c, cin)) {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case 3: /* ^C */
						return;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						if (env->sel_col > 0) {
							int prev_width = 0;
							for (int i = env->line_no; i <= env->start_line; i++) {
								line_t * line = env->lines[i - 1];

								int _x = 0;
								int col = 1;

								int j = 0;
								for (; j < line->actual; ++j) {
									char_t * c = &line->text[j];
									_x += c->display_width;
									col = j+1;
									if (_x > env->sel_col) break;
									prev_width = c->display_width;
								}

								if ((_x == env->sel_col && j == line->actual)) {
									line_delete(line, line->actual, i - 1);
									set_modified();
								} else if (_x > env->sel_col) {
									line_delete(line, col - 1, i - 1);
									set_modified();
								}
							}

							env->sel_col -= prev_width;
							env->col_no--;
							redraw_text();
						}
						break;
					case ENTER_KEY:
					case LINE_FEED:
						/* do nothing in these cases */
						break;
					case 23: /* ^W */
						break;
					case 22: /* ^V */
						render_commandline_message("^V");
						while ((cin = bim_getch()) == -1);
						c = cin;
						redraw_commandline();
						/* fallthrough */
					default:
						/* Okay, this is going to duplicate a lot of insert_char */
						if (c) {
							char_t _c;
							_c.codepoint = c;
							_c.flags = 0;
							_c.display_width = codepoint_width(c);

							int inserted_width = 0;

							/* For each line */
							for (int i = env->line_no; i <= env->start_line; i++) {
								line_t * line = env->lines[i - 1];

								int _x = 0;
								int col = 1;

								int j = 0;
								for (; j < line->actual; ++j) {
									char_t * c = &line->text[j];
									_x += c->display_width;
									col = j+1;
									if (_x > env->sel_col) break;
								}

								if ((_x == env->sel_col && j == line->actual)) {
									_x = env->sel_col + 1;
									col = line->actual + 1;
								}

								if (_x > env->sel_col) {
									line_t * nline = line_insert(line, _c, col - 1, i - 1);
									if (line != nline) {
										env->lines[i - 1] = nline;
										line = nline;
									}
									set_modified();
								}
								recalculate_tabs(line);
								inserted_width = line->text[col-1].display_width;
							}

							env->sel_col += inserted_width;
							env->col_no++;
							redraw_text();

						}
				}
			} else {
				/* Ignore escape sequences for now, but handle them nicely */
				switch (handle_command_escape(this_buf,&timeout,c)) {
					case 1:
						bim_unget(c);
						return;
				}
			}
		} else if (istate == UTF8_REJECT) {
			istate = 0;
		}
	}
}

/**
 * COL SELECTION mode
 *
 * Limited selection mode for doing inserts on multiple lines.
 * Experimental. Based on how I usually use vim's VISUAL BLOCK mode.
 */
void col_selection_mode(void) {
	env->start_line = env->line_no;
	env->sel_col = env->preferred_column;
	int prev_line = env->start_line;

	env->mode = MODE_COL_SELECTION;
	redraw_commandline();

	int c;
	int timeout = 0;
	int this_buf[20];

	while ((c = bim_getch())) {
		if (c == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				goto _leave_select_col;
			}
			timeout = 0;
			continue;
		} else {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case 'I':
						if (env->readonly) goto _readonly;
						col_insert_mode();
						goto _leave_select_col;
					case 'a':
						if (env->readonly) goto _readonly;
						env->sel_col += 1;
						redraw_text();
						col_insert_mode();
						goto _leave_select_col;
					case ':':
						global_config.break_from_selection = 0;
						command_mode();
						if (global_config.break_from_selection) break;
						goto _leave_select_col;
					default:
						handle_navigation(c);
						break;
				}
			} else {
				switch (handle_escape(this_buf,&timeout,c)) {
					case 1:
						bim_unget(c);
						goto _leave_select_col;
					/* Doesn't support anything else. */
				}
			}
		}

		reset_nav_buffer(c);

		_redraw_line_col(env->line_no, 0);
		/* Properly mark everything in the span we just moved through */
		if (prev_line < env->line_no) {
			for (int i = prev_line; i < env->line_no; ++i) {
				_redraw_line_col(i,0);
			}
			prev_line = env->line_no;
		} else if (prev_line > env->line_no) {
			for (int i = env->line_no + 1; i <= prev_line; ++i) {
				_redraw_line_col(i,0);
			}
			prev_line = env->line_no;
		}

		/* prev_line... */
		redraw_commandline();
		place_cursor_actual();
		continue;
_readonly:
		render_error("Buffer is read-only.");
	}

_leave_select_col:
	set_history_break();
	env->mode = MODE_NORMAL;
	redraw_all();
}

/**
 * Determine if a column + line number are within range of the
 * current character selection specified by start_line, etc.
 *
 * Used to determine how syntax flags should be set when redrawing
 * selected text in CHAR SELECTION mode.
 */
int point_in_range(int start_line, int end_line, int start_col, int end_col, int line, int col) {
	if (start_line == end_line) {
		if ( end_col < start_col) {
			int tmp = end_col;
			end_col = start_col;
			start_col = tmp;
		}
		return (col >= start_col && col <= end_col);
	}

	if (start_line > end_line) {
		int tmp = end_line;
		end_line = start_line;
		start_line = tmp;

		tmp = end_col;
		end_col = start_col;
		start_col = tmp;
	}

	if (line < start_line || line > end_line) return 0;

	if (line == start_line) {
		return col >= start_col;
	}

	if (line == end_line) {
		return col <= end_col;
	}

	return 1;
}

#define _redraw_line_char(line, force_start_line) \
	do { \
		if (!(force_start_line) && (line) == start_line) break; \
		if ((line) > env->line_count + 1) { \
			if ((line) - env->offset - 1 < global_config.term_height - global_config.bottom_size - 1) { \
				draw_excess_line((line) - env->offset - 1); \
			} \
			break; \
		} \
		if ((env->line_no < start_line  && ((line) < env->line_no || (line) > start_line)) || \
			(env->line_no > start_line  && ((line) > env->line_no || (line) < start_line)) || \
			(env->line_no == start_line && (line) != start_line)) { \
			/* Line is completely outside selection */ \
			recalculate_syntax(env->lines[(line)-1],(line)-1); \
		} else { \
			if ((line) == start_line || (line) == env->line_no) { \
				recalculate_syntax(env->lines[(line)-1],(line)-1); \
			} \
			for (int j = 0; j < env->lines[(line)-1]->actual; ++j) { \
				if (point_in_range(start_line, env->line_no,start_col, env->col_no, (line), j+1)) { \
					env->lines[(line)-1]->text[j].flags |= FLAG_SELECT; \
				} \
			} \
		} \
		if ((line) - env->offset + 1 > 1 && \
			(line) - env->offset - 1< global_config.term_height - global_config.bottom_size - 1) { \
			redraw_line((line) - env->offset - 1, (line)-1); \
		} \
	} while (0)

/**
 * CHAR SELECTION mode.
 */
void char_selection_mode(void) {
	int start_line = env->line_no;
	int start_col  = env->col_no;
	int prev_line  = start_line;

	env->mode = MODE_CHAR_SELECTION;
	redraw_commandline();

	int c;
	int timeout = 0;
	int this_buf[20];

	/* Select single character */
	env->lines[env->line_no-1]->text[env->col_no-1].flags |= FLAG_SELECT;
	redraw_line(env->line_no - env->offset - 1, env->line_no-1);

	while ((c = bim_getch())) {
		if (c == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				goto _leave_select_char;
			}
			timeout = 0;
			continue;
		} else {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						cursor_left();
						break;
					case 'v':
						goto _leave_select_char;
					case 'y':
						{
							int end_line = env->line_no;
							int end_col  = env->col_no;
							if (start_line == end_line) {
								if (start_col > end_col) {
									int tmp = start_col;
									start_col = end_col;
									end_col = tmp;
								}
							} else if (start_line > end_line) {
								int tmp = start_line;
								start_line = end_line;
								end_line = tmp;
								tmp = start_col;
								start_col = end_col;
								end_col = tmp;
							}
							yank_text(start_line, start_col, end_line, end_col);
						}
						goto _leave_select_char;
					case 'D':
					case 'd':
					case 'x':
					case 's':
						if (env->readonly) goto _readonly;
						{
							int end_line = env->line_no;
							int end_col  = env->col_no;
							if (start_line == end_line) {
								if (start_col > end_col) {
									int tmp = start_col;
									start_col = end_col;
									end_col = tmp;
								}
								yank_text(start_line, start_col, end_line, end_col);
								for (int i = start_col; i <= end_col; ++i) {
									line_delete(env->lines[start_line-1], start_col, start_line - 1);
								}
								env->col_no = start_col;
							} else {
								if (start_line > end_line) {
									int tmp = start_line;
									start_line = end_line;
									end_line = tmp;
									tmp = start_col;
									start_col = end_col;
									end_col = tmp;
								}
								/* Copy lines */
								yank_text(start_line, start_col, end_line, end_col);
								/* Delete lines */
								for (int i = start_line+1; i < end_line; ++i) {
									env->lines = remove_line(env->lines, start_line);
								} /* end_line is no longer valid; should be start_line+1*/
								/* Delete from start_col forward */
								int tmp = env->lines[start_line-1]->actual;
								for (int i = start_col; i <= tmp; ++i) {
									line_delete(env->lines[start_line-1], start_col, start_line - 1);
								}
								for (int i = 1; i <= end_col; ++i) {
									line_delete(env->lines[start_line], 1, start_line);
								}
								/* Merge start and end lines */
								merge_lines(env->lines, start_line);
								env->line_no = start_line;
								env->col_no = start_col;
							}
						}
						if (env->line_no > env->line_count) {
							env->line_no = env->line_count;
						}
						set_preferred_column();
						set_modified();
						if (c == 's') {
							redraw_text();
							env->mode = MODE_INSERT;
							return; /* When returning from char selection mode, normal mode will check for MODE_INSERT */
						}
						goto _leave_select_char;
					case 'r':
						{
							int c = read_one_character("CHAR SELECTION r");
							if (c == -1) break;
							char_t _c = {codepoint_width(c), 0, c};
							/* This should probably be a function line "do_over_range" or something */
							if (start_line == env->line_no) {
								int s = (start_col < env->col_no) ? start_col : env->col_no;
								int e = (start_col < env->col_no) ? env->col_no : start_col;
								for (int i = s; i <= e; ++i) {
									line_replace(env->lines[start_line-1], _c, i-1, start_line-1);
								}
								redraw_text();
							} else {
								if (start_line < env->line_no) {
									for (int s = start_col-1; s < env->lines[start_line-1]->actual; ++s) {
										line_replace(env->lines[start_line-1], _c, s, start_line-1);
									}
									for (int line = start_line + 1; line < env->line_no; ++line) {
										for (int i = 0; i < env->lines[line-1]->actual; ++i) {
											line_replace(env->lines[line-1], _c, i, line-1);
										}
									}
									for (int s = 0; s < env->col_no; ++s) {
										line_replace(env->lines[env->line_no-1], _c, s, env->line_no-1);
									}
								} else {
									for (int s = env->col_no-1; s < env->lines[env->line_no-1]->actual; ++s) {
										line_replace(env->lines[env->line_no-1], _c, s, env->line_no-1);
									}
									for (int line = env->line_no + 1; line < start_line; ++line) {
										for (int i = 0; i < env->lines[line-1]->actual; ++i) {
											line_replace(env->lines[line-1], _c, i, line-1);
										}
									}
									for (int s = 0; s < start_col; ++s) {
										line_replace(env->lines[start_line-1], _c, s, start_line-1);
									}
								}
							}
						}
						goto _leave_select_char;
					case 'A':
						for (int i = 0; i < env->line_count; ++i) {
							recalculate_syntax(env->lines[i],i);
						}
						redraw_text();
						env->col_no = env->col_no > start_col ? env->col_no + 1 : start_col + 1;
						env->mode = MODE_INSERT;
						return;
					case ':':
						global_config.break_from_selection = 0;
						command_mode();
						if (global_config.break_from_selection) break;
						goto _leave_select_char;
					default:
						handle_navigation(c);
						break;
				}
			} else {
				switch (handle_escape(this_buf,&timeout,c)) {
					case 1:
						bim_unget(c);
						goto _leave_select_char;
				}
			}

			reset_nav_buffer(c);

			/* Mark current line */
			_redraw_line_char(env->line_no,1);

			/* Properly mark everything in the span we just moved through */
			if (prev_line < env->line_no) {
				for (int i = prev_line; i < env->line_no; ++i) {
					_redraw_line_char(i,1);
				}
				prev_line = env->line_no;
			} else if (prev_line > env->line_no) {
				for (int i = env->line_no + 1; i <= prev_line; ++i) {
					_redraw_line_char(i,1);
				}
				prev_line = env->line_no;
			}
			place_cursor_actual();
			continue;
_readonly:
			render_error("Buffer is read-only");
		}
	}

_leave_select_char:
	set_history_break();
	env->mode = MODE_NORMAL;
	for (int i = 0; i < env->line_count; ++i) {
		recalculate_syntax(env->lines[i],i);
	}
	redraw_all();
}

struct completion_match {
	char * string;
	char * file;
};

void free_completion_match(struct completion_match * match) {
	if (match->string) free(match->string);
	if (match->file) free(match->file);
}

/**
 * Read ctags file to find matches for a symbol
 */
int read_tags(uint32_t * comp, struct completion_match **matches, int * matches_count) {
	int matches_len = 4;
	*matches_count = 0;
	*matches = malloc(sizeof(struct completion_match) * (matches_len));

	FILE * tags = fopen("tags","r");
	if (!tags) return 1;
	char tmp[4096]; /* max line */
	while (!feof(tags) && fgets(tmp, 4096, tags)) {
		if (tmp[0] == '!') continue;
		int i = 0;
		while (comp[i] && comp[i] == (unsigned int)tmp[i]) i++;
		if (comp[i] == '\0') {
			int j = i;
			while (tmp[j] != '\t' && tmp[j] != '\n' && tmp[j] != '\0') j++;
			tmp[j] = '\0'; j++;
			char * file = &tmp[j];
			while (tmp[j] != '\t' && tmp[j] != '\n' && tmp[j] != '\0') j++;
			tmp[j] = '\0'; j++;

			/* Dedup */
			#if 0
			int match_found = 0;
			for (int i = 0; i < *matches_count; ++i) {
				if (!strcmp((*matches)[i].string, tmp)) {
					match_found = 1;
					break;
				}
			}
			if (match_found) continue;
			#endif

			if (*matches_count == matches_len) {
				matches_len *= 2;
				*matches = realloc(*matches, sizeof(struct completion_match) * (matches_len));
			}
			(*matches)[*matches_count].string = strdup(tmp);
			(*matches)[*matches_count].file = strdup(file);
			(*matches_count)++;
		}
	}
	fclose(tags);
	return 0;
}

/**
 * Draw an autocomplete popover with matches.
 */
void draw_completion_matches(uint32_t * tmp, struct completion_match *matches, int matches_count, int index) {
	int original_length = 0;
	while (tmp[original_length]) original_length++;
	int max_width = 0;
	for (int i = 0; i < matches_count; ++i) {
		/* TODO unicode width */
		unsigned int my_width = strlen(matches[i].string) + (matches[i].file ? strlen(matches[i].file) + 1 : 0);
		if (my_width > (unsigned int)max_width) {
			max_width = my_width;
		}
	}

	/* Figure out how much space we have to display the window */
	int cursor_y = env->line_no - env->offset + 1;
	int max_y = global_config.term_height - 2 - cursor_y;

	/* Find a good place to put the box horizontally */
	int num_size = num_width() + 3;
	int x = num_size + 1 - env->coffset;

	/* Determine where the cursor is physically */
	for (int i = 0; i < env->col_no - 1 - original_length; ++i) {
		char_t * c = &env->lines[env->line_no-1]->text[i];
		x += c->display_width;
	}

	int box_width = max_width;
	int box_x = x;
	int box_y = cursor_y+1;
	if (max_width > env->width - num_width()) {
		box_width = env->width - num_width();
		box_x = 1;
	} else if (env->width - x < max_width) {
		box_width = max_width;
		box_x = env->width - max_width;
	}

	int max_count = (max_y < matches_count) ? max_y - 1 : matches_count;

	for (int x = index; x < max_count+index; ++x) {
		int i = x % matches_count;
		place_cursor(box_x, box_y+x-index);
		set_colors(COLOR_KEYWORD, COLOR_STATUS_BG);
		/* TODO wide characters */
		int match_width = strlen(matches[i].string);
		int file_width = matches[i].file ? strlen(matches[i].file) : 0;
		for (int j = 0; j < box_width; ++j) {
			if (j == original_length) set_colors(i == index ? COLOR_NUMERAL : COLOR_STATUS_FG, COLOR_STATUS_BG);
			if (j == match_width) set_colors(COLOR_TYPE, COLOR_STATUS_BG);
			if (j < match_width) printf("%c", matches[i].string[j]);
			else if (j > match_width && j - match_width - 1 < file_width) printf("%c", matches[i].file[j-match_width-1]);
			else printf(" ");
		}
	}
	if (max_count == 0) {
		place_cursor(box_x, box_y);
		set_colors(COLOR_STATUS_FG, COLOR_STATUS_BG);
		printf(" (no matches) ");
	} else if (max_count != matches_count) {
		place_cursor(box_x, box_y+max_count);
		set_colors(COLOR_STATUS_FG, COLOR_STATUS_BG);
		printf(" (%d more) ", matches_count-max_count);
	}
}

/**
 * Autocomplete words (function/variable names, etc.) in input mode.
 */
int omni_complete(void) {
	int c;

	/* Pull the word from before the cursor */
	int c_before = 0;
	int i = env->col_no-1;
	while (i > 0) {
		if (!c_keyword_qualifier(env->lines[env->line_no-1]->text[i-1].codepoint)) break;
		c_before++;
		i--;
	}

	/* Populate with characters */
	uint32_t * tmp = malloc(sizeof(uint32_t) * (c_before+1));
	int j = 0;
	while (c_before) {
		tmp[j] = env->lines[env->line_no-1]->text[env->col_no-c_before-1].codepoint;
		c_before--;
		j++;
	}
	tmp[j] = 0;

	/*
	 * TODO matches should probably be a struct with more data than just
	 * the matching string; maybe offset where the needle was found,
	 * class information, source file information - anything we can extract
	 * from ctags, but also other information for other sources of completion.
	 */
	struct completion_match *matches;
	int matches_count;

	/* TODO just reading ctags is rather mediocre; can we do something cool here? */
	if (read_tags(tmp, &matches, &matches_count)) goto _completion_done;

	/* Draw box with matches at cursor-width(tmp) */
	draw_completion_matches(tmp, matches, matches_count, 0);

	int retval = 0;
	int index = 0;

_completion_done:
	place_cursor_actual();
	while (1) {
		c = bim_getch();
		if (c == -1) continue;
		if (matches_count < 1) {
			redraw_all();
			break;
		}
		if (c == 15) {
			index = (index + 1) % matches_count;
			draw_completion_matches(tmp, matches, matches_count, index);
			place_cursor_actual();
			continue;
		} else if (c == '\t') {
			for (unsigned int i = j; i < strlen(matches[index].string); ++i) {
				insert_char(matches[index].string[i]);
			}
			set_preferred_column();
			redraw_text();
			place_cursor_actual();
			goto _finish_completion;
		} else if (isgraph(c) && c != '}') {
			/* insert and continue matching */
			insert_char(c);
			set_preferred_column();
			redraw_text();
			place_cursor_actual();
			retval = 1;
			goto _finish_completion;
		} else if (c == DELETE_KEY || c == BACKSPACE_KEY) {
			delete_at_cursor();
			set_preferred_column();
			redraw_text();
			place_cursor_actual();
			retval = 1;
			goto _finish_completion;
		}
		/* TODO: Keyboard navigation of the matches list would be nice */
		redraw_all();
		break;
	}
	bim_unget(c);
_finish_completion:
	for (int i = 0; i < matches_count; ++i) {
		free_completion_match(&matches[i]);
	}
	free(matches);
	free(tmp);
	return retval;
}

/**
 * INSERT mode
 *
 * Accept input into the text buffer.
 */
void insert_mode(void) {
	int cin;
	uint32_t c;

	/* Set mode line */
	env->mode = MODE_INSERT;
	redraw_commandline();

	/* Place the cursor in the text area */
	place_cursor_actual();

	int timeout = 0;
	int this_buf[20];
	uint32_t istate = 0;
	int redraw = 0;
	while ((cin = bim_getch_timeout((redraw ? 10 : 200)))) {
		if (cin == -1) {
			if (redraw) {
				if (redraw & 2) {
					redraw_text();
				} else {
					redraw_line(env->line_no - env->offset - 1, env->line_no-1);
				}
				redraw_statusbar();
				place_cursor_actual();
				redraw = 0;
			}
			if (timeout && this_buf[timeout-1] == '\033') {
				leave_insert();
				return;
			}
			timeout = 0;
			continue;
		}
		if (!decode(&istate, &c, cin)) {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case 3: /* ^C */
						leave_insert();
						return;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						if (!env->tabs && env->col_no > 1) {
							int i;
							for (i = 0; i < env->col_no-1; ++i) {
								if (!is_whitespace(env->lines[env->line_no-1]->text[i].codepoint)) break;
							}
							if (i == env->col_no-1) {
								/* Backspace until aligned */
								delete_at_cursor();
								while (env->col_no > 1 && (env->col_no-1) % env->tabstop) {
									delete_at_cursor();
								}
								redraw |= 2;
								break; /* out of case */
							}
						}
						delete_at_cursor();
						break;
					case ENTER_KEY:
					case LINE_FEED:
						if (env->indent) {
							if ((env->lines[env->line_no-1]->text[env->col_no-2].flags & 0x1F) == FLAG_COMMENT &&
								(env->lines[env->line_no-1]->text[env->col_no-2].codepoint == ' ') &&
								(env->col_no > 3) &&
								(env->lines[env->line_no-1]->text[env->col_no-3].codepoint == '*')) {
								delete_at_cursor();
							}
						}
						insert_line_feed();
						redraw |= 2;
						break;
					case 15: /* ^O */
						while (omni_complete() == 1);
						break;
					case 22: /* ^V */
						/* Insert next byte raw */
						{
							/* Indicate we're in literal mode */
							render_commandline_message("^V");
							/* Put the cursor back into the text field */
							place_cursor_actual();
							/* Get next character */
							while ((cin = bim_getch()) == -1);
							/* Insert literal */
							insert_char(cin);
							/* Redraw INSERT */
							redraw_commandline();
							/* Draw text */
							redraw |= 1;
						}
						break;
					case 23: /* ^W */
						delete_word();
						set_preferred_column();
						break;
					case '\t':
						if (env->tabs) {
							insert_char('\t');
						} else {
							for (int i = 0; i < env->tabstop; ++i) {
								insert_char(' ');
							}
						}
						redraw |= 1;
						set_preferred_column();
						break;
					case '/':
						if (env->indent) {
							if ((env->lines[env->line_no-1]->text[env->col_no-2].flags & 0x1F) == FLAG_COMMENT &&
								(env->lines[env->line_no-1]->text[env->col_no-2].codepoint == ' ') &&
								(env->col_no > 3) &&
								(env->lines[env->line_no-1]->text[env->col_no-3].codepoint == '*')) {
								env->col_no--;
								replace_char('/');
								env->col_no++;
								place_cursor_actual();
								break;
							}
						}
						goto _just_insert;
					case '}':
						if (env->indent) {
							int was_whitespace = 1;
							for (int i = 0; i < env->lines[env->line_no-1]->actual; ++i) {
								if (env->lines[env->line_no-1]->text[i].codepoint != ' ' &&
									env->lines[env->line_no-1]->text[i].codepoint != '\t') {
									was_whitespace = 0;
									break;
								}
							}
							insert_char('}');
							if (was_whitespace) {
								int line = -1, col = -1;
								env->col_no--;
								find_matching_paren(&line,&col, 1);
								if (line != -1) {
									while (env->lines[env->line_no-1]->actual) {
										line_delete(env->lines[env->line_no-1], env->lines[env->line_no-1]->actual, env->line_no-1);
									}
									add_indent(env->line_no-1,line-1,1);
									env->col_no = env->lines[env->line_no-1]->actual + 1;
									insert_char('}');
								}
							}
							set_preferred_column();
							redraw |= 1;
							break;
						}
						/* fallthrough */
					default:
_just_insert:
						insert_char(c);
						set_preferred_column();
						redraw |= 1;
						break;
				}
			} else {
				if (handle_escape(this_buf,&timeout,c)) {
					bim_unget(c);
					leave_insert();
					return;
				}
			}
		} else if (istate == UTF8_REJECT) {
			istate = 0;
		}
	}
}

/**
 * REPLACE mode
 *
 * Like insert, but replaces characters.
 */
void replace_mode(void) {
	int cin;
	uint32_t c;

	/* Set mode line */
	env->mode = MODE_REPLACE;
	redraw_commandline();

	/* Place the cursor in the text area */
	place_cursor_actual();

	int timeout = 0;
	int this_buf[20];
	uint32_t istate = 0;
	while ((cin = bim_getch())) {
		if (cin == -1) {
			if (timeout && this_buf[timeout-1] == '\033') {
				leave_insert();
				return;
			}
			timeout = 0;
			continue;
		}
		if (!decode(&istate, &c, cin)) {
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						if (env->line_no > 1 && env->col_no == 1) {
							env->line_no--;
							env->col_no = env->lines[env->line_no-1]->actual;
							set_preferred_column();
							place_cursor_actual();
						} else {
							cursor_left();
						}
						break;
					case ENTER_KEY:
					case LINE_FEED:
						insert_line_feed();
						redraw_text();
						set_modified();
						redraw_statusbar();
						place_cursor_actual();
						break;
					default:
						if (env->col_no <= env->lines[env->line_no - 1]->actual) {
							replace_char(c);
							env->col_no += 1;
						} else {
							insert_char(c);
							redraw_line(env->line_no - env->offset - 1, env->line_no-1);
						}
						set_preferred_column();
						redraw_statusbar();
						place_cursor_actual();
						break;
				}
			} else {
				if (handle_escape(this_buf,&timeout,c)) {
					bim_unget(c);
					leave_insert();
					return;
				}
			}
		} else if (istate == UTF8_REJECT) {
			istate = 0;
		}
	}
}

/**
 * NORMAL mode
 *
 * Default editor mode - just cursor navigation and keybinds
 * to enter the other modes.
 */
void normal_mode(void) {

	while (1) {
		place_cursor_actual();
		int c;
		int timeout = 0;
		int this_buf[20];
		while ((c = bim_getch())) {
			if (c == -1) {
				/* getch timed out, nothing to do in normal mode */
				continue;
			}
			if (timeout == 0) {
				switch (c) {
					case '\033':
						if (timeout == 0) {
							this_buf[timeout] = c;
							timeout++;
						}
						break;
					case DELETE_KEY:
					case BACKSPACE_KEY:
						if (env->line_no > 1 && env->col_no == 1) {
							env->line_no--;
							env->col_no = env->lines[env->line_no-1]->actual;
							set_preferred_column();
							place_cursor_actual();
						} else {
							cursor_left();
						}
						break;
					case 'V': /* Enter LINE SELECTION mode */
						line_selection_mode();
						if (env->mode == MODE_INSERT) goto _insert;
						break;
					case 'v': /* Enter CHAR SELECTION mode */
						char_selection_mode();
						if (env->mode == MODE_INSERT) goto _insert;
						break;
					case 22: /* ctrl-v, enter COL SELECTION mode */
						set_preferred_column();
						col_selection_mode();
						break;
					case 'O': /* Append line before and enter INSERT mode */
						{
							if (env->readonly) goto _readonly;
							env->lines = add_line(env->lines, env->line_no-1);
							env->col_no = 1;
							add_indent(env->line_no-1,env->line_no,0);
							redraw_text();
							set_preferred_column();
							set_modified();
							place_cursor_actual();
							goto _insert;
						}
					case 'o': /* Append line after and enter INSERT mode */
						{
							if (env->readonly) goto _readonly;
							env->lines = add_line(env->lines, env->line_no);
							env->col_no = 1;
							env->line_no += 1;
							add_indent(env->line_no-1,env->line_no-2,0);
							set_preferred_column();
							if (env->line_no > env->offset + global_config.term_height - global_config.bottom_size - 1) {
								env->offset += 1;
							}
							redraw_text();
							set_modified();
							place_cursor_actual();
							goto _insert;
						}
					case 'a': /* Enter INSERT mode with cursor after current position */
						if (env->col_no < env->lines[env->line_no-1]->actual + 1) {
							env->col_no += 1;
						}
						goto _insert;
					case 's':
					case 'x':
						if (env->col_no <= env->lines[env->line_no-1]->actual) {
							line_delete(env->lines[env->line_no-1], env->col_no, env->line_no-1);
							redraw_text();
						}
						if (c == 's') goto _insert;
						set_history_break();
						break;
					case 'P': /* Paste before */
					case 'p': /* Paste after */
						if (env->readonly) goto _readonly;
						if (global_config.yanks) {
							if (!global_config.yank_is_full_lines) {
								/* Handle P for paste before, p for past after */
								int target_column = (c == 'P' ? (env->col_no) : (env->col_no+1));
								if (target_column > env->lines[env->line_no-1]->actual + 1) {
									target_column = env->lines[env->line_no-1]->actual + 1;
								}
								if (global_config.yank_count > 1) {
									/* Spit the current line at the current position */
									env->lines = split_line(env->lines, env->line_no - 1, target_column - 1); /* Split after */
								}
								/* Insert first line at current position */
								for (int i = 0; i < global_config.yanks[0]->actual; ++i) {
									env->lines[env->line_no - 1] = line_insert(env->lines[env->line_no - 1], global_config.yanks[0]->text[i], target_column + i - 1, env->line_no - 1); 
								}
								if (global_config.yank_count > 1) {
									/* Insert full lines */
									for (unsigned int i = 1; i < global_config.yank_count - 1; ++i) {
										env->lines = add_line(env->lines, env->line_no);
									}
									for (unsigned int i = 1; i < global_config.yank_count - 1; ++i) {
										replace_line(env->lines, env->line_no + i - 1, global_config.yanks[i]);
									}
									/* Insert characters from last line into (what was) the next line */
									for (int i = 0; i < global_config.yanks[global_config.yank_count-1]->actual; ++i) {
										env->lines[env->line_no + global_config.yank_count - 2] = line_insert(env->lines[env->line_no + global_config.yank_count - 2], global_config.yanks[global_config.yank_count-1]->text[i], i, env->line_no + global_config.yank_count - 2);
									}
								}
							} else {
								/* Insert full lines */
								for (unsigned int i = 0; i < global_config.yank_count; ++i) {
									env->lines = add_line(env->lines, env->line_no - (c == 'P' ? 1 : 0));
								}
								for (unsigned int i = 0; i < global_config.yank_count; ++i) {
									replace_line(env->lines, env->line_no - (c == 'P' ? 1 : 0) + i, global_config.yanks[i]);
								}
							}
							/* Recalculate whole document syntax */
							for (int i = 0; i < env->line_count; ++i) {
								env->lines[i]->istate = 0;
							}
							for (int i = 0; i < env->line_count; ++i) {
								recalculate_syntax(env->lines[i],i);
							}
							if (c == 'p') {
								if (global_config.yank_is_full_lines) {
									env->line_no += 1;
								} else {
									if (global_config.yank_count == 1) {
										env->col_no = env->col_no + global_config.yanks[0]->actual;
									} else {
										env->line_no = env->line_no + global_config.yank_count - 1;
										env->col_no = global_config.yanks[global_config.yank_count-1]->actual;
									}
								}
							}
							if (global_config.yank_is_full_lines) {
								env->col_no = 1;
								for (int i = 0; i < env->lines[env->line_no-1]->actual; ++i) {
									if (!is_whitespace(env->lines[env->line_no-1]->text[i].codepoint)) {
										env->col_no = i + 1;
										break;
									}
								}
							}
							set_history_break();
							set_modified();
							redraw_all();
						}
						break;
					case 'r': /* Replace with next */
						{
							int c = read_one_character("r");
							if (c != -1) {
								replace_char(c);
							}
						}
						break;
					case 'A':
						env->col_no = env->lines[env->line_no-1]->actual+1;
						goto _insert;
					case 'u': /* Undo one block of history */
						undo_history();
						break;
					case 18: /* ^R - Redo one block of history */
						redo_history();
						break;
					case 12: /* ^L - Repaint the whole screen */
						redraw_all();
						break;
					case 'i': /* Enter INSERT mode */
_insert:
						if (env->readonly) goto _readonly;
						insert_mode();
						redraw_statusbar();
						redraw_commandline();
						timeout = 0;
						break;
					case 'R': /* Enter REPLACE mode */
						if (env->readonly) goto _readonly;
						replace_mode();
						redraw_statusbar();
						redraw_commandline();
						timeout = 0;
						break;
_readonly:
						render_error("Buffer is read-only");
						break;
					default:
						handle_navigation(c);
						break;
				}
			} else {
				handle_escape(this_buf,&timeout,c);
			}
			reset_nav_buffer(c);
			place_cursor_actual();
		}
	}

}

/**
 * Show help text for -?
 */
static void show_usage(char * argv[]) {
#define _s "\033[3m"
#define _e "\033[0m\n"
	printf(
			"bim - Text editor\n"
			"\n"
			"usage: %s [options] [file]\n"
			"       %s [options] -- -\n"
			"\n"
			" -R     " _s "open initial buffer read-only" _e
			" -O     " _s "set various options:" _e
			"        noscroll    " _s "disable terminal scrolling" _e
			"        noaltscreen " _s "disable alternate screen buffer" _e
			"        nomouse     " _s "disable mouse support" _e
			"        nounicode   " _s "disable unicode display" _e
			"        nobright    " _s "disable bright next" _e
			"        nohideshow  " _s "disable togglging cursor visibility" _e
			"        nosyntax    " _s "disable syntax highlighting on load" _e
			"        notitle     " _s "disable title-setting escapes" _e
			"        history     " _s "enable experimental undo/redo" _e
			" -c,-C  " _s "print file to stdout with syntax highlighting" _e
			"        " _s "-C includes line numbers, -c does not" _e
			" -u     " _s "override bimrc file" _e
			" -?     " _s "show this help text" _e
			" --version " _s "show version information and available plugins" _e
			"\n", argv[0], argv[0]);
#undef _e
#undef _s
}

/**
 * Run global initialization tasks
 */
void initialize(void) {
	setlocale(LC_ALL, "");

	detect_weird_terminals();
	load_colorscheme_ansi();
	load_bimrc();

	buffers_avail = 4;
	buffers = malloc(sizeof(buffer_t *) * buffers_avail);
}

int main(int argc, char * argv[]) {
	int opt;
	while ((opt = getopt(argc, argv, "?c:C:u:RS:O:-:")) != -1) {
		switch (opt) {
			case 'R':
				global_config.initial_file_is_read_only = 1;
				break;
			case 'c':
			case 'C':
				/* Print file to stdout using our syntax highlighting and color theme */
				initialize();
				global_config.go_to_line = 0;
				open_file(optarg);
				for (int i = 0; i < env->line_count; ++i) {
					if (opt == 'C') {
						draw_line_number(i);
					}
					render_line(env->lines[i], 6 * (env->lines[i]->actual + 1), 0, i + 1);
					reset();
					fprintf(stdout, "\n");
				}
				return 0;
			case 'u':
				global_config.bimrc_path = optarg;
				break;
			case 'S':
				global_config.syntax_fallback = optarg;
				break;
			case 'O':
				/* Set various display options */
				if (!strcmp(optarg,"noaltscreen"))     global_config.can_altscreen = 0;
				else if (!strcmp(optarg,"noscroll"))   global_config.can_scroll = 0;
				else if (!strcmp(optarg,"nomouse"))    global_config.can_mouse = 0;
				else if (!strcmp(optarg,"nounicode"))  global_config.can_unicode = 0;
				else if (!strcmp(optarg,"nobright"))   global_config.can_bright = 0;
				else if (!strcmp(optarg,"nohideshow")) global_config.can_hideshow = 0;
				else if (!strcmp(optarg,"nosyntax"))   global_config.highlight_on_open = 0;
				else if (!strcmp(optarg,"nohistory"))  global_config.history_enabled = 0;
				else if (!strcmp(optarg,"notitle"))    global_config.can_title = 0;
				else if (!strcmp(optarg,"nobce"))      global_config.can_bce = 0;
				else {
					fprintf(stderr, "%s: unrecognized -O option: %s\n", argv[0], optarg);
					return 1;
				}
				break;
			case '-':
				if (!strcmp(optarg,"version")) {
					fprintf(stderr, "bim %s %s\n", BIM_VERSION, BIM_COPYRIGHT);
					fprintf(stderr, " Available syntax highlighters:");
					for (struct syntax_definition * s = syntaxes; s->name; ++s) {
						fprintf(stderr, " %s", s->name);
					}
					fprintf(stderr, "\n");
					fprintf(stderr, " Available color themes:");
					for (struct theme_def * d = themes; d->name; ++d) {
						fprintf(stderr, " %s", d->name);
					}
					fprintf(stderr, "\n");
					return 0;
				} else if (!strcmp(optarg,"help")) {
					show_usage(argv);
					return 0;
				} else if (strlen(optarg)) {
					fprintf(stderr, "bim: unrecognized option `%s'\n", optarg);
					return 1;
				} /* Else, this is -- to indicate end of arguments */
				break;
			case '?':
				show_usage(argv);
				return 0;
		}
	}

	/* Set up terminal */
	initialize();
	init_terminal();

	/* Open file */
	if (argc > optind) {
		while (argc > optind) {
			open_file(argv[optind]);
			update_title();
			if (global_config.initial_file_is_read_only) {
				env->readonly = 1;
			}
			optind++;
		}
		env = buffers[0];
	} else {
		env = buffer_new();
		setup_buffer(env);
	}

	update_title();

	/* Draw the screen once */
	redraw_all();

	/* Start accepting key commands */
	normal_mode();

	return 0;
}
