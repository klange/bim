#pragma once

#include <wchar.h>
#include <stdint.h>
#include <string.h>

#include "kuroko/src/kuroko.h"
#include "kuroko/src/object.h"
#include "kuroko/src/vm.h"

extern int codepoint_width(wchar_t codepoint);
extern int codepoint_to_eight(uint32_t codepoint, char * out);
extern uint32_t codepoint_from_eight(uint32_t* state, uint32_t* codep, uint32_t byte);
#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

#define S(c) (krk_copyString(c,sizeof(c)-1))

#define CHECK_ARG(i, check, type, name) \
	if (argc < (i+1) || !IS_ ## check(argv[i])) { \
		return krk_runtimeError(vm.exceptions.typeError, "%s() expects %s, not '%s'", \
		/* Function name */ (__func__ + 12), /* expected type */ #type, krk_typeName(argv[i])); } \
	type name = AS_ ## check(argv[i])

#define CHECK_TYPE(klass,type,name,ptr) \
	if (argc < 1 || !krk_isInstanceOf(argv[0],klass)) return krk_runtimeError(vm.exceptions.typeError, "expected %s", #klass); \
	struct klass * self = (void*)AS_OBJECT(argv[0]); \
	type name = ptr self->value

#define MAKE_FUNC_NAME(klass,name) _ ## klass ## _ ## name
#define KRK_FUNC(klass, name, body) static KrkValue _ ## klass ## _ ## name (int argc, KrkValue argv[]) { CHECK_TYPE(klass,CURRENT_TYPE,CURRENT_NAME,CURRENT_PTR); body; return NONE_VAL(); }

#define MAKE_CLASS(klass) do { krk_makeClass(module,&klass,#klass,vm.objectClass); klass ->allocSize = sizeof(struct klass); } while (0)
#define BIND_METHOD(klass,method) do { krk_defineNative(&klass->methods, "." #method, _ ## klass ## _ ## method); } while (0)
#define BIND_FIELD(klass,method) do { krk_defineNative(&klass->methods, ":" #method, _ ## klass ## _ ## method); } while (0)
#define BIND_PROP(klass,method) do { krk_defineNativeProperty(&klass->fields, #method, _ ## klass ## _ ## method); } while (0)

#define FLEXIBLE_ARRAY(name, add_name, type, zero) \
	int flex_ ## name ## _count = 0; \
	int flex_ ## name ## _space = 0; \
	type * name = NULL; \
	void add_name (type input) { \
		if (flex_ ## name ## _space == 0) { \
			flex_ ## name ## _space = 4; \
			name = calloc(sizeof(type), flex_ ## name ## _space); \
		} else if (flex_ ## name ## _count + 1 == flex_ ## name ## _space) { \
			flex_ ## name ## _space *= 2; \
			name = realloc(name, sizeof(type) * flex_ ## name ## _space); \
			for (int i = flex_ ## name ## _count; i < flex_ ## name ## _space; ++i) name[i] = zero; \
		} \
		name[flex_ ## name ## _count] = input; \
		flex_ ## name ## _count ++; \
	}

static inline int bim_isdigit(unsigned int c) {
	return (c >= '0' && c <= '9');
}

static inline int bim_isxdigit(unsigned int c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int bim_isalpha(unsigned int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int bim_isalnum(unsigned int c) {
	return bim_isdigit(c) || bim_isalpha(c);
}

extern void krk_makeClass(KrkInstance * module, KrkClass ** _class, const char * name, KrkClass * base);
extern void krk_defineNativeProperty(KrkTable * table, const char * name, NativeFn func);
