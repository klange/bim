#include "bim-core.h"
#include "bim-syntax.h"

static struct syntax_definition * syn_xml = NULL;
static int _initialized = 0;

static int paint_single_string(struct syntax_state * state) {
	paint(1, FLAG_STRING);
	while (charat() != -1) {
		if (charat() == '\\' && nextchar() == '\'') {
			paint(2, FLAG_ESCAPE);
		} else if (charat() == '\'') {
			paint(1, FLAG_STRING);
			return 0;
		} else if (charat() == '\\') {
			paint(2, FLAG_ESCAPE);
		} else {
			paint(1, FLAG_STRING);
		}
	}
	return 0;
}

static char * soy_keywords[] = {
	"call","template","param","namespace","let","and","if","else","elseif",
	"switch","case","default","foreach","literal","sp","nil","lb","rb","in",
	NULL
};

static char * soy_functions[] = {
	"isNonnull","strContains","ceiling","floor","max","min","randomInt",
	"round","index","isFirst","isLast","length","augmentMap","keys",
	NULL
};

int soy_keyword_qualifier(int c) {
	return isalnum(c) || (c == '_') || (c == '.');
}

int syn_soy_calculate(struct syntax_state * state) {
	if (!_initialized) {
		_initialized = 1;
		syn_xml  = find_syntax_calculator("xml");
	}

	if (state->state > 0 && state->state <= 4) {
		return syn_xml ? syn_xml->calculate(state) : 0;
	} else if (state->state == 5) {
		if (paint_c_comment(state) == 1) return 5;
		return 0;
	} else {
		if (charat() == '{') {
			paint(1, FLAG_TYPE);
			while (charat() != -1 && charat() != '}') {
				if (find_keywords(state, soy_keywords, FLAG_KEYWORD, soy_keyword_qualifier)) {
					continue;
				} else if (find_keywords(state, soy_functions, FLAG_TYPE, soy_keyword_qualifier)) {
					continue;
				} else if (charat() == '\'') {
					paint_single_string(state);
				} else if (charat() == '\"') {
					paint_simple_string(state);
				} else if (charat() == '$') {
					paint(1,FLAG_NUMERAL);
					while (charat() != -1 && soy_keyword_qualifier(charat())) paint(1, FLAG_NUMERAL);
				} else {
					skip();
				}
			}
			if (charat() == '}') paint(1, FLAG_TYPE);
			return 0;
		} else if (charat() == '/' && nextchar() == '*') {
			if (paint_c_comment(state) == 1) return 5;
			return 0;
		} else {
			return syn_xml ? syn_xml->calculate(state) : 0;
		}
	}

	return -1;
}

char * syn_soy_ext[] = {".soy",NULL};

BIM_SYNTAX(soy, 1)
