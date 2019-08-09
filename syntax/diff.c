#include "bim-core.h"
#include "bim-syntax.h"

int syn_diff_calculate(struct syntax_state * state) {
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

char * syn_diff_ext[] = {".patch",".diff",NULL};

BIM_SYNTAX(diff, 1)
