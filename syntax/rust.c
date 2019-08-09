#include "bim-core.h"
#include "bim-syntax.h"

static char * syn_rust_keywords[] = {
	"as","break","const","continue","crate","else","enum","extern",
	"false","fn","for","if","impl","in","let","loop","match","mod",
	"move","mut","pub","ref","return","Self","self","static","struct",
	"super","trait","true","type","unsafe","use","where","while",
	NULL,
};

static char * syn_rust_types[] = {
	"bool","char","str",
	"i8","i16","i32","i64",
	"u8","u16","u32","u64",
	"isize","usize",
	"f32","f64",
	NULL,
};

static int paint_rs_comment(struct syntax_state * state) {
	while (charat() != -1) {
		if (common_comment_buzzwords(state)) continue;
		else if (charat() == '*' && nextchar() == '/') {
			paint(2, FLAG_COMMENT);
			state->state--;
			if (state->state == 0) return 0;
		} else if (charat() == '/' && nextchar() == '*') {
			state->state++;
			paint(2, FLAG_COMMENT);
		} else {
			paint(1, FLAG_COMMENT);
		}
	}
	return state->state;
}

int paint_rust_numeral(struct syntax_state * state) {
	if (charat() == '0' && nextchar() == 'b') {
		paint(2, FLAG_NUMERAL);
		while (charat() == '0' || charat() == '1' || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == 'o') {
		paint(2, FLAG_NUMERAL);
		while ((charat() >= '0' && charat() <= '7') || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == 'x') {
		paint(2, FLAG_NUMERAL);
		while (isxdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
	} else if (charat() == '0' && nextchar() == '.') {
		paint(2, FLAG_NUMERAL);
		while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
	} else {
		while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
		if (charat() == '.') {
			paint(1, FLAG_NUMERAL);
			while (isdigit(charat()) || charat() == '_') paint(1, FLAG_NUMERAL);
		}
	}
	return 0;
}

static int syn_rust_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == '/' && nextchar() == '/') {
				/* C++-style comments */
				paint_comment(state);
			} else if (charat() == '/' && nextchar() == '*') {
				paint(2, FLAG_COMMENT);
				state->state = 1;
				return paint_rs_comment(state);
			} else if (find_keywords(state, syn_rust_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
				return 0;
			} else if (find_keywords(state, syn_rust_types, FLAG_TYPE, c_keyword_qualifier)) {
				return 0;
			} else if (charat() == '\"') {
				paint_simple_string(state);
				return 0;
			} else if (charat() == '\'') {
				paint_c_char(state);
				return 0;
			} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
				paint_rust_numeral(state);
				return 0;
			} else if (charat() != -1) {
				skip();
				return 0;
			}
			break;
		default: /* Nested comments */
			return paint_rs_comment(state);
	}

	return -1;
}

char * syn_rust_ext[] = {".rs",NULL};

BIM_SYNTAX(rust, 1)
