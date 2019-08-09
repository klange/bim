#include "bim-core.h"
#include "bim-syntax.h"

int make_command_qualifier(int c) {
	return isalnum(c) || c == '_' || c == '-' || c == '.';
}

char * syn_make_commands[] = {
	"define","endef","undefine","ifdef","ifndef","ifeq","ifneq","else","endif",
	"include","sinclude","override","export","unexport","private","vpath",
	"-include",
	NULL
};

char * syn_make_functions[] = {
	"subst","patsubst","findstring","filter","filter-out",
	"sort","word","words","wordlist","firstword","lastword",
	"dir","notdir","suffix","basename","addsuffix","addprefix",
	"join","wildcard","realpath","abspath","error","warning",
	"shell","origin","flavor","foreach","if","or","and",
	"call","eval","file","value",
	NULL
};

char * syn_make_special_targets[] = {
	"all", /* Not really special, but highlight it 'cause I feel like it. */
	".PHONY", ".SUFFIXES", ".DEFAULT", ".PRECIOUS", ".INTERMEDIATE",
	".SECONDARY", ".SECONDEXPANSION", ".DELETE_ON_ERROR", ".IGNORE",
	".LOW_RESOLUTION_TIME", ".SILENT", ".EXPORT_ALL_VARIABLES",
	".NOTPARALLEL", ".ONESHELL", ".POSIX",
	NULL
};

int make_close_paren(struct syntax_state * state) {
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

int make_close_brace(struct syntax_state * state) {
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

int make_variable_or_comment(struct syntax_state * state, int flag) {
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

int syn_make_calculate(struct syntax_state * state) {
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

char * syn_make_ext[] = {"Makefile","makefile","GNUmakefile",".mak",NULL};

BIM_SYNTAX(make, 0)
