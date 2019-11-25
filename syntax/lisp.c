#include "bim-core.h"
#include "bim-syntax.h"

int lisp_paren_flags[] = {
	FLAG_STRING, FLAG_TYPE, FLAG_PRAGMA, FLAG_KEYWORD,
};

int lisp_paren_flag_from_state(int i) {
	return lisp_paren_flags[i % (sizeof(lisp_paren_flags) / sizeof(*lisp_paren_flags))];
}

int syn_lisp_calculate(struct syntax_state * state) {

	if (state->state == -1) state->state = 0;

	while (charat() != -1) {
		if (charat() == ';') {
			while (charat() != -1) paint(1, FLAG_COMMENT);
		} else if (charat() == '(') {
			paint(1, lisp_paren_flag_from_state(state->state));
			state->state++;

			while (charat() != ' ' && charat() != -1 && charat() != '(' && charat() != ')') {
				paint(1, FLAG_KEYWORD);
			}
		} else if (charat() == ')') {
			if (state->state == 0) {
				paint(1, FLAG_ERROR);
			} else {
				state->state--;
				paint(1, lisp_paren_flag_from_state(state->state));
			}
		} else if (charat() == ':') {
			while (charat() != ' ' && charat() != -1 && charat() != '(' && charat() != ')') {
				paint(1, FLAG_PRAGMA);
			}
		} else if (charat() == '"') {
			paint_simple_string(state);
		} else if (charat() != -1) {
			skip();
		}
	}

	if (state->state == 0) return -1;
	if (charat() == -1) return state->state;
	return -1; /* Not sure what happened */
}

char * syn_lisp_ext[] = {
	".lisp",".lsp",".cl",
	NULL
};

BIM_SYNTAX(lisp, 0)
