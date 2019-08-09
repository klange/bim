#include "bim-core.h"
#include "bim-syntax.h"

static struct syntax_definition * syn_c = NULL;
static struct syntax_definition * syn_py = NULL;
static struct syntax_definition * syn_java = NULL;
static struct syntax_definition * syn_json = NULL;
static struct syntax_definition * syn_xml = NULL;
static struct syntax_definition * syn_make = NULL;
static struct syntax_definition * syn_diff = NULL;
static struct syntax_definition * syn_rust = NULL;

static int _initialized = 0;

int syn_markdown_calculate(struct syntax_state * state) {
	if (!_initialized) {
		_initialized = 1;
		syn_c    = find_syntax_calculator("c");
		syn_py   = find_syntax_calculator("py");
		syn_java = find_syntax_calculator("java");
		syn_json = find_syntax_calculator("json");
		syn_xml  = find_syntax_calculator("xml");
		syn_make = find_syntax_calculator("make");
		syn_diff = find_syntax_calculator("diff");
		syn_rust = find_syntax_calculator("rust");
	}
	if (state->state < 1) {
		while (charat() != -1) {
			if (state->i == 0 && charat() == '#') {
				while (charat() == '#') paint(1, FLAG_KEYWORD);
				while (charat() != -1) paint(1, FLAG_BOLD);
				return -1;
			} else if (state->i == 0) {
				while (charat() == ' ') skip();
				if (charat() == '`' && nextchar() == '`' && charrel(2) == '`') {
					paint(3, FLAG_STRING);
					if (syn_c &&match_forward(state, "c")) {
						nest(syn_c->calculate, 100);
					} else if (syn_c && match_forward(state,"c++")) {
						nest(syn_c->calculate, 100);
					} else if (syn_py && (match_forward(state,"py") || match_forward(state,"python"))) {
						nest(syn_py->calculate, 200);
					} else if (syn_java && match_forward(state, "java")) {
						nest(syn_java->calculate, 300);
					} else if (syn_json && match_forward(state,"json")) {
						nest(syn_json->calculate, 400);
					} else if (syn_xml && match_forward(state,"xml")) {
						nest(syn_xml->calculate, 500);
					} else if (syn_xml && match_forward(state,"html")) {
						nest(syn_xml->calculate, 500); // TODO this will be a different highlighter later
					} else if (syn_make && match_forward(state,"make")) {
						nest(syn_make->calculate, 600);
					} else if (syn_diff && match_forward(state, "diff")) {
						nest(syn_diff->calculate, 700);
					} else if (syn_rust && match_forward(state, "rust")) {
						nest(syn_rust->calculate, 800); /* Keep this at the end for now */
					}
					return 1;
				}
			}
			if (charat() == '`') {
				paint(1, FLAG_STRING);
				while (charat() != -1) {
					if (charat() == '`') {
						paint(1, FLAG_STRING);
						return 0;
					}
					paint(1, FLAG_STRING);
				}
			} else if (charat() == '[') {
				skip();
				while (charat() != -1 && charat() != ']') {
					paint(1, FLAG_LINK);
				}
				if (charat() == ']') skip();
				if (charat() == '(') {
					skip();
					while (charat() != -1 && charat() != ')') {
						paint(1, FLAG_NUMERAL);
					}
				}
			} else {
				skip();
				return 0;
			}
		}
		return -1;
	} else if (state->state >= 1) {
		/* Continuing generic triple-` */
		if (state->i == 0) {
			/* Go backwards until we find the source ``` */
			int count = 0;
			for (int i = state->line_no; i > 0; i--) {
				if (env->lines[i]->istate < 1) {
					while (env->lines[i]->text[count].codepoint == ' ') {
						if (charrel(count) != ' ') goto _nope;
						count++;
					}
					break;
				}
			}
			if (charrel(count) == '`' && charrel(count+1) == '`' && charrel(count+2) == '`' && charrel(count+3) == -1) {
				paint(count+3,FLAG_STRING);
				return -1;
			}
		}
_nope:
		if (state->state == 1) {
			while (charat() != -1) paint(1, FLAG_STRING);
			return 1;
		} else if (state->state < 199) {
			nest(syn_c->calculate, 100);
		} else if (state->state < 299) {
			nest(syn_py->calculate, 200);
		} else if (state->state < 399) {
			nest(syn_java->calculate, 300);
		} else if (state->state < 499) {
			nest(syn_json->calculate, 400);
		} else if (state->state < 599) {
			nest(syn_xml->calculate, 500);
		} else if (state->state < 699) {
			nest(syn_make->calculate, 600);
		} else if (state->state < 799) {
			nest(syn_diff->calculate, 700);
		} else {
			nest(syn_rust->calculate, 800);
		}
	}
	return -1;
}

char * syn_markdown_ext[] = {".md",".markdown",NULL};

BIM_SYNTAX(markdown, 1)
