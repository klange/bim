#include "bim-core.h"
#include "bim-syntax.h"

char * syn_proto_keywords[] = {
	"syntax","import","option","package","message","group","oneof",
	"optional","required","repeated","default","extend","extensions","to","max","reserved",
	"service","rpc","returns","stream",
	NULL
};

char * syn_proto_types[] = {
	"int32","int64","uint32","uint64","sint32","sint64",
	"fixed32","fixed64","sfixed32","sfixed64",
	"float","double","bool","string","bytes",
	"enum",
	NULL
};

char * syn_proto_special[] = {
	"true","false",
	NULL
};

int syn_proto_calculate(struct syntax_state * state) {
	if (state->state < 1) {
		if (charat() == '/' && nextchar() == '/') {
			paint_comment(state);
		} else if (charat() == '/' && nextchar() == '*') {
			if (paint_c_comment(state) == 1) return 1;
			return 0;
		} else if (find_keywords(state, syn_proto_keywords, FLAG_KEYWORD, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, syn_proto_types, FLAG_TYPE, c_keyword_qualifier)) {
			return 0;
		} else if (find_keywords(state, syn_proto_special, FLAG_NUMERAL, c_keyword_qualifier)) {
			return 0;
		} else if (charat() == '"') {
			paint_simple_string(state);
			return 0;
		} else if (!c_keyword_qualifier(lastchar()) && isdigit(charat())) {
			paint_c_numeral(state);
			return 0;
		} else if (charat() != -1) {
			skip();
			return 0;
		}
		return -1;
	} else {
		if (paint_c_comment(state) == 1) return 1;
		return 0;
	}
}

char * syn_proto_ext[] = {".proto",NULL};

BIM_SYNTAX(proto, 1)
