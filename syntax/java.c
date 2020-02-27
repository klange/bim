#include "bim-core.h"
#include "bim-syntax.h"

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

static int java_at_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_') || (c == '@');
}

static char * syn_java_brace_comments[] = {
	"{@docRoot","{@inheritDoc","{@link","{@linkplain",
	"{@value","{@code","{@literal","{@serial",
	"{@serialData","{@serialField",
	NULL
};

static int java_brace_keyword_qualifier(int c) {
	return isalnum(c) || (c == '{') || (c == '@') || (c == '_');
}

static int paint_java_comment(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (charat() == '@') {
			if (!find_keywords(state, syn_java_at_comments, FLAG_ESCAPE, java_at_keyword_qualifier)) {
				if (match_and_paint(state, "@param", FLAG_ESCAPE, java_at_keyword_qualifier)) {
					while (charat() == ' ') skip();
					while (c_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
				} else {
					/* Paint the @ */
					paint(1, FLAG_COMMENT);
				}
			}
		} else if (charat() == '{') {
			/* see if this terminates */
			if (find_keywords(state, syn_java_brace_comments, FLAG_ESCAPE, java_brace_keyword_qualifier)) {
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

int syn_java_calculate(struct syntax_state * state) {
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

char * syn_java_ext[] = {".java",NULL};

BIM_SYNTAX_COMPLETER(java) {
	for (char ** keyword = syn_java_keywords; *keyword; ++keyword) {
		add_if_match((*keyword),"(java keyword)");
	}
	for (char ** keyword = syn_java_types; *keyword; ++keyword) {
		add_if_match((*keyword),"(java type)");
	}

	/* XXX Massive hack */
	if (env->col_no > 1 && env->lines[env->line_no-1]->text[env->col_no-2].flags == FLAG_COMMENT) {
		if (comp[0] == '@') {
			for (char ** keyword = syn_java_at_comments; *keyword; ++keyword) {
				add_if_match((*keyword),"(javadoc annotation)");
			}
		} else if (comp[0] == '{') {
			for (char ** keyword = syn_java_brace_comments; *keyword; ++keyword) {
				add_if_match((*keyword),"(javadoc annotation)");
			}
		}
	}

	return 0;
}

BIM_SYNTAX_EXT(java, 1, java_brace_keyword_qualifier)
