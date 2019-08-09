#include "bim-core.h"
#include "bim-syntax.h"

int syn_gitcommit_calculate(struct syntax_state * state) {
	if (state->i == 0 && charat() == '#') {
		while (charat() != -1) paint(1, FLAG_COMMENT);
	} else if (state->line_no == 0) {
		/* First line is special */
		while (charat() != -1 && state->i < 50) paint(1, FLAG_KEYWORD);
		while (charat() != -1) paint(1, FLAG_DIFFMINUS);
	} else if (state->line_no == 1) {
		/* No text on second line */
		while (charat() != -1) paint(1, FLAG_DIFFMINUS);
	} else if (charat() != -1) {
		skip();
		return 0;
	}
	return -1;
}

char * syn_gitcommit_ext[] = {"COMMIT_EDITMSG", NULL};

char * syn_gitrebase_commands[] = {
	"p","r","e","s","f","x","d",
	"pick","reword","edit","squash","fixup",
	"exec","drop",
	NULL
};

int syn_gitrebase_calculate(struct syntax_state * state) {
	if (state->i == 0 && charat() == '#') {
		while (charat() != -1) paint(1, FLAG_COMMENT);
	} else if (state->i == 0 && find_keywords(state, syn_gitrebase_commands, FLAG_KEYWORD, c_keyword_qualifier)) {
		while (charat() == ' ') skip();
		while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
		return -1;
	}

	return -1;
}

char * syn_gitrebase_ext[] = {"git-rebase-todo",NULL};

BIM_SYNTAX(gitcommit, 1)
BIM_SYNTAX(gitrebase, 1)
