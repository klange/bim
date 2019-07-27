#ifndef _BIM_THEMES_H
#define _BIM_THEMES_H

/**
 * Theming data
 *
 * This is all overridden by a load_colorscheme_ method.
 * The default is to load_colorscheme_ansi, but config
 * files can be used to set a different default theme.
 */
extern const char * COLOR_FG;
extern const char * COLOR_BG;
extern const char * COLOR_ALT_FG;
extern const char * COLOR_ALT_BG;
extern const char * COLOR_NUMBER_FG;
extern const char * COLOR_NUMBER_BG;
extern const char * COLOR_STATUS_FG;
extern const char * COLOR_STATUS_BG;
extern const char * COLOR_TABBAR_BG;
extern const char * COLOR_TAB_BG;
extern const char * COLOR_ERROR_FG;
extern const char * COLOR_ERROR_BG;
extern const char * COLOR_SEARCH_FG;
extern const char * COLOR_SEARCH_BG;
extern const char * COLOR_KEYWORD;
extern const char * COLOR_STRING;
extern const char * COLOR_COMMENT;
extern const char * COLOR_TYPE;
extern const char * COLOR_PRAGMA;
extern const char * COLOR_NUMERAL;
extern const char * COLOR_SELECTFG;
extern const char * COLOR_SELECTBG;
extern const char * COLOR_RED;
extern const char * COLOR_GREEN;
extern const char * COLOR_BOLD;
extern const char * COLOR_LINK;
extern const char * COLOR_ESCAPE;
extern const char * current_theme;

struct theme_def {
	const char * name;
	void (*load)(void);
};

extern struct theme_def themes[];
extern void load_colorscheme_ansi(void); /* exposed as a fallback */

#endif // _BIM_THEMES_H
