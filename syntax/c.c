#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

/**
 * Syntax definition for C
 */
char * syn_c_keywords[] = {
	"while","if","for","continue","return","break","switch","case","sizeof",
	"struct","union","typedef","do","default","else","goto",
	"alignas","alignof","offsetof","asm","__asm__",
	/* C++ stuff */
	"public","private","class","using","namespace","virtual","override","protected",
	"template","typename","static_cast","throw",
	NULL
};

char * syn_c_types[] = {
	"static","int","char","short","float","double","void","unsigned","volatile","const",
	"register","long","inline","restrict","enum","auto","extern","bool","complex",
	"uint8_t","uint16_t","uint32_t","uint64_t",
	"int8_t","int16_t","int32_t","int64_t","FILE",
	"ssize_t","size_t","uintptr_t","intptr_t","__volatile__",
	"constexpr",
	NULL
};

char * syn_c_special[] = {
	"NULL",
	"stdin","stdout","stderr",
	"STDIN_FILENO","STDOUT_FILENO","STDERR_FILENO",
	NULL
};

int c_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_');
}

/**
 * Paints a basic C-style quoted string.
 */
void paint_c_string(struct syntax_state * state) {
	/* Assumes you came in from a check of charat() == '"' */
	paint(1, FLAG_STRING);
	int last = -1;
	while (charat() != -1) {
		if (last != '\\' && charat() == '"') {
			paint(1, FLAG_STRING);
			return;
		} else if (charat() == '\\' && (nextchar() == '\\' || nextchar() == 'n' || nextchar() == 'r')) {
			paint(2, FLAG_ESCAPE);
			last = -1;
		} else if (charat() == '\\' && nextchar() >= '0' && nextchar() <= '7') {
			paint(2, FLAG_ESCAPE);
			if (charat() >= '0' && charat() <= '7') {
				paint(1, FLAG_ESCAPE);
				if (charat() >= '0' && charat() <= '7') {
					paint(1, FLAG_ESCAPE);
				}
			}
			last = -1;
		} else if (charat() == '%') {
			paint(1, FLAG_ESCAPE);
			if (charat() == '%') {
				paint(1, FLAG_ESCAPE);
			} else {
				while (charat() == '-' || charat() == '#' || charat() == '*' || charat() == '0' || charat() == '+') paint(1, FLAG_ESCAPE);
				while (isdigit(charat())) paint(1, FLAG_ESCAPE);
				if (charat() == '.') {
					paint(1, FLAG_ESCAPE);
					if (charat() == '*') paint(1, FLAG_ESCAPE);
					else while (isdigit(charat())) paint(1, FLAG_ESCAPE);
				}
				while (charat() == 'l' || charat() == 'z') paint(1, FLAG_ESCAPE);
				paint(1, FLAG_ESCAPE);
			}
		} else if (charat() == '\\' && nextchar() == 'x') {
			paint(2, FLAG_ESCAPE);
			while (isxdigit(charat())) paint(1, FLAG_ESCAPE);
		} else {
			last = charat();
			paint(1, FLAG_STRING);
		}
	}
}

/**
 * Paint a C character numeral. Can be arbitrarily large, so
 * it supports multibyte chars for things like defining weird
 * ASCII multibyte integer constants.
 */
void paint_c_char(struct syntax_state * state) {
	/* Assumes you came in from a check of charat() == '\'' */
	paint(1, FLAG_NUMERAL);
	int last = -1;
	while (charat() != -1) {
		if (last != '\\' && charat() == '\'') {
			paint(1, FLAG_NUMERAL);
			return;
		} else if (last == '\\' && charat() == '\\') {
			paint(1, FLAG_NUMERAL);
			last = -1;
		} else {
			last = charat();
			paint(1, FLAG_NUMERAL);
		}
	}
}

/**
 * Paint a classic C comment which continues until terminated.
 * Assumes you've already painted the starting / and *.
 */
int paint_c_comment(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (last == '*' && charat() == '/') {
			paint(1, FLAG_COMMENT);
			return 0;
		} else {
			last = charat();
			paint(1, FLAG_COMMENT);
		}
	}
	return 1;
}

/**
 * Paint a generic C pragma, eg. a #define statement.
 */
int paint_c_pragma(struct syntax_state * state) {
	while (state->i < state->line->actual) {
		if (charat() == '"') {
			/* Paint C string */
			paint_c_string(state);
		} else if (charat() == '\'') {
			paint_c_char(state);
		} else if (charat() == '\\' && state->i == state->line->actual - 1) {
			paint(1, FLAG_PRAGMA);
			return 2;
		} else if (find_keywords(state, syn_c_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			continue;
		} else if (find_keywords(state, syn_c_types, FLAG_TYPE, c_keyword_qualifier)) {
			continue;
		} else if (charat() == '/' && nextchar() == '/') {
			/* C++-style comments */
			paint_comment(state);
			return -1;
		} else if (charat() == '/' && nextchar() == '*') {
			/* C-style comments */
			if (paint_c_comment(state) == 1) return 3;
			continue;
		} else {
			paint(1, FLAG_PRAGMA);
		}
	}
	return 0;
}

/**
 * Paint integers and floating point values with some handling for suffixes.
 */
int paint_c_numeral(struct syntax_state * state) {
	if (charat() == '0' && (nextchar() == 'x' || nextchar() == 'X')) {
		paint(2, FLAG_NUMERAL);
		while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == '.') {
		paint(2, FLAG_NUMERAL);
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		if (charat() == 'f') paint(1, FLAG_NUMERAL);
		return 0;
	} else if (charat() == '0') {
		paint(1, FLAG_NUMERAL);
		while (charat() >= '0' && charat() <= '7') paint(1, FLAG_NUMERAL);
	} else {
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		if (charat() == '.') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			if (charat() == 'f') paint(1, FLAG_NUMERAL);
			return 0;
		}
	}
	while (charat() == 'u' || charat() == 'U' || charat() == 'l' || charat() == 'L') paint(1, FLAG_NUMERAL);
	return 0;
}

int syn_c_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '#') {
				/* Must be first thing on line, but can have spaces before */
				for (int i = 0; i < state->i; ++i) {
					if (state->line->text[i].codepoint != ' ' && state->line->text[i].codepoint != '\t') {
						skip();
						return 0;
					}
				}
				/* Handle preprocessor functions */
				paint(1, FLAG_PRAGMA);
				while (charat() == ' ') paint(1, FLAG_PRAGMA);
				if (match_and_paint(state, "include", FLAG_PRAGMA, c_keyword_qualifier)) {
					/* Put quotes around <includes> */
					while (charat() == ' ') paint(1, FLAG_PRAGMA);
					if (charat() == '<') {
						paint(1, FLAG_STRING);
						while (charat() != '>' && state->i < state->line->actual) {
							paint(1, FLAG_STRING);
						}
						if (charat() != -1) {
							paint(1, FLAG_STRING);
						}
					}
					/* (for "includes", normal pragma highlighting covers that. */
				} else if (match_and_paint(state, "if", FLAG_PRAGMA, c_keyword_qualifier)) {
					/* These are to prevent #if and #else from being highlighted as keywords */
				} else if (match_and_paint(state, "else", FLAG_PRAGMA, c_keyword_qualifier)) {
					/* ... */
				}
				return paint_c_pragma(state);
			} else if (charat() == '/' && nextchar() == '/') {
				/* C++-style comments */
				paint_comment(state);
			} else if (charat() == '/' && nextchar() == '*') {
				/* C-style comments */
				if (paint_c_comment(state) == 1) return 1;
				return 0;
			} else if (find_keywords(state, syn_c_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_c_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_c_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\"') {
				paint_c_string(state);
				return 0;
			} else if (charat() == '\'') {
				paint_c_char(state);
				return 0;
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_c_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		case 1:
			/* In a block comment */
			if (paint_c_comment(state) == 1) return 1;
			return 0;
		case 2:
			/* In an unclosed preprocessor statement */
			return paint_c_pragma(state);
		case 3:
			/* In a block comment within an unclosed preprocessor statement */
			if (paint_c_comment(state) == 1) return 3;
			return paint_c_pragma(state);
	}
	return -1;
}

char * syn_c_ext[] = {".c",".h",".cpp",".hpp",".c++",".h++",".cc",".hh",NULL};

BIM_SYNTAX(c, 0)
