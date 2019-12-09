/**
 * NROFF highlighter
 */
#include "bim-core.h"
#include "bim-syntax.h"

int syn_man_calculate(struct syntax_state * state) {
	while (charat() != -1) {
		if (state->i == 0 && charat() == '.') {
			if (nextchar() == 'S' && charrel(2) == 'H' && charrel(3) == ' ') {
				paint(3, FLAG_KEYWORD);
				while (charat() != -1) paint(1, FLAG_STRING);
			} else if (nextchar() == 'B' && charrel(2) == ' ') {
				paint(2, FLAG_KEYWORD);
				while (charat() != -1) paint(1, FLAG_BOLD);
			} else if (isalpha(nextchar())) {
				paint(1, FLAG_KEYWORD);
				while (charat() != -1 && isalpha(charat())) paint(1, FLAG_KEYWORD);
			} else if (nextchar() == '\\' && charrel(2) == '"') {
				while (charat() != -1) paint(1, FLAG_COMMENT);
			} else {
				skip();
			}
		} else if (charat() == '\\') {
			if (nextchar() == 'f') {
				paint(2, FLAG_NUMERAL);
				paint(1, FLAG_PRAGMA);
			} else {
				paint(2, FLAG_ESCAPE);
			}
		} else {
			skip();
		}
	}
	return -1;
}

/* Yes this is dumb. No, I don't care. */
char * syn_man_ext[] = {".1",".2",".3",".4",".5",".6",".7",".8",NULL};

BIM_SYNTAX(man, 0)
