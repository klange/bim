#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

int esh_variable_qualifier(int c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_');
}

int paint_esh_variable(struct syntax_state * state) {
	if (charat() == '{') {
		paint(1, FLAG_TYPE);
		while (charat() != '}') paint(1, FLAG_TYPE);
		if (charat() == '}') paint(1, FLAG_TYPE);
	} else {
		if (charat() == '?' || charat() == '$' || charat() == '#') {
			paint(1, FLAG_TYPE);
		} else {
			while (esh_variable_qualifier(charat())) paint(1, FLAG_TYPE);
		}
	}
	return 0;
}

int paint_esh_string(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (last != '\\' && charat() == '"') {
			paint(1, FLAG_STRING);
			return 0;
		} else if (charat() == '$') {
			paint(1, FLAG_TYPE);
			paint_esh_variable(state);
			last = -1;
		} else if (charat() != -1) {
			last = charat();
			paint(1, FLAG_STRING);
		}
	}
	return 2;
}

int paint_esh_single_string(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (last != '\\' && charat() == '\'') {
			paint(1, FLAG_STRING);
			return 0;
		} else if (charat() != -1) {
			last = charat();
			paint(1, FLAG_STRING);
		}
	}
	return 1;
}

int esh_keyword_qualifier(int c) {
	return (isalnum(c) || c == '?' || c == '_' || c == '-'); /* technically anything that isn't a space should qualify... */
}

char * esh_keywords[] = {
	"cd","exit","export","help","history","if","empty?",
	"equals?","return","export-cmd","source","exec","not","while",
	"then","else","echo",
	NULL
};

int syn_esh_calculate(struct syntax_state * state) {
	if (state->state == 1) {
		return paint_esh_single_string(state);
	} else if (state->state == 2) {
		return paint_esh_string(state);
	}
	if (charat() == '#') {
		while (charat() != -1) {
			if (common_comment_buzzwords(state)) continue;
			else paint(1, FLAG_COMMENT);
		}
		return -1;
	} else if (charat() == '$') {
		paint(1, FLAG_TYPE);
		paint_esh_variable(state);
		return 0;
	} else if (charat() == '\'') {
		paint(1, FLAG_STRING);
		return paint_esh_single_string(state);
	} else if (charat() == '"') {
		paint(1, FLAG_STRING);
		return paint_esh_string(state);
	} else if (match_and_paint(state, "export", FLAG_KEYWORD, esh_keyword_qualifier)) {
		while (charat() == ' ') skip();
		while (esh_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
		return 0;
	} else if (match_and_paint(state, "export-cmd", FLAG_KEYWORD, esh_keyword_qualifier)) {
		while (charat() == ' ') skip();
		while (esh_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
		return 0;
	} else if (find_keywords(state, esh_keywords, FLAG_KEYWORD, esh_keyword_qualifier)) {
		return 0;
	} else if (isdigit(charat())) {
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		return 0;
	} else if (charat() != -1) {
		skip();
		return 0;
	}
	return -1;
}

/* Only enable esh highlighting by default on ToaruOS */
char * syn_esh_ext[] = {
#ifdef __toaru__
	".sh",
#endif
	".eshrc",".yutanirc",
	NULL
};

BIM_SYNTAX(esh, 0)
