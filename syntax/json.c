#include "bim-core.h"
#include "bim-syntax.h"

char * syn_json_keywords[] = {
	"true","false","null",
	NULL
};

int syn_json_calculate(struct syntax_state * state) {
	while (charat() != -1) {
		if (charat() == '"') {
			int backtrack = state->i;
			paint_simple_string(state);
			int backtrack_end = state->i;
			while (charat() == ' ') skip();
			if (charat() == ':') {
				/* This is dumb. */
				state->i = backtrack;
				paint(1, FLAG_ESCAPE);
				while (state->i < backtrack_end-1) {
					paint(1, FLAG_KEYWORD);
				}
				if (charat() == '"') {
					paint(1, FLAG_ESCAPE);
				}
			}
			return 0;
		} else if (charat() == '-' || isdigit(charat())) {
			if (charat() == '-') paint(1, FLAG_NUMERAL);
			if (charat() == '0') {
				paint(1, FLAG_NUMERAL);
			} else {
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
			if (charat() == '.') {
				paint(1, FLAG_NUMERAL);
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
			if (charat() == 'e' || charat() == 'E') {
				paint(1, FLAG_NUMERAL);
				if (charat() == '+' || charat() == '-') {
					paint(1, FLAG_NUMERAL);
				}
				while (isdigit(charat())) paint(1, FLAG_NUMERAL);
			}
		} else if (find_keywords(state,syn_json_keywords,FLAG_NUMERAL,c_keyword_qualifier)) {
			/* ... */
		} else {
			skip();
			return 0;
		}
	}
	return -1;
}

char * syn_json_ext[] = {".json",NULL}; // TODO other stuff that uses json

BIM_SYNTAX(json, 1)
