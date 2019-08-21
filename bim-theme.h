#ifndef _BIM_THEME_H
#define _BIM_THEME_H

#define BIM_THEME(name) \
	static void load_colorscheme_ ## name (void); \
	__attribute__((constructor)) static void _load_ ## name (void) { \
		add_colorscheme((struct theme_def){#name, load_colorscheme_ ## name}); \
	} \
	static void load_colorscheme_ ## name (void)

#endif /* _BIM_THEME_H */
