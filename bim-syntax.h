#ifndef _BIM_SYNTAX_H
#define _BIM_SYNTAX_H

#define BIM_SYNTAX(name, spaces) \
	__attribute__((constructor)) static void _load_ ## name (void) { \
		add_syntax((struct syntax_definition){#name, syn_ ## name ## _ext, syn_ ## name ## _calculate, spaces, NULL, NULL}); \
	} \

#define BIM_SYNTAX_EXT(name, spaces, matcher) \
	__attribute__((constructor)) static void _load_ ## name (void) { \
		add_syntax((struct syntax_definition){#name, syn_ ## name ## _ext, syn_ ## name ## _calculate, spaces, matcher, _match_completions_ ## name}); \
	} \

#define BIM_SYNTAX_COMPLETER(name) \
	static int _match_completions_ ## name ( \
		uint32_t * comp __attribute__((unused)), \
		struct completion_match **matches __attribute__((unused)), \
		int * matches_count __attribute__((unused)), \
		int complete_match __attribute__((unused)), \
		int *matches_len __attribute__((unused)), \
		buffer_t * env __attribute__((unused)))

#define paint(length, flag) do { for (int i = 0; i < (length) && state->i < state->line->actual; i++, state->i++) { state->line->text[state->i].flags = (flag); } } while (0)
#define charat() (state->i < state->line->actual ? state->line->text[(state->i)].codepoint : -1)
#define nextchar() (state->i + 1 < state->line->actual ? state->line->text[(state->i+1)].codepoint : -1)
#define lastchar() (state->i - 1 >= 0 ? state->line->text[(state->i-1)].codepoint : -1)
#define skip() (state->i++)
#define charrel(x) (state->i + (x) < state->line->actual ? state->line->text[(state->i+(x))].codepoint : -1)

extern int find_keywords(struct syntax_state * state, char ** keywords, int flag, int (*keyword_qualifier)(int c));
extern int match_and_paint(struct syntax_state * state, const char * keyword, int flag, int (*keyword_qualifier)(int c));
extern void paint_single_string(struct syntax_state * state);
extern void paint_simple_string(struct syntax_state * state);
extern int common_comment_buzzwords(struct syntax_state * state);
extern int paint_comment(struct syntax_state * state);
extern int match_forward(struct syntax_state * state, char * c);
extern struct syntax_definition * find_syntax_calculator(const char * name);

#define nest(lang, low) \
	do { \
		state->state = (state->state < 1 ? 0 : state->state - low); \
		do { state->state = lang(state); } while (state->state == 0); \
		if (state->state == -1) return low; \
		return state->state + low; \
	} while (0)

/* Some of the C stuff is widely used */
extern int c_keyword_qualifier(int c);
extern int paint_c_numeral(struct syntax_state * state);
extern int paint_c_comment(struct syntax_state * state);
extern void paint_c_char(struct syntax_state * state);

/* Hacky workaround for isdigit not really accepting Unicode stuff */
static __attribute__((used)) int _isdigit(int c) { if (c > 128) return 0; return isdigit(c); }
static __attribute__((used)) int _isxdigit(int c) { if (c > 128) return 0; return isxdigit(c); }

#undef isdigit
#undef isxdigit
#define isdigit(c) _isdigit(c)
#define isxdigit(c) _isxdigit(c)


#endif /* _BIM_SYNTAX_H */
