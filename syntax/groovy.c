#include "bim-core.h"
#include "bim-syntax.h"

static char * groovy_keywords[] = {
	"as","assert","break","case",
	"catch","class","const","continue",
	"def","default","do","else","enum",
	"extends","finally","for",
	"goto","if","implements","import",
	"in","instanceof","interface","new",
	"package","return","super",
	"switch","throw","throws",
	"trait","try","while",
	NULL
};

static char * groovy_stuff[] = {
	"true","false","null","this",
	NULL
};

static char * groovy_primitives[] = {
	"byte","char","short","int","long","BigInteger",
	NULL
};

static int paint_single_string(struct syntax_state * state) {
	paint(1, FLAG_STRING);
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == '\'') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '\'') {
			paint(1, FLAG_STRING);
			return 0;
		} else if (charat() == '\\') {
			paint(2, FLAG_ESCAPE);
		} else {
			paint(1, FLAG_STRING);
		}
	}
	return 0;
}

static int paint_triple_single(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '\'') {
			paint(1, FLAG_STRING);
			if (charat() == '\'' && nextchar() == '\'') {
				paint(2, FLAG_STRING);
				return 0;
			}
		} else {
			paint(1, FLAG_STRING);
		}
	}
	return 2; /* continues */
}

int syn_groovy_calculate(struct syntax_state * state) {
	if (state->state <= 0) {
		if (state->line_no == 0 && state->i == 0 && charat() == '#') {
			paint_comment(state);
			return -1;
		} else if (charat() == '/' && nextchar() == '/') {
			/* C++-style comments */
			paint_comment(state);
			return -1;
		} else if (charat() == '/' && nextchar() == '*') {
			if (paint_c_comment(state) == 1) return 1;
		} else if (charat() == '"') {
			paint_simple_string(state);
			return 0;
		} else if (charat() == '\'') {
			paint_single_string(state);
			return 0;
		} else if (find_keywords(state, groovy_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, groovy_primitives, FLAG_TYPE, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, groovy_stuff, FLAG_NUMERAL, c_keyword_qualifier)) {
			return 0;
		} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
			paint_c_numeral(state);
			return 0;
		} else if (charat() != -1) {
			skip();
			return 0;
		}
		return -1;
	} else if (state->state == 1) {
		if (paint_c_comment(state) == 1) return 1;
		return 0;
	} else if (state->state == 2) {
		return paint_triple_single(state);
	}
	return -1;
}

char * syn_groovy_ext[] = {".groovy",".jenkinsfile",NULL};

BIM_SYNTAX(groovy, 1)
