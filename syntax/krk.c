#include "bim-core.h"
#include "bim-syntax.h"

int paint_krk_string(struct syntax_state * state, int type) {
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == type) {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == type) {
			paint(1, FLAG_STRING);
			return 0;
		} else if (charat() == '\\') {
			if (nextchar() == 'x') {
				paint(2, FLAG_ESCAPE);
				/* Why is my FLAG_ERROR not valid in rline? */
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
			} else if (nextchar() == 'u') {
				paint(2, FLAG_ESCAPE);
				/* Why is my FLAG_ERROR not valid in rline? */
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_DIFFMINUS);
			} else {
				paint(2, FLAG_ESCAPE);
			}
		} else {
			paint(1, FLAG_STRING);
		}
	}
	if (lastchar() == '\\') {
		return (type == '"') ? 3 : 4; /* continues */
	}
	return -1;
}

char * syn_krk_keywords[] = {
	"and","class","def","else","for","if","in","import","del",
	"let","not","or","return","while","try","except","raise",
	"continue","break","as","from","elif","lambda","with","is",
	"pass",
	NULL
};

char * syn_krk_types[] = {
	/* built-in functions */
	"self", "super", /* implicit in a class method */
	"len", "str", "int", "float", "dir", "repr", /* global functions from __builtins__ */
	"list","dict","range", /* builtin classes */
	"object","exception","isinstance","type","tuple",
	"print","set","any","all","bool","ord","chr","hex",
	NULL
};

char * syn_krk_special[] = {
	"True","False","None",
	NULL
};

char * syn_krk_exception[] = {
	"TypeError","ArgumentError","IndexError","KeyError","AttributeError",
	"NameError","ImportError","IOError","ValueError","KeyboardInterrupt",
	"ZeroDivisionError","SyntaxError","Exception",
	NULL
};

int paint_krk_numeral(struct syntax_state * state) {
	if (charat() == '0' && (nextchar() == 'x' || nextchar() == 'X')) {
		paint(2, FLAG_NUMERAL);
		while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && (nextchar() == 'o' || nextchar() == 'O')) {
		paint(2, FLAG_NUMERAL);
		while (charat() >= '0' && charat() <= '7') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && (nextchar() == 'b' || nextchar() == 'B')) {
		paint(2, FLAG_NUMERAL);
		while (charat() == '0' || charat() == '1') paint(1, FLAG_NUMERAL);
	} else {
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		if (charat() == '.' && isdigit(nextchar())) {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		}
	}
	return 0;
}

int paint_krk_triple_string(struct syntax_state * state, int type) {
	while (charat() != -1) {
		if (charat() == type) {
			paint(1, FLAG_STRING);
			if (charat() == type && nextchar() == type) {
				paint(2, FLAG_STRING);
				return 0;
			}
		} else if (charat() == '\\') {
			if (nextchar() == 'x') {
				paint(2, FLAG_ESCAPE);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
			} else if (nextchar() == 'u') {
				paint(2, FLAG_ESCAPE);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
				paint(1, isxdigit(charat()) ? FLAG_ESCAPE : FLAG_ERROR);
			} else {
				paint(2, FLAG_ESCAPE);
			}
		} else {
			paint(1, FLAG_STRING);
		}
	}
	return (type == '"') ? 1 : 2; /* continues */
}

int syn_krk_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '#') {
				paint_comment(state);
			} else if (charat() == '@') {
				paint(1, FLAG_TYPE);
				while (c_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
				return 0;
			} else if (charat() == '"' || charat() == '\'') {
				int type = charat();
				if (nextchar() == type && charrel(2) == type) {
					paint(3, FLAG_STRING);
					return paint_krk_triple_string(state, type);
				} else {
					paint(1, FLAG_STRING);
					return paint_krk_string(state, type);
				}
				return 0;
			} else if (find_keywords(state, syn_krk_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (lastchar() != '.' && find_keywords(state, syn_krk_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_krk_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_krk_exception, FLAG_PRAGMA, c_keyword_qualifier)) {
				return 0;
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_krk_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		case 1:
			return paint_krk_triple_string(state, '"');
		case 2:
			return paint_krk_triple_string(state, '\'');
		case 3:
			return paint_krk_string(state, '"');
		case 4:
			return paint_krk_string(state, '\'');
	}
	return -1;
}

char * syn_krk_ext[] = {".krk",NULL};

BIM_SYNTAX(krk, 1)
