#include "bim-core.h"
#include "bim-syntax.h"

static char * syn_js_keywords[] = {
	"abstract","arguments",
	"await","break","case","catch","class","const",
	"continue","debugger","default","delete","do","else","enum","eval",
	"export","extends","final","finally","for","function","goto",
	"if","implements","import","in","instanceof","interface","let","long",
	"native","new","package","private","protected","public","return",
	"static","super","switch","synchronized","this","throw","throws",
	"transienttrue","try","typeof","volatile","while","with","yield",
	NULL
};

static char * syn_js_types[] = {
	"int","float","double","short","var","void","byte","char","boolean",
	NULL
};

static char * syn_js_special[] = {
	"true","false","null",
	NULL
};

int syn_js_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_c_numeral(state);
				return 0;
			} else if (charat() == '/' && nextchar() == '/') {
				/* C++-style comments */
				paint_comment(state);
			} else if (charat() == '/' && nextchar() == '*') {
				if (paint_c_comment(state) == 1) return 1;
			} else if (find_keywords(state, syn_js_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_js_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_js_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\"') {
				paint_simple_string(state);
				return 0;
			} else if (charat() == '\'') {
				paint_c_char(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		case 1:
			if (paint_c_comment(state) == 1) return 1;
			return 0;
	}
	return -1;
}

char * syn_js_ext[] = {".js",NULL};

BIM_SYNTAX_COMPLETER(js) {
	for (char ** keyword = syn_js_keywords; *keyword; ++keyword) {
		add_if_match((*keyword),"(javascript keyword)");
	}
	for (char ** keyword = syn_js_types; *keyword; ++keyword) {
		add_if_match((*keyword),"(javascript type)");
	}

	return 0;
}

BIM_SYNTAX_EXT(js, 1, c_keyword_qualifier)
