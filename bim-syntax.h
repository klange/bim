#ifndef _BIM_SYNTAX_H
#define _BIM_SYNTAX_H

#define BIM_SYNTAX(name, spaces) \
	__attribute__((constructor)) static void _load_ ## name (void) { \
		add_syntax((struct syntax_definition){#name, syn_ ## name ## _ext, syn_ ## name ## _calculate, spaces}); \
	} \

#define paint(length, flag) do { for (int i = 0; i < (length) && state->i < state->line->actual; i++, state->i++) { state->line->text[state->i].flags = (flag); } } while (0)
#define charat() (state->i < state->line->actual ? state->line->text[(state->i)].codepoint : -1)
#define nextchar() (state->i + 1 < state->line->actual ? state->line->text[(state->i+1)].codepoint : -1)
#define lastchar() (state->i - 1 >= 0 ? state->line->text[(state->i-1)].codepoint : -1)
#define skip() (state->i++)
#define charrel(x) (state->i + (x) < state->line->actual ? state->line->text[(state->i+(x))].codepoint : -1)

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


#endif /* _BIM_SYNTAX_H */
