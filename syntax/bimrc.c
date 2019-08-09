#include "bim-core.h"
#include "bim-syntax.h"

char * syn_bimrc_keywords[] = {
	"history","padding","hlparen","hlcurrent","splitpercent","numbers",
	"shiftscrolling","scrollamount","git","colorgutter","relativenumber",
	NULL
};

int syn_bimrc_calculate(struct syntax_state * state) {
	/* No states */
	if (state->i == 0) {
		if (charat() == '#') {
			while (charat() != -1) {
				if (common_comment_buzzwords(state)) continue;
				else paint(1, FLAG_COMMENT);
			}
		} else if (match_and_paint(state, "theme", FLAG_KEYWORD, c_keyword_qualifier)) {
			if (charat() == '=') {
				skip();
				for (struct theme_def * s = themes; themes && s->name; ++s) {
					if (match_and_paint(state, s->name, FLAG_TYPE, c_keyword_qualifier)) break;
				}
			}
		} else if (find_keywords(state, syn_bimrc_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return -1;
		}
	}
	return -1;
}

char * syn_bimrc_ext[] = {".bimrc",NULL};

BIM_SYNTAX(bimrc, 0)
