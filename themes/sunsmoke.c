#include "bim-core.h"
#include "bim-theme.h"

BIM_THEME(sunsmoke256) {
	if (!global_config.can_256color) return;
	COLOR_FG        = "5;188";
	COLOR_BG        = "5;234";
	COLOR_ALT_FG    = "5;244";
	COLOR_ALT_BG    = "5;236";
	COLOR_NUMBER_FG = "5;101";
	COLOR_NUMBER_BG = "5;232";
	COLOR_STATUS_FG = "5;188";
	COLOR_STATUS_BG = "5;59";
	COLOR_STATUS_ALT= "5;244";
	COLOR_TABBAR_BG = "5;59";
	COLOR_TAB_BG    = "5;59";
	COLOR_KEYWORD   = "5;74";
	COLOR_STRING    = "5;71";
	COLOR_COMMENT   = global_config.can_italic ? "5;102;3" : "5;102";
	COLOR_TYPE      = "5;221";
	COLOR_PRAGMA    = "5;160";
	COLOR_NUMERAL   = "5;161";

	COLOR_ERROR_FG  = "5;15";
	COLOR_ERROR_BG  = "5;196";
	COLOR_SEARCH_FG = "5;234";
	COLOR_SEARCH_BG = "5;226";

	COLOR_SELECTFG  = "5;17";
	COLOR_SELECTBG  = "5;109";

	COLOR_RED       = "@1";
	COLOR_GREEN     = "@2";

	COLOR_BOLD      = "5;188;1";
	COLOR_LINK      = "5;74;4";
	COLOR_ESCAPE    = "5;79";

	current_theme = "sunsmoke256";
}

/* Custom theme */
BIM_THEME(sunsmoke) {
	if (!global_config.can_24bit) {
		load_colorscheme_sunsmoke256();
		return;
	}
	COLOR_FG        = "2;230;230;230";
	COLOR_BG        = "2;31;31;31";
	COLOR_ALT_FG    = "2;122;122;122";
	COLOR_ALT_BG    = "2;46;43;46";
	COLOR_NUMBER_FG = "2;150;139;57";
	COLOR_NUMBER_BG = "2;0;0;0";
	COLOR_STATUS_FG = "2;230;230;230";
	COLOR_STATUS_BG = "2;71;64;58";
	COLOR_STATUS_ALT= "2;122;122;122";
	COLOR_TABBAR_BG = "2;71;64;58";
	COLOR_TAB_BG    = "2;71;64;58";
	COLOR_KEYWORD   = "2;51;162;230";
	COLOR_STRING    = "2;72;176;72";
	COLOR_COMMENT   = "2;158;153;129;3";
	COLOR_TYPE      = "2;230;206;110";
	COLOR_PRAGMA    = "2;194;70;54";
	COLOR_NUMERAL   = "2;230;43;127";

	COLOR_ERROR_FG  = "5;15";
	COLOR_ERROR_BG  = "5;196";
	COLOR_SEARCH_FG = "5;234";
	COLOR_SEARCH_BG = "5;226";

	COLOR_SELECTFG  = "2;0;43;54";
	COLOR_SELECTBG  = "2;147;161;161";

	COLOR_RED       = "2;222;53;53";
	COLOR_GREEN     = "2;55;167;0";

	COLOR_BOLD      = "2;230;230;230;1";
	COLOR_LINK      = "2;51;162;230;4";
	COLOR_ESCAPE    = "2;113;203;173";

	current_theme = "sunsmoke";
}


