/**
 * Syntax highlighting bindings.
 */
#include "kuroko/src/kuroko.h"
#include "kuroko/src/object.h"
#include "kuroko/src/vm.h"

#include "buffer.h"
#include "syntax.h"
#include "util.h"

KrkClass * SyntaxState = NULL;

#define CURRENT_PTR   &
#define CURRENT_TYPE  struct syntax_state *
#define CURRENT_NAME  state
KRK_FUNC(SyntaxState,state, { return INTEGER_VAL(state->state); })
KRK_FUNC(SyntaxState,set_state, {
	if (argc > 1 && IS_INTEGER(argv[1])) {
		state->state = AS_INTEGER(argv[1]);
	} else if (argc > 1 && IS_NONE(argv[1])) {
		state->state = -1;
	} else {
		CHECK_ARG(1,INTEGER,int,newState);
		(void)newState;
	}
})
KRK_FUNC(SyntaxState,i, { return INTEGER_VAL(state->i); })
KRK_FUNC(SyntaxState,lineno, { return INTEGER_VAL(state->line_no); })
#define charrel(x) ((state->i + (x) >= 0 && state->i + (x) < state->line->actual) ? state->line->text[(state->i+(x))].codepoint : -1)
KRK_FUNC(SyntaxState,__get__, {
	CHECK_ARG(1,INTEGER,int,arg);
	int charRel = charrel(arg);
	if (charRel == -1) return NONE_VAL();
	char tmp[8];
	size_t len = codepoint_to_eight(charRel, tmp);
	return OBJECT_VAL(krk_copyString(tmp,len));
})
KRK_FUNC(SyntaxState,isdigit, {
	(void)state;
	if (argc > 1 && IS_NONE(argv[1])) return BOOLEAN_VAL(0);
	CHECK_ARG(1,STRING,KrkString*,str);
	if (str->codesLength > 1) return krk_runtimeError(vm.exceptions.typeError, "isdigit() expected str of len 1, not %d", str->codesLength);
	unsigned int c = krk_unicodeCodepoint(str,0);
	return BOOLEAN_VAL(!!bim_isdigit(c));
})
KRK_FUNC(SyntaxState,isxdigit, {
	(void)state;
	if (argc > 1 && IS_NONE(argv[1])) return BOOLEAN_VAL(0);
	CHECK_ARG(1,STRING,KrkString*,str);
	if (str->codesLength > 1) return krk_runtimeError(vm.exceptions.typeError, "isxdigit() expected str of len 1, not %d", str->codesLength);
	unsigned int c = krk_unicodeCodepoint(str,0);
	return BOOLEAN_VAL(!!bim_isxdigit(c));
})
KRK_FUNC(SyntaxState,isalpha, {
	(void)state;
	if (argc > 1 && IS_NONE(argv[1])) return BOOLEAN_VAL(0);
	CHECK_ARG(1,STRING,KrkString*,str);
	if (str->codesLength > 1) return krk_runtimeError(vm.exceptions.typeError, "isalpha() expected str of len 1, not %d", str->codesLength);
	unsigned int c = krk_unicodeCodepoint(str,0);
	return BOOLEAN_VAL(!!bim_isalpha(c));
})
KRK_FUNC(SyntaxState,isalnum, {
	(void)state;
	if (argc > 1 && IS_NONE(argv[1])) return BOOLEAN_VAL(0);
	CHECK_ARG(1,STRING,KrkString*,str);
	if (str->codesLength > 1) return krk_runtimeError(vm.exceptions.typeError, "isalnum() expected str of len 1, not %d", str->codesLength);
	unsigned int c = krk_unicodeCodepoint(str,0);
	return BOOLEAN_VAL(!!bim_isalnum(c));
})
#define paint(howMuch,whatFlag) \
	do { for (int i = 0; i < howMuch && state->i < state->line->actual; i++, state->i++) { \
		state->line->text[state->i].flags = (state->line->text[state->i].flags & (3 << 5)) | whatFlag; \
	} } while (0)
KRK_FUNC(SyntaxState,paint, {
	CHECK_ARG(1,INTEGER,int,howMuch);
	if (howMuch == -1) howMuch = state->line->actual;
	CHECK_ARG(2,INTEGER,int,whatFlag);
	paint(howMuch,whatFlag);
	return NONE_VAL();
})
static int c_keyword_qualifier(int c) {
	return bim_isalnum(c) || (c == '_');
}
KRK_FUNC(SyntaxState,cKeywordQualifier, {
	(void)state;
	if (IS_INTEGER(argv[1])) return BOOLEAN_VAL(!!c_keyword_qualifier(AS_INTEGER(argv[1])));
	if (!IS_STRING(argv[1])) return BOOLEAN_VAL(0);
	if (AS_STRING(argv[1])->length > 1) return BOOLEAN_VAL(0);
	return BOOLEAN_VAL(!!c_keyword_qualifier(AS_CSTRING(argv[1])[0]));
})
static int callQualifier(KrkValue qualifier, int codepoint) {
	krk_push(qualifier);
	krk_push(INTEGER_VAL(codepoint));
	KrkValue result = krk_callSimple(krk_peek(1), 1, 1);
	if (IS_BOOLEAN(result)) return AS_BOOLEAN(result);
	return 0;
}
KRK_FUNC(SyntaxState,matchAndPaint, {
	CHECK_ARG(1,STRING,KrkString*,me);
	CHECK_ARG(2,INTEGER,int,flag);
	CHECK_ARG(3,OBJECT,KrkObj*,qualifier);
	size_t d = 0;
	if (me->type == KRK_STRING_ASCII) {
		while (state->i + (int)d < state->line->actual &&
		       d < me->codesLength &&
		       state->line->text[state->i+d].codepoint == me->chars[d]) d++;
	} else {
		krk_unicodeString(me);
		while (state->i + (int)d < state->line->actual &&
		       d < me->codesLength &&
		       state->line->text[state->i+d].codepoint == KRK_STRING_FAST(me,d)) d++;
	}
	if (d == me->codesLength && (state->i + (int)d >= state->line->actual ||
		!callQualifier(OBJECT_VAL(qualifier),state->line->text[state->i+d].codepoint))) {
		paint((int)me->codesLength, flag);
		return BOOLEAN_VAL(1);
	}
	return BOOLEAN_VAL(0);
})
#define IS_LIST(val) (krk_isInstanceOf((val),vm.baseClasses.listClass))
KRK_FUNC(SyntaxState,findKeywords, {
	CHECK_ARG(1,LIST,KrkValueArray*,keywords);
	CHECK_ARG(2,INTEGER,int,flag);
	CHECK_ARG(3,OBJECT,KrkObj*,qualifier);
	if (callQualifier(OBJECT_VAL(qualifier), charrel(-1))) return BOOLEAN_VAL(0);
	if (!callQualifier(OBJECT_VAL(qualifier), charrel(0))) return BOOLEAN_VAL(0);
	for (size_t keyword = 0; keyword < keywords->count; ++keyword) {
		if (!IS_STRING(keywords->values[keyword]))
			return krk_runtimeError(vm.exceptions.typeError, "expected list of strings, found '%s'", krk_typeName(keywords->values[keyword]));
		KrkString * me = AS_STRING(keywords->values[keyword]);
		size_t d = 0;
		if (me->type == KRK_STRING_ASCII) {
			while (state->i + (int)d < state->line->actual &&
			       d < me->codesLength &&
			       state->line->text[state->i+d].codepoint == me->chars[d]) d++;
		} else {
			krk_unicodeString(me);
			while (state->i + (int)d < state->line->actual &&
			       d < me->codesLength &&
			       state->line->text[state->i+d].codepoint == KRK_STRING_FAST(me,d)) d++;
		}
		if (d == me->codesLength && (state->i + (int)d >= state->line->actual ||
			!callQualifier(OBJECT_VAL(qualifier),state->line->text[state->i+d].codepoint))) {
			paint((int)me->codesLength, flag);
			return BOOLEAN_VAL(1);
		}
	}
	return BOOLEAN_VAL(0);
})
KRK_FUNC(SyntaxState,rewind, {
	CHECK_ARG(1,INTEGER,int,howMuch);
	state->i -= howMuch;
})
KRK_FUNC(SyntaxState,skip, {
	state->i++;
})
KRK_FUNC(SyntaxState,__init__, {
	CHECK_ARG(1,SYNTAXSTATE,struct syntax_state,from);
	*state = from;
	return argv[0];
})

#undef CURRENT_TYPE
#undef CURRENT_NAME
#undef CURRENT_PTR

static int checkClass(KrkClass * _class, KrkClass * base) {
	while (_class) {
		if (_class == base) return 1;
		_class = _class->base;
	}
	return 0;
}

struct syntax_def {
	const char * name;
	char ** ext;
	int prefersSpaces;
	KrkObj * krkFunc;
	KrkObj * krkClass;
};

FLEXIBLE_ARRAY(syntax_highlighters, add_syntax, struct syntax_def, ((struct syntax_def){NULL,NULL,0,NULL,NULL}))

static KrkValue _syntax_dict;
static KrkValue _register_syntax(int argc, KrkValue argv[]) {
	if (argc < 1 || !IS_CLASS(argv[0]) || !checkClass(AS_CLASS(argv[0]), SyntaxState))
		return krk_runtimeError(vm.exceptions.typeError, "Can not register '%s' as a syntax highlighter, expected subclass of SyntaxState.", krk_typeName(argv[0]));

	KrkValue name = NONE_VAL(), extensions = NONE_VAL(), spaces = BOOLEAN_VAL(0), calculate = NONE_VAL();
	krk_tableGet(&AS_CLASS(argv[0])->fields, OBJECT_VAL(S("name")), &name);
	krk_tableGet(&AS_CLASS(argv[0])->fields, OBJECT_VAL(S("extensions")), &extensions);
	krk_tableGet(&AS_CLASS(argv[0])->fields, OBJECT_VAL(S("spaces")), &spaces);
	krk_tableGet(&AS_CLASS(argv[0])->methods, OBJECT_VAL(S("calculate")), &calculate);
	if (!IS_STRING(name))
		return krk_runtimeError(vm.exceptions.typeError, "%s.name must be str", AS_CLASS(argv[0])->name->chars);
	if (!IS_TUPLE(extensions))
		return krk_runtimeError(vm.exceptions.typeError, "%s.extensions must by tuple<str>", AS_CLASS(argv[0])->name->chars);
	if (!IS_BOOLEAN(spaces))
		return krk_runtimeError(vm.exceptions.typeError, "%s.spaces must be bool", AS_CLASS(argv[0])->name->chars);
	if (!IS_CLOSURE(calculate))
		return krk_runtimeError(vm.exceptions.typeError, "%s.calculate must be method, not '%s'", AS_CLASS(argv[0])->name->chars, krk_typeName(calculate));

	/* Convert tuple of strings */
	char ** ext = malloc(sizeof(char *) * (AS_TUPLE(extensions)->values.count + 1)); /* +1 for NULL */
	ext[AS_TUPLE(extensions)->values.count] = NULL;
	for (size_t i = 0; i < AS_TUPLE(extensions)->values.count; ++i) {
		if (!IS_STRING(AS_TUPLE(extensions)->values.values[i])) {
			free(ext);
			return krk_runtimeError(vm.exceptions.typeError, "%s.extensions must by tuple<str>", AS_CLASS(argv[0])->name->chars);
		}
		ext[i] = AS_CSTRING(AS_TUPLE(extensions)->values.values[i]);
	}

	add_syntax((struct syntax_def){AS_CSTRING(name), ext, AS_BOOLEAN(spaces), AS_OBJECT(calculate), AS_OBJECT(argv[0])});

	/* And save it in the module stuff. */
	krk_tableSet(AS_DICT(_syntax_dict), name, argv[0]);

	return NONE_VAL();
}

#define FLAG(n) krk_attachNamedValue(&SyntaxState->fields, #n, INTEGER_VAL(n))

static KrkValue _highlight_buffer(int argc, KrkValue argv[]) {
	if (argc < 1 || !IS_TEXTBUFFER(argv[0]))
		return krk_runtimeError(vm.exceptions.typeError, "expected TextBuffer");

	buffer_t * env = AS_TEXTBUFFER(argv[0]);

	/* Find the right highlighter */
	KrkValue highlighter = NONE_VAL();
	if (!krk_tableGet(AS_DICT(_syntax_dict), OBJECT_VAL(S("c")), &highlighter))
		return krk_runtimeError(vm.exceptions.valueError, "highlighter isn't loaded");

	KrkValue func = NONE_VAL();
	if (!krk_tableGet(&AS_CLASS(highlighter)->methods, OBJECT_VAL(S("calculate")), &func))
		return krk_runtimeError(vm.exceptions.valueError, "failed to get calculate function");

	for (int i = 0; i < env->line_count; ++i) {
		for (int j = 0; j < env->lines[i]->actual; ++j) {
			env->lines[i]->text[j].flags = 0;
		}

		struct SyntaxState * s = (void*)krk_newInstance(AS_CLASS(highlighter));
		s->value.env = env;
		s->value.line = env->lines[i];
		s->value.line_no = i;
		s->value.state = env->lines[i]->istate;
		s->value.i = 0;

		while (1) {
			ptrdiff_t before = vm.stackTop - vm.stack;
			krk_push(OBJECT_VAL(s));
			KrkValue result = krk_callSimple(func, 1, 0);
			vm.stackTop = vm.stack + before;

			s->value.state = IS_NONE(result) ? -1 : (IS_INTEGER(result) ? AS_INTEGER(result) : -1);
			if (s->value.state != 0) {
				if (i + 1 < env->line_count) {
					env->lines[i+1]->istate = s->value.state;
				}
				break;
			}
		}
	}

	return NONE_VAL();
}

void kuroko_bind_syntax(KrkInstance * module) {
	MAKE_CLASS(SyntaxState);
	BIND_FIELD(SyntaxState, i);
	BIND_FIELD(SyntaxState, lineno);
	BIND_FIELD(SyntaxState, state);
	BIND_METHOD(SyntaxState, __get__);
	BIND_METHOD(SyntaxState, __init__);
	BIND_METHOD(SyntaxState, set_state);
	BIND_METHOD(SyntaxState, isdigit);
	BIND_METHOD(SyntaxState, isxdigit);
	BIND_METHOD(SyntaxState, isalpha);
	BIND_METHOD(SyntaxState, isalnum);
	BIND_METHOD(SyntaxState, paint);
	BIND_METHOD(SyntaxState, cKeywordQualifier);
	BIND_METHOD(SyntaxState, matchAndPaint);
	BIND_METHOD(SyntaxState, findKeywords);
	BIND_METHOD(SyntaxState, rewind);
	BIND_METHOD(SyntaxState, skip);

	FLAG(FLAG_NONE);
	FLAG(FLAG_KEYWORD);
	FLAG(FLAG_STRING);
	FLAG(FLAG_COMMENT);
	FLAG(FLAG_TYPE);
	FLAG(FLAG_PRAGMA);
	FLAG(FLAG_NUMERAL);
	FLAG(FLAG_ERROR);
	FLAG(FLAG_DIFFPLUS);
	FLAG(FLAG_DIFFMINUS);
	FLAG(FLAG_NOTICE);
	FLAG(FLAG_BOLD);
	FLAG(FLAG_LINK);
	FLAG(FLAG_ESCAPE);
	FLAG(FLAG_SELECT);
	FLAG(FLAG_SEARCH);

	krk_finalizeClass(SyntaxState);

	_syntax_dict = krk_dict_of(0,NULL);
	krk_attachNamedValue(&module->fields, "highlighters", _syntax_dict);
	krk_defineNative(&module->fields, "bindHighlighter", _register_syntax);

	krk_defineNative(&module->fields, "xxx_highlight_buffer", _highlight_buffer);
}
