#include "bim-core.h"
#include "bim-syntax.h"

int syn_biminfo_calculate(struct syntax_state * state) {
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

char * syn_biminfo_ext[] = {".biminfo",NULL};

BIM_SYNTAX(biminfo, 0)
