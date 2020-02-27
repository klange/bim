#include "bim-core.h"
#include "bim-syntax.h"

static char * syn_kotlin_keywords[] = {
	"as","as?","break","class","continue","do","else","false","for",
	"fun","if","in","!in","interface","is","!is","null","object",
	"package","return","super","this","throw","true","try","typealias",
	"typeof","val","var","when","while",
	"by","catch","constructor","delegate","dynamic","field","file",
	"finally","get","import","init","param","property","receiver",
	"set","setparam","where",
	"actual","abstract","annotation","companion","const",
	"crossinline","data","enum","expect","external","final",
	"infix","inner","internal","lateinit","noinline","open",
	"operator","out","override","private","protected","public",
	"reified","sealed","suspend","tailrec","vararg",
	"field","it","inline",
	NULL
};

static char * syn_kotlin_types[] = {
	"Byte","Short","Int","Long",
	"Float","Double",
	NULL
};

static char * syn_kotlin_at_comments[] = {
	"@author","@see","@since","@return","@throws",
	"@version","@exception","@deprecated",
	/* @param is special */
	NULL
};

static int kt_at_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_') || (c == '@');
}

static char * syn_kotlin_brace_comments[] = {
	"{@docRoot","{@inheritDoc","{@link","{@linkplain",
	"{@value","{@code","{@literal","{@serial",
	"{@serialData","{@serialField",
	NULL
};

static int kotlin_keyword_qualifier(int c) {
	return isalnum(c) || (c == '?') || (c == '!') || (c == '_');
}

static int kt_brace_keyword_qualifier(int c) {
	return isalnum(c) || (c == '{') || (c == '@') || (c == '_');
}

static int paint_kotlin_comment(struct syntax_state * state) {
	int last = -1;
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (charat() == '@') {
			if (!find_keywords(state, syn_kotlin_at_comments, FLAG_ESCAPE, kt_at_keyword_qualifier)) {
				if (match_and_paint(state, "@param", FLAG_ESCAPE, kt_at_keyword_qualifier)) {
					while (charat() == ' ') skip();
					while (c_keyword_qualifier(charat())) paint(1, FLAG_TYPE);
				} else {
					/* Paint the @ */
					paint(1, FLAG_COMMENT);
				}
			}
		} else if (charat() == '{') {
			/* see if this terminates */
			if (find_keywords(state, syn_kotlin_brace_comments, FLAG_ESCAPE, kt_brace_keyword_qualifier)) {
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

int syn_kotlin_calculate(struct syntax_state * state) {
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
				if (paint_kotlin_comment(state) == 1) return 1;
			} else if (find_keywords(state, syn_kotlin_keywords, FLAG_KEYWORD, kotlin_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_kotlin_types, FLAG_TYPE, c_keyword_qualifier)) {
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
			if (paint_kotlin_comment(state) == 1) return 1;
			return 0;
	}
	return -1;
}

char * syn_kotlin_ext[] = {".kt",NULL};

BIM_SYNTAX_COMPLETER(kotlin) {
	for (char ** keyword = syn_kotlin_keywords; *keyword; ++keyword) {
		add_if_match((*keyword),"(kotlin keyword)");
	}
	for (char ** keyword = syn_kotlin_types; *keyword; ++keyword) {
		add_if_match((*keyword),"(kotlin type)");
	}

	/* XXX Massive hack */
	if (env->col_no > 1 && env->lines[env->line_no-1]->text[env->col_no-2].flags == FLAG_COMMENT) {
		if (comp[0] == '@') {
			for (char ** keyword = syn_kotlin_at_comments; *keyword; ++keyword) {
				add_if_match((*keyword),"(javadoc annotation)");
			}
		} else if (comp[0] == '{') {
			for (char ** keyword = syn_kotlin_brace_comments; *keyword; ++keyword) {
				add_if_match((*keyword),"(javadoc annotation)");
			}
		}
	}

	return 0;
}

BIM_SYNTAX_EXT(kotlin, 1, kt_brace_keyword_qualifier)

