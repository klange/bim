#include "bim-core.h"
#include "bim-syntax.h"

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

char * syn_ctags_ext[] = { "tags", NULL };

BIM_SYNTAX(ctags, 0)
