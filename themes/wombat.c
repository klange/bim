#include "bim-core.h"
#include "bim-theme.h"

/* Based on the wombat256 theme for vim */
BIM_THEME(wombat) {
	if (!global_config.can_256color) return;
	COLOR_FG        = "5;230";
	COLOR_BG        = "5;235";
	COLOR_ALT_FG    = "5;244";
	COLOR_ALT_BG    = "5;236";
	COLOR_NUMBER_BG = "5;232";
	COLOR_NUMBER_FG = "5;101";
	COLOR_STATUS_FG = "5;230";
	COLOR_STATUS_BG = "5;238";
	COLOR_STATUS_ALT= "5;186";
	COLOR_TABBAR_BG = "5;230";
	COLOR_TAB_BG    = "5;248";
	COLOR_KEYWORD   = "5;117";
	COLOR_STRING    = "5;113";
	COLOR_COMMENT   = global_config.can_italic ? "5;102;3" : "5;102";
	COLOR_TYPE      = "5;186";
	COLOR_PRAGMA    = "5;173";
	COLOR_NUMERAL   = COLOR_PRAGMA;

	COLOR_ERROR_FG  = "5;15";
	COLOR_ERROR_BG  = "5;196";
	COLOR_SEARCH_FG = "5;234";
	COLOR_SEARCH_BG = "5;226";

	COLOR_SELECTFG  = "5;235";
	COLOR_SELECTBG  = "5;230";

	COLOR_RED       = "@1";
	COLOR_GREEN     = "@2";

	COLOR_BOLD      = "5;230;1";
	COLOR_LINK      = "5;117;4";
	COLOR_ESCAPE    = "5;194";

	current_theme = "wombat";
}

