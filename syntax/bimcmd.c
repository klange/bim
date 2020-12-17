#include "bim-core.h"
#include "bim-syntax.h"

int cmd_qualifier(int c) { return c != -1 && c != ' '; }

extern int syn_bash_calculate(struct syntax_state * state);
extern int syn_py_calculate(struct syntax_state * state);

static int bimcmd_paint_replacement(struct syntax_state * state) {
	paint(1, FLAG_KEYWORD);
	char special = charat();
	paint(1, FLAG_TYPE);
	while (charat() != -1 && charat() != special) {
		paint(1, FLAG_DIFFMINUS);
	}
	if (charat() == special) paint(1, FLAG_TYPE);
	while (charat() != -1 && charat() != special) {
		paint(1, FLAG_DIFFPLUS);
	}
	if (charat() == special) paint(1, FLAG_TYPE);
	while (charat() != -1) paint(1, FLAG_NUMERAL);
	return -1;
}

extern struct command_def * regular_commands;
extern struct command_def * prefix_commands;

static int bimcmd_find_commands(struct syntax_state * state) {
	for (struct command_def * c = regular_commands; regular_commands && c->name; ++c) {
		if (match_and_paint(state, c->name, FLAG_KEYWORD, cmd_qualifier)) return 1;
	}
	for (struct command_def * c = prefix_commands; prefix_commands && c->name; ++c) {
		if (match_and_paint(state, c->name, FLAG_KEYWORD, cmd_qualifier)) return 1;
	}
	return 0;
}

static char * bimscript_comments[] = {
	"@author","@version","@url","@description",
	NULL
};

static int bcmd_at_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_') || (c == '@');
}

int syn_bimcmd_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		while (charat() == ' ') skip();
		if (charat() == '#') {
			while (charat() != -1) {
				if (charat() == '@') {
					if (!find_keywords(state, bimscript_comments, FLAG_ESCAPE, bcmd_at_keyword_qualifier)) {
						paint(1, FLAG_COMMENT);
					}
				} else {
					paint(1, FLAG_COMMENT);
				}
			}
			return -1;
		} else if (match_and_paint(state, "function", FLAG_PRAGMA, cmd_qualifier)) {
			while (charat() == ' ') skip();
			while (charat() != -1 && charat() != ' ') paint(1, FLAG_TYPE);
			while (charat() != -1) paint(1, FLAG_ERROR);
			return -1;
		} else if (match_and_paint(state, "end", FLAG_PRAGMA, cmd_qualifier)) {
			while (charat() != -1) paint(1, FLAG_ERROR);
			return -1;
		} else if (match_and_paint(state, "return", FLAG_PRAGMA, cmd_qualifier)) {
			while (charat() == ' ') skip();
			while (charat() != -1 && charat() != ' ') paint(1, FLAG_NUMERAL);
			return -1;
		} else if (match_and_paint(state, "call", FLAG_KEYWORD, cmd_qualifier) ||
			match_and_paint(state, "trycall", FLAG_KEYWORD, cmd_qualifier) ||
			match_and_paint(state, "showfunction", FLAG_KEYWORD, cmd_qualifier)) {
			while (charat() == ' ') skip();
			for (struct bim_function ** f = user_functions; user_functions && *f; ++f) {
				if (match_and_paint(state, (*f)->command, FLAG_TYPE, cmd_qualifier)) break;
			}
		} else if (match_and_paint(state, "theme", FLAG_KEYWORD, cmd_qualifier) ||
			match_and_paint(state, "colorscheme", FLAG_KEYWORD, cmd_qualifier)) {
			while (charat() == ' ') skip();
			for (struct theme_def * s = themes; themes && s->name; ++s) {
				if (match_and_paint(state, s->name, FLAG_TYPE, cmd_qualifier)) break;
			}
		} else if (match_and_paint(state, "syntax", FLAG_KEYWORD, cmd_qualifier)) {
			while (charat() == ' ') skip();
			for (struct syntax_definition * s = syntaxes; syntaxes && s->name; ++s) {
				if (match_and_paint(state, s->name, FLAG_TYPE, cmd_qualifier)) return -1;
			}
			if (match_and_paint(state, "none", FLAG_TYPE, cmd_qualifier)) return -1;
		} else if (match_and_paint(state, "setcolor", FLAG_KEYWORD, cmd_qualifier)) {
			while (charat() == ' ') skip();
			for (struct ColorName * c = color_names; c->name; ++c) {
				if (match_and_paint(state, c->name, FLAG_TYPE, cmd_qualifier)) {
					while (charat() != -1) paint(1, FLAG_STRING);
					return -1;
				}
			}
			return -1;
		} else if (match_and_paint(state, "quirk", FLAG_KEYWORD, cmd_qualifier)) {
			if (charat() == ' ') skip();
			while (charat() != -1 && charat() != ' ') paint(1, FLAG_TYPE);
			if (charat() == ' ') skip();
			while (charat() != -1 && charat() != ' ') paint(1, FLAG_NUMERAL);
			if (charat() != ' ') skip();
			while (charat() != -1) {
				if (charat() == 'n' && nextchar() == 'o') {
					while (charat() != -1 && charat() != ' ') paint(1, FLAG_DIFFMINUS);
				} else if (charat() == 'c' && nextchar() == 'a' && charrel(2) == 'n') {
					while (charat() != -1 && charat() != ' ') paint(1, FLAG_DIFFPLUS);
				} else {
					while (charat() != -1 && charat() != ' ') paint(1, FLAG_ERROR);
				}
				if (charat() == ' ') skip();
			}
			return -1;
		} else if (match_and_paint(state, "mapkey", FLAG_KEYWORD, cmd_qualifier)) {
			if (charat() == ' ') skip(); else { paint(1, FLAG_ERROR); return -1; }
			for (struct mode_names * m = mode_names; m->name; ++m) {
				if (match_and_paint(state, m->name, FLAG_TYPE, cmd_qualifier)) break;
			}
			if (charat() == ' ') skip(); else { paint(1, FLAG_ERROR); return -1; }
			while (charat() != ' ' && charat() != -1) skip(); /* key name */
			if (charat() == ' ') skip(); else { paint(1, FLAG_ERROR); return -1; }
			for (struct action_def * a = mappable_actions; a->name; ++a) {
				if (match_and_paint(state, a->name, FLAG_TYPE, cmd_qualifier)) goto _found;
			}
			for (struct bim_function ** f = user_functions; user_functions && *f; ++f) {
				if (match_and_paint(state, (*f)->command, FLAG_TYPE, cmd_qualifier)) goto _found;
			}
			match_and_paint(state, "none", FLAG_TYPE, cmd_qualifier);
	_found:
			if (charat() == -1) return -1;
			if (charat() == ' ') skip(); else { paint(1, FLAG_ERROR); return -1; }
			while (charat() != -1 && charat() != ' ') {
				if (!strchr("racnwmb",charat())) {
					paint(1, FLAG_ERROR);
				} else {
					skip();
				}
			}
			return -1;
		} else if (match_and_paint(state, "action", FLAG_KEYWORD, cmd_qualifier)) {
			while (charat() == ' ') skip();
			for (struct action_def * a = mappable_actions; a->name; ++a) {
				if (match_and_paint(state, a->name, FLAG_TYPE, cmd_qualifier)) return -1;
			}
		} else if (charat() == '%' && nextchar() == 's') {
			paint(1, FLAG_KEYWORD);
			return bimcmd_paint_replacement(state);
		} else if (charat() == 's' && !isalpha(nextchar())) {
			return bimcmd_paint_replacement(state);
		} else if (bimcmd_find_commands(state)) {
			return -1;
		} else if (charat() == '!') {
			paint(1, FLAG_NUMERAL);
			nest(syn_bash_calculate, 1);
		} else if (charat() == '`') {
			paint(1, FLAG_NUMERAL);
			nest(syn_py_calculate, 1);
		} else if (isdigit(charat()) || charat() == '-' || charat() == '+') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			return -1;
		}
	}
	return -1;
}

char * syn_bimcmd_ext[] = {".bimscript",".bimrc",NULL}; /* no files */

BIM_SYNTAX_COMPLETER(bimcmd) {
	for (struct command_def * c = regular_commands; regular_commands && c->name; ++c) {
		add_if_match(c->name,c->description);
	}
	add_if_match("function","Define a function");
	add_if_match("end","End a function definition");
	return 0;
}

BIM_SYNTAX_EXT(bimcmd, 1, cmd_qualifier)
