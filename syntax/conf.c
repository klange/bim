#include "bim-core.h"
#include "bim-syntax.h"

int syn_conf_calculate(struct syntax_state * state) {
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

char * syn_conf_ext[] = {".conf",".ini",".git/config",NULL};

BIM_SYNTAX(conf, 1)
