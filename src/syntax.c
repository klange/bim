#define _XOPEN_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "syntax.h"
#include "buffer.h"
#include "themes.h"
#include "line.h"
#include "util.h"
#include "display.h"
#include "config.h"

/**
 * Convert syntax highlighting flag to color code
 */
const char * flag_to_color(int _flag) {
	int flag = _flag & 0xF;
	switch (flag) {
		case FLAG_KEYWORD:
			return COLOR_KEYWORD;
		case FLAG_STRING:
			return COLOR_STRING;
		case FLAG_COMMENT:
			return COLOR_COMMENT;
		case FLAG_TYPE:
			return COLOR_TYPE;
		case FLAG_NUMERAL:
			return COLOR_NUMERAL;
		case FLAG_PRAGMA:
			return COLOR_PRAGMA;
		case FLAG_DIFFPLUS:
			return COLOR_GREEN;
		case FLAG_DIFFMINUS:
			return COLOR_RED;
		case FLAG_SELECT:
			return COLOR_FG;
		case FLAG_BOLD:
			return COLOR_BOLD;
		case FLAG_LINK:
			return COLOR_LINK;
		case FLAG_ESCAPE:
			return COLOR_ESCAPE;
		default:
			return COLOR_FG;
	}
}

/**
 * Find keywords from a list and paint them, assuming they aren't in the middle of other words.
 * Returns 1 if a keyword from the last was found, otherwise 0.
 */
static int find_keywords(struct syntax_state * state, char ** keywords, int flag, int (*keyword_qualifier)(int c)) {
	if (keyword_qualifier(lastchar())) return 0;
	if (!keyword_qualifier(charat())) return 0;
	for (char ** keyword = keywords; *keyword; ++keyword) {
		int d = 0;
		while (state->i + d < state->line->actual && state->line->text[state->i+d].codepoint == (*keyword)[d]) d++;
		if ((*keyword)[d] == '\0' && (state->i + d >= state->line->actual || !keyword_qualifier(state->line->text[state->i+d].codepoint))) {
			paint((int)strlen(*keyword), flag);
			return 1;
		}
	}

	return 0;
}

/**
 * Match and paint a single keyword. Returns 1 if the keyword was matched and 0 otherwise,
 * so it can be used for prefix checking for things that need further special handling.
 */
static int match_and_paint(struct syntax_state * state, const char * keyword, int flag, int (*keyword_qualifier)(int c)) {
	if (keyword_qualifier(lastchar())) return 0;
	if (!keyword_qualifier(charat())) return 0;
	int i = state->i;
	int slen = 0;
	while (i < state->line->actual || *keyword == '\0') {
		if (*keyword == '\0' && (i >= state->line->actual || !keyword_qualifier(state->line->text[i].codepoint))) {
			for (int j = 0; j < slen; ++j) {
				paint(1, flag);
			}
			return 1;
		}
		if (*keyword != state->line->text[i].codepoint) return 0;

		i++;
		keyword++;
		slen++;
	}
	return 0;
}

/**
 * Syntax definition for C
 */
static char * syn_c_keywords[] = {
	"while","if","for","continue","return","break","switch","case","sizeof",
	"struct","union","typedef","do","default","else","goto",
	"alignas","alignof","offsetof","asm","__asm__",
	/* C++ stuff */
	"public","private","class","using","namespace","virtual","override","protected",
	"template","typename","static_cast","throw",
	NULL
};

static char * syn_c_types[] = {
	"static","int","char","short","float","double","void","unsigned","volatile","const",
	"register","long","inline","restrict","enum","auto","extern","bool","complex",
	"uint8_t","uint16_t","uint32_t","uint64_t",
	"int8_t","int16_t","int32_t","int64_t","FILE",
	"ssize_t","size_t","uintptr_t","intptr_t","__volatile__",
	"constexpr",
	NULL
};

static char * syn_c_special[] = {
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
static void paint_c_string(struct syntax_state * state) {
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

static void paint_simple_string(struct syntax_state * state) {
	/* Assumes you came in from a check of charat() == '"' */
	paint(1, FLAG_STRING);
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == '"') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '"') {
			paint(1, FLAG_STRING);
			return;
		} else if (charat() == '\\') {
			paint(2, FLAG_ESCAPE);
		} else {
			paint(1, FLAG_STRING);
		}
	}
}

/**
 * Paint a C character numeral. Can be arbitrarily large, so
 * it supports multibyte chars for things like defining weird
 * ASCII multibyte integer constants.
 */
static void paint_c_char(struct syntax_state * state) {
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
 * These words can appear in comments and should be highlighted.
 * Since there are a lot of comment highlighters, let's break them out.
 */
static int common_comment_buzzwords(struct syntax_state * state) {
	if (match_and_paint(state, "TODO", FLAG_NOTICE, c_keyword_qualifier)) { return 1; }
	else if (match_and_paint(state, "XXX", FLAG_NOTICE, c_keyword_qualifier)) { return 1; }
	else if (match_and_paint(state, "FIXME", FLAG_ERROR, c_keyword_qualifier)) { return 1; }
	return 0;
}

/**
 * Paint a comment until end of line, assumes this comment can not continue.
 * (Some languages have comments that can continue with a \ - don't use this!)
 * Assumes you've already painted your comment start characters.
 */
static int paint_comment(struct syntax_state * state) {
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else { paint(1, FLAG_COMMENT); }
	}
	return -1;
}

/**
 * Paint a classic C comment which continues until terminated.
 * Assumes you've already painted the starting / and *.
 */
static int paint_c_comment(struct syntax_state * state) {
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
static int paint_c_pragma(struct syntax_state * state) {
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
static int paint_c_numeral(struct syntax_state * state) {
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

static int syn_c_calculate(struct syntax_state * state) {
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

static char * c_ext[] = {".c",".h",".cpp",".hpp",".c++",".h++",".cc",".hh",NULL};

static char * syn_py_keywords[] = {
	"class","def","return","del","if","else","elif","for","while","continue",
	"break","assert","as","and","or","except","finally","from","global",
	"import","in","is","lambda","with","nonlocal","not","pass","raise","try","yield",
	NULL
};

static char * syn_py_types[] = {
	/* built-in functions */
	"abs","all","any","ascii","bin","bool","breakpoint","bytes",
	"bytearray","callable","compile","complex","delattr","chr",
	"dict","dir","divmod","enumerate","eval","exec","filter","float",
	"format","frozenset","getattr","globals","hasattr","hash","help",
	"hex","id","input","int","isinstance","issubclass","iter","len",
	"list","locals","map","max","memoryview","min","next","object",
	"oct","open","ord","pow","print","property","range","repr","reverse",
	"round","set","setattr","slice","sorted","staticmethod","str","sum",
	"super","tuple","type","vars","zip",
	NULL
};

static char * syn_py_special[] = {
	"True","False","None",
	NULL
};

static int paint_py_triple_double(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '"') {
			paint(1, FLAG_STRING);
			if (charat() == '"' && nextchar() == '"') {
				paint(2, FLAG_STRING);
				return 0;
			}
		} else {
			paint(1, FLAG_STRING);
		}
	}
	return 1; /* continues */
}

static int paint_py_triple_single(struct syntax_state * state) {
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

static int paint_py_single_string(struct syntax_state * state) {
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

static int paint_py_numeral(struct syntax_state * state) {
	if (charat() == '0' && (nextchar() == 'x' || nextchar() == 'X')) {
		paint(2, FLAG_NUMERAL);
		while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == '.') {
		paint(2, FLAG_NUMERAL);
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		if ((charat() == '+' || charat() == '-') && (nextchar() == 'e' || nextchar() == 'E')) {
			paint(2, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		} else if (charat() == 'e' || charat() == 'E') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		}
		if (charat() == 'j') paint(1, FLAG_NUMERAL);
		return 0;
	} else {
		while (isdigit(charat())) paint(1, FLAG_NUMERAL);
		if (charat() == '.') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			if ((charat() == '+' || charat() == '-') && (nextchar() == 'e' || nextchar() == 'E')) {
				paint(2, FLAG_NUMERAL);
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			} else if (charat() == 'e' || charat() == 'E') {
				paint(1, FLAG_NUMERAL);
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
			if (charat() == 'j') paint(1, FLAG_NUMERAL);
			return 0;
		}
		if (charat() == 'j') paint(1, FLAG_NUMERAL);
	}
	while (charat() == 'l' || charat() == 'L') paint(1, FLAG_NUMERAL);
	return 0;
}

static void paint_py_format_string(struct syntax_state * state) {
	paint(1, FLAG_STRING);
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == '"') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '"') {
			paint(1, FLAG_STRING);
			return;
		} else if (charat() == '\\') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '{') {
			paint(1, FLAG_NUMERAL);
			if (charat() == '}') {
				state->i--;
				paint(2, FLAG_ERROR); /* Can't do that. */
			} else {
				while (charat() != -1 && charat() != '}') {
					paint(1, FLAG_NUMERAL);
				}
				paint(1, FLAG_NUMERAL);
			}
		} else {
			paint(1, FLAG_STRING);
		}
	}
}

static int syn_py_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '#') {
				paint_comment(state);
			} else if (state->i == 0 && match_and_paint(state, "import", FLAG_PRAGMA, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '@') {
				paint(1, FLAG_PRAGMA);
				while (c_keyword_qualifier(charat())) paint(1, FLAG_PRAGMA);
				return 0;
			} else if (charat() == '"') {
				if (nextchar() == '"' && charrel(2) == '"') {
					paint(3, FLAG_STRING);
					return paint_py_triple_double(state);
				} else if (lastchar() == 'f') {
					/* I don't like backtracking like this, but it makes this parse easier */
					state->i--;
					paint(1,FLAG_TYPE);
					paint_py_format_string(state);
					return 0;
				} else {
					paint_simple_string(state);
					return 0;
				}
			} else if (find_keywords(state, syn_py_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (lastchar() != '.' && find_keywords(state, syn_py_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_py_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\'') {
				if (nextchar() == '\'' && charrel(2) == '\'') {
					paint(3, FLAG_STRING);
					return paint_py_triple_single(state);
				} else {
					return paint_py_single_string(state);
				}
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_py_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		case 1: /* multiline """ string */
			return paint_py_triple_double(state);
		case 2: /* multiline ''' string */
			return paint_py_triple_single(state);
	}
	return -1;
}

static char * py_ext[] = {".py",NULL};

static char * syn_java_keywords[] = {
	"assert","break","case","catch","class","continue",
	"default","do","else","enum","exports","extends","finally",
	"for","if","implements","instanceof","interface","module","native",
	"new","requires","return","throws",
	"strictfp","super","switch","synchronized","this","throw","try","while",
	NULL
};

static char * syn_java_types[] = {
	"var","boolean","void","short","long","int","double","float","enum","char",
	"private","protected","public","static","final","transient","volatile","abstract",
	NULL
};

static char * syn_java_special[] = {
	"true","false","import","package","null",
	NULL
};

static char * syn_java_at_comments[] = {
	"@author","@see","@since","@return","@throws",
	"@version","@exception","@deprecated",
	/* @param is special */
	NULL
};

static int at_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_') || (c == '@');
}

static char * syn_java_brace_comments[] = {
	"{@docRoot","{@inheritDoc","{@link","{@linkplain",
	"{@value","{@code","{@literal","{@serial",
	"{@serialData","{@serialField",
	NULL
};

static int brace_keyword_qualifier(int c) {
	return isalnum(c) || (c == '{') || (c == '@');
}

static int paint_java_comment(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (charat() == '@') {
			if (!find_keywords(state, syn_java_at_comments, FLAG_ESCAPE, at_keyword_qualifier)) {
				if (match_and_paint(state, "@param", FLAG_ESCAPE, at_keyword_qualifier)) {
					while (charat() == ' ') skip();
					while (c_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
				} else {
					/* Paint the @ */
					paint(1, FLAG_COMMENT);
				}
			}
		} else if (charat() == '{') {
			/* see if this terminates */
			if (find_keywords(state, syn_java_brace_comments, FLAG_ESCAPE, brace_keyword_qualifier)) {
				while (charat() != '}' && charat() != -1) {
					paint(1, FLAG_ESCAPE);
				}
				if (charat() == '}') paint(1, FLAG_ESCAPE);
			} else {
				paint(1, FLAG_COMMENT);
			}
		} else if (charat() == '<') {
			int is_tag = 0;
			for (int i = 1; charrel(i) != -1; ++i) {
				if (charrel(i) == '>') {
					is_tag = 1;
					break;
				}
				if (!isalnum(charrel(i)) && charrel(i) != '/') {
					is_tag = 0;
					break;
				}
			}
			if (is_tag) {
				paint(1, FLAG_TYPE);
				while (charat() != -1 && charat() != '>') {
					if (charat() == '/') paint(1, FLAG_TYPE);
					else paint(1, FLAG_KEYWORD);
				}
				if (charat() == '>') paint(1, FLAG_TYPE);
			} else {
				/* Paint the < */
				paint(1, FLAG_COMMENT);
			}
		} else if (last == '*' && charat() == '/') {
			paint(1, FLAG_COMMENT);
			return 0;
		} else {
			last = charat();
			paint(1, FLAG_COMMENT);
		}
	}
	return 1;
}

static int syn_java_calculate(struct syntax_state * state) {
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
				/* C-style comments; TODO: Needs special stuff for @author; <html>; etc. */
				if (paint_java_comment(state) == 1) return 1;
			} else if (find_keywords(state, syn_java_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_java_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_java_special, FLAG_NUMERAL, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\"') {
				paint_simple_string(state);
				return 0;
			} else if (charat() == '\'') {
				paint_c_char(state);
				return 0;
			} else if (charat() == '@') {
				paint(1, FLAG_PRAGMA);
				while (c_keyword_qualifier(charat())) paint(1, FLAG_PRAGMA);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		case 1:
			if (paint_java_comment(state) == 1) return 1;
			return 0;
	}
	return -1;
}

static char * java_ext[] = {".java",NULL};

static int syn_diff_calculate(struct syntax_state * state) {
	/* No states to worry about */
	if (state->i == 0) {
		int flag = 0;
		if (charat() == '+') {
			flag = FLAG_DIFFPLUS;
		} else if (charat() == '-') {
			flag = FLAG_DIFFMINUS;
		} else if (charat() == '@') {
			flag = FLAG_TYPE;
		} else if (charat() != ' ') {
			flag = FLAG_KEYWORD;
		} else {
			return -1;
		}
		while (charat() != -1) paint(1, flag);
	}
	return -1;
}

static char * diff_ext[] = {".patch",".diff",NULL};

static int syn_conf_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (charat() == ';') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
		} else if (charat() == '#') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
		} else if (charat() == '[') {
			paint(1, FLAG_KEYWORD);
			while (charat() != ']' && charat() != -1) paint(1, FLAG_KEYWORD);
			if (charat() == ']') paint(1, FLAG_KEYWORD);
		} else {
			while (charat() != '=' && charat() != -1) paint(1, FLAG_TYPE);
		}
	}

	return -1;
}

static char * conf_ext[] = {".conf",".ini",".git/config",NULL};

static char * syn_rust_keywords[] = {
	"as","break","const","continue","crate","else","enum","extern",
	"false","fn","for","if","impl","in","let","loop","match","mod",
	"move","mut","pub","ref","return","Self","self","static","struct",
	"super","trait","true","type","unsafe","use","where","while",
	NULL,
};

static char * syn_rust_types[] = {
	"bool","char","str",
	"i8","i16","i32","i64",
	"u8","u16","u32","u64",
	"isize","usize",
	"f32","f64",
	NULL,
};

static int paint_rs_comment(struct syntax_state * state) {
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (charat() == '*' && nextchar() == '/') {
			paint(2, FLAG_COMMENT);
			state->state--;
			if (state->state == 0) return 0;
		} else if (charat() == '/' && nextchar() == '*') {
			state->state++;
			paint(2, FLAG_COMMENT);
		} else {
			paint(1, FLAG_COMMENT);
		}
	}
	return state->state;
}

static int paint_rust_numeral(struct syntax_state * state) {
	if (charat() == '0' && nextchar() == 'b') {
		paint(2, FLAG_NUMERAL);
		while (charat() == '0' || charat() == '1' || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == 'o') {
		paint(2, FLAG_NUMERAL);
		while ((charat() >= '0' && charat() <= '7') || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == 'x') {
		paint(2, FLAG_NUMERAL);
		while (isxdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == '.') {
		paint(2, FLAG_NUMERAL);
		while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
	} else {
		while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
		if (charat() == '.') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
		}
	}
	return 0;
}

static int syn_rust_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '/' && nextchar() == '/') {
				/* C++-style comments */
				paint_comment(state);
			} else if (charat() == '/' && nextchar() == '*') {
				paint(2, FLAG_COMMENT);
				state->state = 1;
				return paint_rs_comment(state);
			} else if (find_keywords(state, syn_rust_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_rust_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\"') {
				paint_simple_string(state);
				return 0;
			} else if (charat() == '\'') {
				paint_c_char(state);
				return 0;
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_rust_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		default: /* Nested comments */
			return paint_rs_comment(state);
	}

	return -1;
}

static char * rust_ext[] = {".rs",NULL};

static char * syn_bimrc_keywords[] = {
	"history","padding","hlparen","hlcurrent","splitpercent",
	"shiftscrolling","scrollamount","git","colorgutter","relativenumber",
	NULL
};

static int syn_bimrc_calculate(struct syntax_state * state) {
	/* No states */
	if (state->i == 0) {
		if (charat() == '#') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
		} else if (match_and_paint(state, "theme", FLAG_KEYWORD, c_keyword_qualifier)) {
			if (charat() == '=') {
				skip();
				for (struct theme_def * s = themes; s->name; ++s) {
					if (match_and_paint(state, s->name, FLAG_TYPE, c_keyword_qualifier)) break;
				}
			}
		} else if (find_keywords(state, syn_bimrc_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return -1;
		}
	}
	return -1;
}

static char * bimrc_ext[] = {".bimrc",NULL};

static int syn_biminfo_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (charat() == '#') {
			while (charat() != -1) paint(1, FLAG_COMMENT);
		} else if (charat() == '>') {
			paint(1, FLAG_KEYWORD);
			while (charat() != ' ') paint(1, FLAG_TYPE);
			skip();
			while (charat() != -1) paint(1, FLAG_NUMERAL);
		} else {
			while (charat() != -1) paint(1, FLAG_ERROR);
		}
	}
	return -1;
}

static char * biminfo_ext[] = {".biminfo",NULL};

static int syn_gitcommit_calculate(struct syntax_state * state) {
	if (state->i == 0 && charat() == '#') {
		while (charat() != -1) paint(1, FLAG_COMMENT);
	} else if (state->line_no == 0) {
		/* First line is special */
		while (charat() != -1 && state->i < 50) paint(1, FLAG_KEYWORD);
		while (charat() != -1) paint(1, FLAG_DIFFMINUS);
	} else if (state->line_no == 1) {
		/* No text on second line */
		while (charat() != -1) paint(1, FLAG_DIFFMINUS);
	} else if (charat() != -1) {
		skip();
		return 0;
	}
	return -1;
}

static char * gitcommit_ext[] = {"COMMIT_EDITMSG", NULL};

static char * syn_gitrebase_commands[] = {
	"p","r","e","s","f","x","d",
	"pick","reword","edit","squash","fixup",
	"exec","drop",
	NULL
};

static int syn_gitrebase_calculate(struct syntax_state * state) {
	if (state->i == 0 && charat() == '#') {
		while (charat() != -1) paint(1, FLAG_COMMENT);
	} else if (state->i == 0 && find_keywords(state, syn_gitrebase_commands, FLAG_KEYWORD, c_keyword_qualifier)) {
		while (charat() == ' ') skip();
		while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
		return -1;
	}

	return -1;
}

static char * gitrebase_ext[] = {"git-rebase-todo",NULL};

static int make_command_qualifier(int c) {
	return isalnum(c) || c == '_' || c == '-' || c == '.';
}

static char * syn_make_commands[] = {
	"define","endef","undefine","ifdef","ifndef","ifeq","ifneq","else","endif",
	"include","sinclude","override","export","unexport","private","vpath",
	"-include",
	NULL
};


static char * syn_make_functions[] = {
	"subst","patsubst","findstring","filter","filter-out",
	"sort","word","words","wordlist","firstword","lastword",
	"dir","notdir","suffix","basename","addsuffix","addprefix",
	"join","wildcard","realpath","abspath","error","warning",
	"shell","origin","flavor","foreach","if","or","and",
	"call","eval","file","value",
	NULL
};

static char * syn_make_special_targets[] = {
	"all", /* Not really special, but highlight it 'cause I feel like it. */
	".PHONY", ".SUFFIXES", ".DEFAULT", ".PRECIOUS", ".INTERMEDIATE",
	".SECONDARY", ".SECONDEXPANSION", ".DELETE_ON_ERROR", ".IGNORE",
	".LOW_RESOLUTION_TIME", ".SILENT", ".EXPORT_ALL_VARIABLES",
	".NOTPARALLEL", ".ONESHELL", ".POSIX",
	NULL
};

static int make_close_paren(struct syntax_state * state) {
	paint(2, FLAG_TYPE);
	find_keywords(state, syn_make_functions, FLAG_KEYWORD, c_keyword_qualifier);
	int i = 1;
	while (charat() != -1) {
		if (charat() == '(') {
			i++;
		} else if (charat() == ')') {
			i--;
			if (i == 0) {
				paint(1,FLAG_TYPE);
				return 0;
			}
		} else if (charat() == '"') {
			paint_simple_string(state);
		}
		paint(1,FLAG_TYPE);
	}
	return 0;
}

static int make_close_brace(struct syntax_state * state) {
	paint(2, FLAG_TYPE);
	while (charat() != -1) {
		if (charat() == '}') {
			paint(1, FLAG_TYPE);
			return 0;
		}
		paint(1, FLAG_TYPE);
	}
	return 0;
}

static int make_variable_or_comment(struct syntax_state * state, int flag) {
	while (charat() != -1) {
		if (charat() == '$') {
			switch (nextchar()) {
				case '(':
					make_close_paren(state);
					break;
				case '{':
					make_close_brace(state);
					break;
				default:
					paint(2, FLAG_TYPE);
					break;
			}
		} else if (charat() == '#') {
			while (charat() != -1) paint(1, FLAG_COMMENT);
		} else {
			paint(1, flag);
		}
	}
	return 0;
}

static int syn_make_calculate(struct syntax_state * state) {
	if (state->i == 0 && charat() == '\t') {
		make_variable_or_comment(state, FLAG_NUMERAL);
	} else {
		while (charat() == ' ') { skip(); }
		/* Peek forward to see if this is a rule or a variable */
		int whatisit = 0;
		for (int i = 0; charrel(i) != -1; ++i) {
			if (charrel(i) == ':' && charrel(i+1) != '=') {
				whatisit = 1;
				break;
			} else if (charrel(i) == '=') {
				whatisit = 2;
				break;
			} else if (charrel(i) == '#') {
				break;
			}
		}
		if (!whatisit) {
			/* Check for functions */
			while (charat() != -1) {
				if (charat() == '#') {
					while (charat() != -1) {
						if (common_comment_buzzwords(state)) continue;
						else paint(1, FLAG_COMMENT);
					}
				} else if (find_keywords(state, syn_make_commands, FLAG_KEYWORD, make_command_qualifier)) {
					continue;
				} else if (charat() == '$') {
					make_variable_or_comment(state, FLAG_NONE);
				} else {
					skip();
				}
			}
		} else if (whatisit == 1) {
			/* It's a rule */
			while (charat() != -1) {
				if (charat() == '#') {
					while (charat() != -1) {
						if (common_comment_buzzwords(state)) continue;
						else paint(1, FLAG_COMMENT);
					}
				} else if (charat() == ':') {
					paint(1, FLAG_TYPE);
					make_variable_or_comment(state, FLAG_NONE);
				} else if (find_keywords(state, syn_make_special_targets, FLAG_KEYWORD, make_command_qualifier)) {
						continue;
				} else {
					paint(1, FLAG_TYPE);
				}
			}
		} else if (whatisit == 2) {
			/* It's a variable definition */
			match_and_paint(state, "export", FLAG_KEYWORD, c_keyword_qualifier);
			while (charat() != -1 && charat() != '+' && charat() != '=' && charat() != ':' && charat() != '?') {
				paint(1, FLAG_TYPE);
			}
			while (charat() != -1 && charat() != '=') skip();
			/* Highlight variable expansions */
			make_variable_or_comment(state, FLAG_NONE);
		}
	}
	return -1;
}

static char * make_ext[] = {"Makefile","makefile","GNUmakefile",".mak",NULL};

#define nest(lang, low) \
	do { \
		state->state = (state->state < 1 ? 0 : state->state - low); \
		do { state->state = lang(state); } while (state->state == 0); \
		if (state->state == -1) return low; \
		return state->state + low; \
	} while (0)

static int match_forward(struct syntax_state * state, char * c) {
	int i = 0;
	while (1) {
		if (charrel(i) == -1 && !*c) return 1;
		if (charrel(i) != *c) return 0;
		c++;
		i++;
	}
	return 0;
}

static int syn_json_calculate(struct syntax_state * state);
static int syn_xml_calculate(struct syntax_state * state);
static int syn_markdown_calculate(struct syntax_state * state) {
	if (state->state < 1) {
		while (charat() != -1) {
			if (state->i == 0 && charat() == '#') {
				while (charat() == '#') paint(1, FLAG_KEYWORD);
				while (charat() != -1) paint(1, FLAG_BOLD);
				return -1;
			} else if (state->i == 0) {
				while (charat() == ' ') skip();
				if (charat() == '`' && nextchar() == '`' && charrel(2) == '`') {
					paint(3, FLAG_STRING);
					if (match_forward(state, "c")) {
						nest(syn_c_calculate, 100);
					} else if (match_forward(state,"c++")) {
						nest(syn_c_calculate, 100);
					} else if (match_forward(state,"py") || match_forward(state,"python")) {
						nest(syn_py_calculate, 200);
					} else if (match_forward(state, "java")) {
						nest(syn_java_calculate, 300);
					} else if (match_forward(state,"json")) {
						nest(syn_json_calculate, 400);
					} else if (match_forward(state,"xml")) {
						nest(syn_xml_calculate, 500);
					} else if (match_forward(state,"html")) {
						nest(syn_xml_calculate, 500); // TODO this will be a different highlighter later
					} else if (match_forward(state,"make")) {
						nest(syn_make_calculate, 600);
					} else if (match_forward(state, "diff")) {
						nest(syn_diff_calculate, 700);
					} else if (match_forward(state, "rust")) {
						nest(syn_rust_calculate, 800); /* Keep this at the end for now */
					}
					return 1;
				}
			}
			if (charat() == '`') {
				paint(1, FLAG_STRING);
				while (charat() != -1) {
					if (charat() == '`') {
						paint(1, FLAG_STRING);
						return 0;
					}
					paint(1, FLAG_STRING);
				}
			} else if (charat() == '[') {
				skip();
				while (charat() != -1 && charat() != ']') {
					paint(1, FLAG_LINK);
				}
				if (charat() == ']') skip();
				if (charat() == '(') {
					skip();
					while (charat() != -1 && charat() != ')') {
						paint(1, FLAG_NUMERAL);
					}
				}
			} else {
				skip();
				return 0;
			}
		}
		return -1;
	} else if (state->state >= 1) {
		/* Continuing generic triple-` */
		if (state->i == 0) {
			/* Go backwards until we find the source ``` */
			int count = 0;
			for (int i = state->line_no; i > 0; i--) {
				if (env->lines[i]->istate < 1) {
					while (env->lines[i]->text[count].codepoint == ' ') {
						if (charrel(count) != ' ') goto _nope;
						count++;
					}
					break;
				}
			}
			if (charrel(count) == '`' && charrel(count+1) == '`' && charrel(count+2) == '`' && charrel(count+3) == -1) {
				paint(count+3,FLAG_STRING);
				return -1;
			}
		}
_nope:
		if (state->state == 1) {
			while (charat() != -1) paint(1, FLAG_STRING);
			return 1;
		} else if (state->state < 199) {
			nest(syn_c_calculate, 100);
		} else if (state->state < 299) {
			nest(syn_py_calculate, 200);
		} else if (state->state < 399) {
			nest(syn_java_calculate, 300);
		} else if (state->state < 499) {
			nest(syn_json_calculate, 400);
		} else if (state->state < 599) {
			nest(syn_xml_calculate, 500);
		} else if (state->state < 699) {
			nest(syn_make_calculate, 600);
		} else if (state->state < 799) {
			nest(syn_diff_calculate, 700);
		} else {
			nest(syn_rust_calculate, 800);
		}
	}
	return -1;
}

static char * markdown_ext[] = {".md",".markdown",NULL};
static char * syn_json_keywords[] = {
	"true","false","null",
	NULL
};

static int syn_json_calculate(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '"') {
			int backtrack = state->i;
			paint_simple_string(state);
			int backtrack_end = state->i;
			while (charat() == ' ') skip();
			if (charat() == ':') {
				/* This is dumb. */
				state->i = backtrack;
				paint(1, FLAG_ESCAPE);
				while (state->i < backtrack_end-1) {
					paint(1, FLAG_KEYWORD);
				}
				if (charat() == '"') {
					paint(1, FLAG_ESCAPE);
				}
			}
			return 0;
		} else if (charat() == '-' || isdigit(charat())) {
			if (charat() == '-') paint(1, FLAG_NUMERAL);
			if (charat() == '0') {
				paint(1, FLAG_NUMERAL);
			} else {
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
			if (charat() == '.') {
				paint(1, FLAG_NUMERAL);
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
			if (charat() == 'e' || charat() == 'E') {
				paint(1, FLAG_NUMERAL);
				if (charat() == '+' || charat() == '-') {
					paint(1, FLAG_NUMERAL);
				}
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
		} else if (find_keywords(state,syn_json_keywords,FLAG_NUMERAL,c_keyword_qualifier)) {
			/* ... */
		} else {
			skip();
			return 0;
		}
	}
	return -1;
}

static char * json_ext[] = {".json",NULL}; // TODO other stuff that uses json

static int syn_xml_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == -1) return -1;
			if (charat() != '<') {
				skip();
				return 0;
			}
			/* Opening brace */
			if (charat() == '<' && nextchar() == '!' && charrel(2) == '-' && charrel(3) == '-') {
				paint(4, FLAG_COMMENT);
				goto _comment;
			}
			paint(1, FLAG_TYPE);
			/* Fall through */
		case 1:
			/* State 1: We saw an opening brace. */
			while (charat() != -1) {
				if (charat() == '/') paint(1, FLAG_TYPE);
				if (charat() == '?') paint(1, FLAG_TYPE);
				if (charat() == ' ' || charat() == '\t') skip();
				if (isalnum(charat())) {
					while (isalnum(charat()) || charat() == '-') paint(1, FLAG_KEYWORD);
					if (charat() == -1) return 2;
					goto _in_tag;
				} else {
					paint(1, FLAG_TYPE);
				}
			}
			return -1;
_in_tag:
		case 2:
			while (charat() != -1) {
				if (charat() == '>') {
					paint(1, FLAG_TYPE);
					return 0;
				} else if (charat() == '"') {
					paint_simple_string(state);
					if (charat() == -1 && lastchar() != '"') {
						return 3;
					}
				} else {
					paint(1, FLAG_TYPE);
				}
			}
			return 2;
		case 3:
			/* In a string in tag */
			if (charat() == '"') {
				paint(1, FLAG_STRING);
				return 2;
			} else {
				paint_simple_string(state);
				if (charat() == -1 && lastchar() != '"') {
					return 3;
				}
			}
			break;
_comment:
		case 4:
			while (charat() != -1) {
				if (charat() == '-' && nextchar() == '-' && charrel(2) == '>') {
					paint(3, FLAG_COMMENT);
					return 0;
				} else {
					if (common_comment_buzzwords(state)) continue;
					else paint(1, FLAG_COMMENT);
				}
			}
			return 4;
	}
	return -1;
}

static char * xml_ext[] = {".xml",".htm",".html",NULL}; // TODO other stuff that uses xml (it's a lot!); FIXME htm/html are temporary; make dedicated SGML ones for this

static char * syn_proto_keywords[] = {
	"syntax","import","option","package","message","group","oneof",
	"optional","required","repeated","default","extend","extensions","to","max","reserved",
	"service","rpc","returns","stream",
	NULL
};

static char * syn_proto_types[] = {
	"int32","int64","uint32","uint64","sint32","sint64",
	"fixed32","fixed64","sfixed32","sfixed64",
	"float","double","bool","string","bytes",
	"enum",
	NULL
};

static char * syn_proto_special[] = {
	"true","false",
	NULL
};

static int syn_proto_calculate(struct syntax_state * state) {
	if (state->state < 1) {
		if (charat() == '/' && nextchar() == '/') {
			paint_comment(state);
		} else if (charat() == '/' && nextchar() == '*') {
			if (paint_c_comment(state) == 1) return 1;
			return 0;
		} else if (find_keywords(state, syn_proto_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, syn_proto_types, FLAG_TYPE, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, syn_proto_special, FLAG_NUMERAL, c_keyword_qualifier)) {
			return 0;
		} else if (charat() == '"') {
			paint_simple_string(state);
			return 0;
		} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
			paint_c_numeral(state);
			return 0;
		} else if (charat() != -1) {
			skip();
			return 0;
		}
		return -1;
	} else {
		if (paint_c_comment(state) == 1) return 1;
		return 0;
	}
}

static char * proto_ext[] = {".proto",NULL};

static int esh_variable_qualifier(int c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_');
}

static int paint_esh_variable(struct syntax_state * state) {
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

static int paint_esh_string(struct syntax_state * state) {
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

static int paint_esh_single_string(struct syntax_state * state) {
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

static int esh_keyword_qualifier(int c) {
	return (isalnum(c) || c == '?' || c == '_' || c == '-'); /* technically anything that isn't a space should qualify... */
}

static char * esh_keywords[] = {
	"cd","exit","export","help","history","if","empty?",
	"equals?","return","export-cmd","source","exec","not","while",
	"then","else","echo",
	NULL
};

static int syn_esh_calculate(struct syntax_state * state) {
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
static char * esh_ext[] = {
#ifdef __toaru__
	".sh",
#endif
	".eshrc",".yutanirc",
	NULL
};

static char * syn_bash_keywords[] = {
	/* Actual bash keywords */
	"if","then","else","elif","fi","case","esac","for","coproc",
	"select","while","until","do","done","in","function","time",
	/* Other keywords */
	"exit","return","source","function","export","alias","complete","shopt","local","eval",
	/* Common Unix utilities */
	"echo","cd","pushd","popd","printf","sed","rm","mv",
	NULL
};

static int bash_pop_state(int state) {
	int new_state = state / 100;
	return new_state * 10;
}

static int bash_push_state(int state, int new) {
	return state * 10 + new;
}

static int bash_paint_tick(struct syntax_state * state, int out_state) {
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

static int bash_paint_braced_variable(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '}') {
			paint(1, FLAG_NUMERAL);
			return 0;
		}
		paint(1, FLAG_NUMERAL);
	}
	return 0;
}

static int bash_special_variable(int c) {
	return (c == '@' || c == '?');
}

static int bash_paint_string(struct syntax_state * state, char terminator, int out_state, int color) {
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

static int syn_bash_calculate(struct syntax_state * state) {
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

static char * bash_ext[] = {
#ifndef __toaru__
	".sh",
#endif
	".bash",".bashrc",
	NULL
};

int syn_ctags_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (charat() == '!') {
			paint_comment(state);
			return -1;
		} else {
			while (charat() != -1 && charat() != '\t') paint(1, FLAG_TYPE);
			if (charat() == '\t') skip();
			while (charat() != -1 && charat() != '\t') paint(1, FLAG_NUMERAL);
			if (charat() == '\t') skip();
			while (charat() != -1 && !(charat() == ';' && nextchar() == '"')) paint(1, FLAG_KEYWORD);
			return -1;
		}
	}
	return -1;
}

static char * ctags_ext[] = { "tags", NULL };

struct syntax_definition syntaxes[] = {
	{"c",c_ext,syn_c_calculate,0},
	{"python",py_ext,syn_py_calculate,1},
	{"java",java_ext,syn_java_calculate,1},
	{"diff",diff_ext,syn_diff_calculate,0},
	{"conf",conf_ext,syn_conf_calculate,0},
	{"rust",rust_ext,syn_rust_calculate,1},
	{"bimrc",bimrc_ext,syn_bimrc_calculate,0},
	{"biminfo",biminfo_ext,syn_biminfo_calculate,0},
	{"gitcommit",gitcommit_ext,syn_gitcommit_calculate,0},
	{"gitrebase",gitrebase_ext,syn_gitrebase_calculate,0},
	{"make",make_ext,syn_make_calculate,0}, /* Definitely don't use spaces for Make */
	{"markdown",markdown_ext,syn_markdown_calculate,1},
	{"json",json_ext,syn_json_calculate,1},
	{"xml",xml_ext,syn_xml_calculate,1},
	{"protobuf",proto_ext,syn_proto_calculate,1},
	{"toarush",esh_ext,syn_esh_calculate,0},
	{"bash",bash_ext,syn_bash_calculate,0},
	{"ctags",ctags_ext,syn_ctags_calculate,0},
	{NULL,NULL,NULL,0},
};


/**
 * Calculate syntax highlighting for the given line.
 */
void recalculate_syntax(line_t * line, int line_no) {
	/* Clear syntax for this line first */
	for (int i = 0; i < line->actual; ++i) {
		line->text[i].flags = 0;
	}

	if (!env->syntax) {
		rehighlight_search(line);
		return;
	}

	/* Start from the line's stored in initial state */
	struct syntax_state state;
	state.line = line;
	state.line_no = line_no;
	state.state = line->istate;
	state.i = 0;

	while (1) {
		state.state = env->syntax->calculate(&state);

		if (state.state != 0) {
			rehighlight_search(line);
			if (line_no + 1 < env->line_count && env->lines[line_no+1]->istate != state.state) {
				env->lines[line_no+1]->istate = state.state;
				if (env->loading) return;
				recalculate_syntax(env->lines[line_no+1], line_no+1);
				rehighlight_search(env->lines[line_no+1]);
				if (line_no + 1 >= env->offset && line_no + 1 < env->offset + global_config.term_height - global_config.bottom_size - 1) {
					redraw_line(line_no + 1 - env->offset, line_no + 1);
				}
			}
			break;
		}
	}
}

/**
 * Recalculate tab widths.
 */
void recalculate_tabs(line_t * line) {
	if (env->loading) return;

	int j = 0;
	for (int i = 0; i < line->actual; ++i) {
		if (line->text[i].codepoint == '\t') {
			line->text[i].display_width = env->tabstop - (j % env->tabstop);
		}
		j += line->text[i].display_width;
	}
}

/**
 * Find a syntax highlighter for the given filename.
 */
struct syntax_definition * match_syntax(char * file) {
	for (struct syntax_definition * s = syntaxes; s->name; ++s) {
		for (char ** ext = s->ext; *ext; ++ext) {
			int i = strlen(file);
			int j = strlen(*ext);

			do {
				if (file[i] != (*ext)[j]) break;
				if (j == 0) return s;
				if (i == 0) break;
				i--;
				j--;
			} while (1);
		}
	}

	return NULL;
}

/**
 * Set the syntax configuration by the name of the syntax highlighter.
 */
void set_syntax_by_name(const char * name) {
	if (!strcmp(name,"none")) {
		for (int i = 0; i < env->line_count; ++i) {
			env->lines[i]->istate = 0;
			for (int j = 0; j < env->lines[i]->actual; ++j) {
				env->lines[i]->text[j].flags = 0;
			}
		}
		redraw_all();
		return;
	}
	for (struct syntax_definition * s = syntaxes; s->name; ++s) {
		if (!strcmp(name,s->name)) {
			env->syntax = s;
			for (int i = 0; i < env->line_count; ++i) {
				env->lines[i]->istate = 0;
			}
			for (int i = 0; i < env->line_count; ++i) {
				recalculate_syntax(env->lines[i],i);
			}
			redraw_all();
			return;
		}
	}
	render_error("unrecognized syntax type");
}


