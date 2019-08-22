#include "bim-core.h"
#include "bim-syntax.h"

int syn_hosts_calculate(struct syntax_state * state) {
	if (state->i == 0) {
		if (charat() == '#') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
		} else {
			while (charat() != '\t' && charat() != ' ' && charat() != -1) paint(1, FLAG_NUMERAL);
			while (charat() != -1) paint(1, FLAG_TYPE);
		}
	}

	return -1;
}

char * syn_hosts_ext[] = {"hosts"};

BIM_SYNTAX(hosts, 1)

