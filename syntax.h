#pragma once

#include <stdint.h>
#include "kuroko/src/object.h"
#include "buffer.h"

struct syntax_state {
	buffer_t * env;
	line_t * line;
	int line_no;
	int state;
	int i;
};

#define FLAG_NONE      0
#define FLAG_KEYWORD   1
#define FLAG_STRING    2
#define FLAG_COMMENT   3
#define FLAG_TYPE      4
#define FLAG_PRAGMA    5
#define FLAG_NUMERAL   6
#define FLAG_ERROR     7
#define FLAG_DIFFPLUS  8
#define FLAG_DIFFMINUS 9
#define FLAG_NOTICE    10
#define FLAG_BOLD      11
#define FLAG_LINK      12
#define FLAG_ESCAPE    13

#define FLAG_SELECT    (1 << 5)
#define FLAG_SEARCH    (1 << 6)

extern KrkClass * SyntaxState;
struct SyntaxState {
	KrkInstance inst;
	struct syntax_state value;
};

#define IS_SYNTAXSTATE(val) (krk_isInstanceOf((val), SyntaxState))
#define AS_SYNTAXSTATE(val) (((struct SyntaxState*)AS_OBJECT((val)))->value)
