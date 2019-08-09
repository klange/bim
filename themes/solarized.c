#include "bim-core.h"
#include "bim-functions.h"
#include "bim-theme.h"

/* Solarized Dark, popular theme */
BIM_THEME(solarized_dark) {
	if (!global_config.can_24bit) return;
	COLOR_FG        = "2;147;161;161";
	COLOR_BG        = "2;0;43;54";
	COLOR_ALT_FG    = "2;147;161;161";
	COLOR_ALT_BG    = "2;7;54;66";
	COLOR_NUMBER_FG = "2;131;148;149";
	COLOR_NUMBER_BG = "2;7;54;66";
	COLOR_STATUS_FG = "2;131;148;150";
	COLOR_STATUS_BG = "2;7;54;66";
	COLOR_TABBAR_BG = "2;7;54;66";
	COLOR_TAB_BG    = "2;131;148;150";
	COLOR_KEYWORD   = "2;133;153;0";
	COLOR_STRING    = "2;42;161;152";
	COLOR_COMMENT   = "2;101;123;131";
	COLOR_TYPE      = "2;181;137;0";
	COLOR_PRAGMA    = "2;203;75;22";
	COLOR_NUMERAL   = "2;220;50;47";

	COLOR_ERROR_FG  = "5;15";
	COLOR_ERROR_BG  = "5;196";
	COLOR_SEARCH_FG = "5;234";
	COLOR_SEARCH_BG = "5;226";

	COLOR_SELECTFG  = "2;0;43;54";
	COLOR_SELECTBG  = "2;147;161;161";

	COLOR_RED       = "2;222;53;53";
	COLOR_GREEN     = "2;55;167;0";

	COLOR_BOLD      = "2;147;161;161;1";
	COLOR_LINK      = "2;42;161;152;4";
	COLOR_ESCAPE    = "2;133;153;0";

	current_theme = "solarized-dark";
}

