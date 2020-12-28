#include "bim-core.h"
#include "bim-syntax.h"

char * syn_krk_keywords[] = {
	"and","class","def","else","export","for","if","in","import",
	"let","not","or","print","return","while",
	NULL
};

char * syn_krk_types[] = {
	/* built-in functions */
	"self", "super", /* implicit in a class method */
	"len", "str", /* global functions from __builtins__ */
	"list","dict","range", /* builtin classes */
	"object",
	NULL
};

char * syn_krk_special[] = {
	"True","False","None",
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
		if (charat() == '.') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		}
	}
	return 0;
}

int syn_krk_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '#') {
				paint_comment(state);
			} else if (charat() == '"') {
				paint_simple_string(state);
				return 0;
			} else if (find_keywords(state, syn_krk_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (lastchar() != '.' && find_keywords(state, syn_krk_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_krk_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_krk_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
	}
	return -1;
}

char * syn_krk_ext[] = {".krk",NULL};

BIM_SYNTAX(krk, 1)
