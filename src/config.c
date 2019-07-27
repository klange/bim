#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "buffer.h"
#include "syntax.h"
#include "themes.h"

/**
 * Global configuration state
 */
struct bim_config global_config = {
	0, /* term_width */
	0, /* term_height */
	2, /* bottom_size */
	NULL, /* yanks */
	0, /* yank_count */
	0, /* yank is full lines */
	STDIN_FILENO, /* tty_in */
	"~/.bimrc", /* bimrc_path */
	1, /* highlight_on_open */
	0, /* initial_file_is_read_only */
	1, /* can scroll */
	1, /* can hide/show cursor */
	1, /* can use alternate screen */
	1, /* can mouse */
	1, /* can unicode */
	1, /* can use bright colors */
	1, /* can set title */
	1, /* can bce */
	1, /* history enabled */
	1, /* highlight parens/braces when cursor moves */
	1, /* smart case */
	1, /* can use 24-bit color */
	1, /* can use 265 colors */
	1, /* can use italics (without inverting) */
	1, /* should go to line when opening file */
	1, /* highlight the current line */
	1, /* shift scrolling (shifts view rather than moving cursor) */
	0, /* check git on open and on save */
	1, /* color the gutter for modified lines */
	0, /* relative line numbers */
	1, /* status bit for whether command should NOT break from selection */
	/* Things below this are outside of the simple on-off bitmap */
	4, /* cursor padding */
	50, /* split percentage */
	5, /* how many lines to scroll on mouse wheel */
	NULL, /* syntax to fall back to if none other match applies */
	NULL, /* search text */
};


/**
 * Load bimrc configuration file.
 *
 * At the moment, this a simple key=value list.
 */
void load_bimrc(void) {
	if (!global_config.bimrc_path) return;

	/* Default is ~/.bimrc */
	char * tmp = strdup(global_config.bimrc_path);

	if (!*tmp) {
		free(tmp);
		return;
	}

	/* Parse ~ at the front of the path. */
	if (*tmp == '~') {
		char path[1024] = {0};
		char * home = getenv("HOME");
		if (!home) {
			/* $HOME is unset? */
			free(tmp);
			return;
		}

		/* New path is $HOME/.bimrc */
		snprintf(path, 1024, "%s%s", home, tmp+1);
		free(tmp);
		tmp = strdup(path);
	}

	/* Try to open the file */
	FILE * bimrc = fopen(tmp, "r");
	free(tmp);

	if (!bimrc) {
		/* No bimrc, or bad permissions */
		return;
	}

	/* Parse through lines */
	char line[1024];
	while (!feof(bimrc)) {
		char * l = fgets(line, 1023, bimrc);

		/* Ignore bad lines */
		if (!l) break;
		if (!*l) continue;
		if (*l == '\n') continue;

		/* Ignore comment lines */
		if (*l == '#') continue;

		/* Remove linefeed at the end */
		char *nl = strstr(l,"\n");
		if (nl) *nl = '\0';

		/* Extract value from keypair, if available
		 * (I foresee options without values in the future) */
		char *value= strstr(l,"=");
		if (value) {
			*value = '\0';
			value++;
		}

		/* theme=... */
		if (!strcmp(l,"theme") && value) {
			/* Examine available themes for a match. */
			for (struct theme_def * d = themes; d->name; ++d) {
				if (!strcmp(value, d->name)) {
					d->load();
					break;
				}
			}
		}

		/* enable history (experimental) */
		if (!strcmp(l,"history")) {
			global_config.history_enabled = (value ? atoi(value) : 1);
		}

		/* padding= */
		if (!strcmp(l,"padding") && value) {
			global_config.cursor_padding = atoi(value);
		}

		if (!strcmp(l,"hlparen") && value) {
			global_config.highlight_parens = atoi(value);
		}

		/* Disable highlighting of current line */
		if (!strcmp(l,"hlcurrent") && value) {
			global_config.highlight_current_line = atoi(value);
		}

		/* Relative line numbers */
		if (!strcmp(l,"relativenumber") && value) {
			global_config.relative_lines = atoi(value);
		}

		if (!strcmp(l,"splitpercent") && value) {
			global_config.split_percent = atoi(value);
		}

		if (!strcmp(l,"shiftscrolling")) {
			global_config.shift_scrolling = (value ? atoi(value) : 1);
		}

		if (!strcmp(l,"scrollamount") && value) {
			global_config.scroll_amount = atoi(value);
		}

		if (!strcmp(l,"git") && value) {
			global_config.check_git = !!atoi(value);
		}

		if (!strcmp(l,"colorgutter") && value) {
			global_config.color_gutter = !!atoi(value);
		}
	}

	fclose(bimrc);
}
