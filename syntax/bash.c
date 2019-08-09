#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

char * syn_bash_keywords[] = {
	/* Actual bash keywords */
	"if","then","else","elif","fi","case","esac","for","coproc",
	"select","while","until","do","done","in","function","time",
	/* Other keywords */
	"exit","return","source","function","export","alias","complete","shopt","local","eval",
	/* Common Unix utilities */
	"echo","cd","pushd","popd","printf","sed","rm","mv",
	NULL
};

int bash_pop_state(int state) {
	int new_state = state / 100;
	return new_state * 10;
}

int bash_push_state(int state, int new) {
	return state * 10 + new;
}

int bash_paint_tick(struct syntax_state * state, int out_state) {
	int last = -1;
	while (charat() != -1) {
		if (last != '\\' && charat() == '\'') {
			paint(1, FLAG_STRING);
			return bash_pop_state(out_state);
		} else if (last == '\\') {
			paint(1, FLAG_STRING);
			last = -1;
		} else if (charat() != -1) {
			last = charat();
			paint(1, FLAG_STRING);
		}
	}
	return out_state;
}

int bash_paint_braced_variable(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '}') {
			paint(1, FLAG_NUMERAL);
			return 0;
		}
		paint(1, FLAG_NUMERAL);
	}
	return 0;
}

int bash_special_variable(int c) {
	return (c == '@' || c == '?');
}

int bash_paint_string(struct syntax_state * state, char terminator, int out_state, int color) {
	int last = -1;
	state->state = out_state;
	while (charat() != -1) {
		if (last != '\\' && charat() == terminator) {
			paint(1, color);
			return bash_pop_state(state->state);
		} else if (last == '\\') {
			paint(1, color);
			last = -1;
		} else if (terminator != '`' && charat() == '`') {
			paint(1, FLAG_ESCAPE);
			state->state = bash_paint_string(state,'`',bash_push_state(out_state, 20),FLAG_ESCAPE);
		} else if (terminator != ')' && charat() == '$' && nextchar() == '(') {
			paint(2, FLAG_TYPE);
			state->state = bash_paint_string(state,')',bash_push_state(out_state, 30),FLAG_TYPE);
		} else if (charat() == '$' && nextchar() == '{') {
			paint(2, FLAG_NUMERAL);
			bash_paint_braced_variable(state);
		} else if (charat() == '$') {
			paint(1, FLAG_NUMERAL);
			if (bash_special_variable(charat())) { paint(1, FLAG_NUMERAL); continue; }
			while (c_keyword_qualifier(charat())) paint(1, FLAG_NUMERAL);
		} else if (terminator != '"' && charat() == '"') {
			paint(1, FLAG_STRING);
			state->state = bash_paint_string(state,'"',bash_push_state(out_state, 40),FLAG_STRING);
		} else if (terminator != '"' && charat() == '\'') { /* No single quotes in regular quotes */
			paint(1, FLAG_STRING);
			state->state = bash_paint_tick(state, out_state);
		} else if (charat() != -1) {
			last = charat();
			paint(1, color);
		}
	}
	return state->state;
}

int syn_bash_calculate(struct syntax_state * state) {
	if (state->state < 1) {
		if (charat() == '#') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
			return -1;
		} else if (charat() == '\'') {
			paint(1, FLAG_STRING);
			return bash_paint_tick(state, 10);
		} else if (charat() == '`') {
			paint(1, FLAG_ESCAPE);
			return bash_paint_string(state,'`',20,FLAG_ESCAPE);
		} else if (charat() == '$' && nextchar() == '(') {
			paint(2, FLAG_TYPE);
			return bash_paint_string(state,')',30,FLAG_TYPE);
		} else if (charat() == '"') {
			paint(1, FLAG_STRING);
			return bash_paint_string(state,'"',40,FLAG_STRING);
		} else if (charat() == '$' && nextchar() == '{') {
			paint(2, FLAG_NUMERAL);
			bash_paint_braced_variable(state);
			return 0;
		} else if (charat() == '$') {
			paint(1, FLAG_NUMERAL);
			if (bash_special_variable(charat())) { paint(1, FLAG_NUMERAL); return 0; }
			while (c_keyword_qualifier(charat())) paint(1, FLAG_NUMERAL);
			return 0;
		} else if (find_keywords(state, syn_bash_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return 0;
		} else if (charat() == ';') {
			paint(1, FLAG_KEYWORD);
			return 0;
		} else if (c_keyword_qualifier(charat())) {
			for (int i = 0; charrel(i) != -1; ++i) {
				if (charrel(i) == ' ') break;
				if (charrel(i) == '=') {
					for (int j = 0; j < i; ++j) {
						paint(1, FLAG_TYPE);
					}
					skip(); /* equals sign */
					return 0;
				}
			}
			for (int i = 0; charrel(i) != -1; ++i) {
				if (charrel(i) == '(') {
					for (int j = 0; j < i; ++j) {
						paint(1, FLAG_TYPE);
					}
					return 0;
				}
				if (!c_keyword_qualifier(charrel(i)) && charrel(i) != '-' && charrel(i) != ' ') break;
			}
			skip();
			return 0;
		} else if (charat() != -1) {
			skip();
			return 0;
		}
	} else if (state->state < 10) {
		/*
		 * TODO: I have an idea of how to do up to `n` (here... 8?) heredocs
		 * by storing them in a static table and using the index into that table
		 * for the state, but it's iffy. It would work well in situations where
		 * someoen used the same heredoc repeatedly throughout their document.
		 */
	} else if (state->state >= 10) {
		/* Nested string states */
		while (charat() != -1) {
			int s = (state->state / 10) % 10;
			if (s == 1) {
				state->state = bash_paint_string(state,'\'',state->state,FLAG_STRING);
			} else if (s == 2) {
				state->state = bash_paint_string(state,'`',state->state,FLAG_ESCAPE);
			} else if (s == 3) {
				state->state = bash_paint_string(state,')',state->state,FLAG_TYPE);
			} else if (s == 4) {
				state->state = bash_paint_string(state,'"',state->state,FLAG_STRING);
			} else if (!s) {
				return -1;
			}
		}
		return state->state;
	}
	return -1;
}

char * syn_bash_ext[] = {
#ifndef __toaru__
	".sh",
#endif
	".bash",".bashrc",
	NULL
};

BIM_SYNTAX(bash, 0)
