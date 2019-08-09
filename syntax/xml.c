#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

int syn_xml_calculate(struct syntax_state * state) {
	switch (state->state) {
		case -1:
		case 0:
			if (charat() == -1) return -1;
			if (charat() != '<') {
				skip();
				return 0;
			}
			/* Opening brace */
			if (charat() == '<' && nextchar() == '!' && charrel(2) == '-' && charrel(3) == '-') {
				paint(4, FLAG_COMMENT);
				goto _comment;
			}
			paint(1, FLAG_TYPE);
			/* Fall through */
		case 1:
			/* State 1: We saw an opening brace. */
			while (charat() != -1) {
				if (charat() == '/') paint(1, FLAG_TYPE);
				if (charat() == '?') paint(1, FLAG_TYPE);
				if (charat() == ' ' || charat() == '\t') skip();
				if (isalnum(charat())) {
					while (isalnum(charat()) || charat() == '-') paint(1, FLAG_KEYWORD);
					if (charat() == -1) return 2;
					goto _in_tag;
				} else {
					paint(1, FLAG_TYPE);
				}
			}
			return -1;
_in_tag:
		case 2:
			while (charat() != -1) {
				if (charat() == '>') {
					paint(1, FLAG_TYPE);
					return 0;
				} else if (charat() == '"') {
					paint_simple_string(state);
					if (charat() == -1 && lastchar() != '"') {
						return 3;
					}
				} else {
					paint(1, FLAG_TYPE);
				}
			}
			return 2;
		case 3:
			/* In a string in tag */
			if (charat() == '"') {
				paint(1, FLAG_STRING);
				return 2;
			} else {
				paint_simple_string(state);
				if (charat() == -1 && lastchar() != '"') {
					return 3;
				}
			}
			break;
_comment:
		case 4:
			while (charat() != -1) {
				if (charat() == '-' && nextchar() == '-' && charrel(2) == '>') {
					paint(3, FLAG_COMMENT);
					return 0;
				} else {
					if (common_comment_buzzwords(state)) continue;
					else paint(1, FLAG_COMMENT);
				}
			}
			return 4;
	}
	return -1;
}

char * syn_xml_ext[] = {".xml",".htm",".html",NULL}; // TODO other stuff that uses xml (it's a lot!); FIXME htm/html are temporary; make dedicated SGML ones for this

BIM_SYNTAX(xml, 1)
