#include "bim-core.h"
#include "bim-syntax.h"

int syn_dirent_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (charat() == '#') {
			while (charat() != -1) paint(1, FLAG_COMMENT);
		} else if (charat() == 'd') {
			paint(1, FLAG_COMMENT);
			while (charat() != -1) paint(1, FLAG_KEYWORD);
		} else if (charat() == 'f') {
			paint(1, FLAG_COMMENT);
			return -1;
		}
	}
	return -1;
}

char * syn_dirent_ext[] = {NULL};

BIM_SYNTAX(dirent, 1)
