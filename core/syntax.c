#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

/**
 * Find keywords from a list and paint them, assuming they aren't in the middle of other words.
 * Returns 1 if a keyword from the last was found, otherwise 0.
 */
int find_keywords(struct syntax_state * state, char ** keywords, int flag, int (*keyword_qualifier)(int c)) {
	if (keyword_qualifier(lastchar())) return 0;
	if (!keyword_qualifier(charat())) return 0;
	for (char ** keyword = keywords; *keyword; ++keyword) {
		int d = 0;
		while (state->i + d < state->line->actual && state->line->text[state->i+d].codepoint == (*keyword)[d]) d++;
		if ((*keyword)[d] == '\0' && (state->i + d >= state->line->actual || !keyword_qualifier(state->line->text[state->i+d].codepoint))) {
			paint((int)strlen(*keyword), flag);
			return 1;
		}
	}

	return 0;
}

/**
 * Match and paint a single keyword. Returns 1 if the keyword was matched and 0 otherwise,
 * so it can be used for prefix checking for things that need further special handling.
 */
int match_and_paint(struct syntax_state * state, const char * keyword, int flag, int (*keyword_qualifier)(int c)) {
	if (keyword_qualifier(lastchar())) return 0;
	if (!keyword_qualifier(charat())) return 0;
	int i = state->i;
	int slen = 0;
	while (i < state->line->actual || *keyword == '\0') {
		if (*keyword == '\0' && (i >= state->line->actual || !keyword_qualifier(state->line->text[i].codepoint))) {
			for (int j = 0; j < slen; ++j) {
				paint(1, flag);
			}
			return 1;
		}
		if (*keyword != state->line->text[i].codepoint) return 0;

		i++;
		keyword++;
		slen++;
	}
	return 0;
}

void paint_simple_string(struct syntax_state * state) {
	/* Assumes you came in from a check of charat() == '"' */
	paint(1, FLAG_STRING);
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == '"') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '"') {
			paint(1, FLAG_STRING);
			return;
		} else if (charat() == '\\') {
			paint(2, FLAG_ESCAPE);
		} else {
			paint(1, FLAG_STRING);
		}
	}
}

/**
 * These words can appear in comments and should be highlighted.
 * Since there are a lot of comment highlighters, let's break them out.
 */
int common_comment_buzzwords(struct syntax_state * state) {
	if (match_and_paint(state, "TODO", FLAG_NOTICE, c_keyword_qualifier)) { return 1; }
	else if (match_and_paint(state, "XXX", FLAG_NOTICE, c_keyword_qualifier)) { return 1; }
	else if (match_and_paint(state, "FIXME", FLAG_ERROR, c_keyword_qualifier)) { return 1; }
	return 0;
}

/**
 * Paint a comment until end of line, assumes this comment can not continue.
 * (Some languages have comments that can continue with a \ - don't use this!)
 * Assumes you've already painted your comment start characters.
 */
int paint_comment(struct syntax_state * state) {
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else { paint(1, FLAG_COMMENT); }
	}
	return -1;
}

/**
 * Match a word forward and return whether it was matched.
 */
int match_forward(struct syntax_state * state, char * c) {
	int i = 0;
	while (1) {
		if (charrel(i) == -1 && !*c) return 1;
		if (charrel(i) != *c) return 0;
		c++;
		i++;
	}
	return 0;
}

/**
 * Find and return a highlighter by name, or NULL
 */
struct syntax_definition * find_syntax_calculator(const char * name) {
	for (struct syntax_definition * s = syntaxes; syntaxes && s->name; ++s) {
		if (!strcmp(s->name, name)) {
			return s;
		}
	}
	return NULL;
}

int syntax_count = 0;
int syntax_space = 0;
struct syntax_definition * syntaxes = NULL;

void add_syntax(struct syntax_definition def) {
	if (syntax_space == 0) {
		syntax_space = 4;
		syntaxes = calloc(sizeof(struct syntax_definition), syntax_space);
	} else if (syntax_count == syntax_space) {
		syntax_space *= 2;
		syntaxes = realloc(syntaxes, sizeof(struct syntax_definition) * syntax_space);
	}
	syntaxes[syntax_count] = def;
	syntax_count++;
}

/**
 * Calculate syntax highlighting for the given line.
 */
void recalculate_syntax(line_t * line, int line_no) {
	/* Clear syntax for this line first */
	int is_original = 1;
	while (1) {
		for (int i = 0; i < line->actual; ++i) {
			line->text[i].flags = 0;
		}

		if (!env->syntax) {
			rehighlight_search(line);
			return;
		}

		/* Start from the line's stored in initial state */
		struct syntax_state state;
		state.line = line;
		state.line_no = line_no;
		state.state = line->istate;
		state.i = 0;

		while (1) {
			state.state = env->syntax->calculate(&state);

			if (state.state != 0) {
				if (line_no == -1) return;
				rehighlight_search(line);
				if (!is_original) {
					redraw_line(line_no);
				}
				if (line_no + 1 < env->line_count && env->lines[line_no+1]->istate != state.state) {
					line_no++;
					line = env->lines[line_no];
					line->istate = state.state;
					if (env->loading) return;
					is_original = 0;
					goto _next;
				}
				return;
			}
		}
_next:
		(void)0;
	}
}

/**
 * Recalculate tab widths.
 */
void recalculate_tabs(line_t * line) {
	if (env->loading) return;

	int j = 0;
	for (int i = 0; i < line->actual; ++i) {
		if (line->text[i].codepoint == '\t') {
			line->text[i].display_width = env->tabstop - (j % env->tabstop);
		}
		j += line->text[i].display_width;
	}
}

/**
 * Find a syntax highlighter for the given filename.
 */
struct syntax_definition * match_syntax(char * file) {
	for (struct syntax_definition * s = syntaxes; syntaxes && s->name; ++s) {
		for (char ** ext = s->ext; *ext; ++ext) {
			int i = strlen(file);
			int j = strlen(*ext);

			do {
				if (file[i] != (*ext)[j]) break;
				if (j == 0) return s;
				if (i == 0) break;
				i--;
				j--;
			} while (1);
		}
	}

	return NULL;
}

/**
 * Set the syntax configuration by the name of the syntax highlighter.
 */
void set_syntax_by_name(const char * name) {
	if (!strcmp(name,"none")) {
		for (int i = 0; i < env->line_count; ++i) {
			env->lines[i]->istate = -1;
			for (int j = 0; j < env->lines[i]->actual; ++j) {
				env->lines[i]->text[j].flags = 0;
			}
		}
		redraw_all();
		return;
	}
	for (struct syntax_definition * s = syntaxes; syntaxes && s->name; ++s) {
		if (!strcmp(name,s->name)) {
			env->syntax = s;
			for (int i = 0; i < env->line_count; ++i) {
				env->lines[i]->istate = -1;
			}
			for (int i = 0; i < env->line_count; ++i) {
				recalculate_syntax(env->lines[i],i);
			}
			redraw_all();
			return;
		}
	}
	render_error("unrecognized syntax type");
}


