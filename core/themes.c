#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

/**
 * Themes
 */
int theme_count = 0;
int theme_space = 0;
struct theme_def * themes = NULL;

void add_colorscheme(const char * name, void (*load)(void)) {
	if (theme_space  == 0) {
		theme_space = 4;
		themes = calloc(sizeof(struct theme_def), theme_space);
	} else if (theme_count == theme_space) {
		theme_space *= 2;
		themes = realloc(themes, sizeof(struct theme_def) * theme_space);
	}
	themes[theme_count].name = name;
	themes[theme_count].load = load;
	theme_count++;
}

/**
 * Theming data
 *
 * This is all overridden by a load_colorscheme_ method.
 * The default is to load_colorscheme_ansi, but config
 * files can be used to set a different default theme.
 */
const char * COLOR_FG        = "@9";
const char * COLOR_BG        = "@9";
const char * COLOR_ALT_FG    = "@9";
const char * COLOR_ALT_BG    = "@9";
const char * COLOR_NUMBER_FG = "@9";
const char * COLOR_NUMBER_BG = "@9";
const char * COLOR_STATUS_FG = "@9";
const char * COLOR_STATUS_BG = "@9";
const char * COLOR_TABBAR_BG = "@9";
const char * COLOR_TAB_BG    = "@9";
const char * COLOR_ERROR_FG  = "@9";
const char * COLOR_ERROR_BG  = "@9";
const char * COLOR_SEARCH_FG = "@0";
const char * COLOR_SEARCH_BG = "@17";
const char * COLOR_KEYWORD   = "@9";
const char * COLOR_STRING    = "@9";
const char * COLOR_COMMENT   = "@9";
const char * COLOR_TYPE      = "@9";
const char * COLOR_PRAGMA    = "@9";
const char * COLOR_NUMERAL   = "@9";
const char * COLOR_SELECTFG  = "@0";
const char * COLOR_SELECTBG  = "@17";
const char * COLOR_RED       = "@1";
const char * COLOR_GREEN     = "@2";
const char * COLOR_BOLD      = "@9";
const char * COLOR_LINK      = "@9";
const char * COLOR_ESCAPE    = "@9";
const char * current_theme = "none";

/**
 * Convert syntax highlighting flag to color code
 */
const char * flag_to_color(int _flag) {
	int flag = _flag & 0xF;
	switch (flag) {
		case FLAG_KEYWORD:
			return COLOR_KEYWORD;
		case FLAG_STRING:
			return COLOR_STRING;
		case FLAG_COMMENT:
			return COLOR_COMMENT;
		case FLAG_TYPE:
			return COLOR_TYPE;
		case FLAG_NUMERAL:
			return COLOR_NUMERAL;
		case FLAG_PRAGMA:
			return COLOR_PRAGMA;
		case FLAG_DIFFPLUS:
			return COLOR_GREEN;
		case FLAG_DIFFMINUS:
			return COLOR_RED;
		case FLAG_SELECT:
			return COLOR_FG;
		case FLAG_BOLD:
			return COLOR_BOLD;
		case FLAG_LINK:
			return COLOR_LINK;
		case FLAG_ESCAPE:
			return COLOR_ESCAPE;
		default:
			return COLOR_FG;
	}
}

