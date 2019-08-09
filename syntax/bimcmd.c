#include "bim-core.h"
#include "bim-syntax.h"

int cmd_qualifier(int c) { return c != -1 && c != ' '; }

extern int syn_bash_calculate(struct syntax_state * state);
extern int syn_py_calculate(struct syntax_state * state);

int syn_bimcmd_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (match_and_paint(state, "theme", FLAG_KEYWORD, cmd_qualifier) ||
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
		} else if (charat() == 's' && !isalpha(nextchar())) {
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
		} else if (find_keywords(state, bim_command_names, FLAG_KEYWORD, cmd_qualifier)) {
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

char * syn_bimcmd_ext[] = {NULL}; /* no files */

BIM_SYNTAX(bimcmd, 1)
