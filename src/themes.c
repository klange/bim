#include "themes.h"
#include "config.h"

const char * COLOR_FG        = "@17";
const char * COLOR_BG        = "@0";
const char * COLOR_ALT_FG    = "@17";
const char * COLOR_ALT_BG    = "@0";
const char * COLOR_NUMBER_FG = "@17";
const char * COLOR_NUMBER_BG = "@0";
const char * COLOR_STATUS_FG = "@17";
const char * COLOR_STATUS_BG = "@0";
const char * COLOR_TABBAR_BG = "@0";
const char * COLOR_TAB_BG    = "@0";
const char * COLOR_ERROR_FG  = "@17";
const char * COLOR_ERROR_BG  = "@0";
const char * COLOR_SEARCH_FG = "@17";
const char * COLOR_SEARCH_BG = "@0";
const char * COLOR_KEYWORD   = "@17";
const char * COLOR_STRING    = "@17";
const char * COLOR_COMMENT   = "@17";
const char * COLOR_TYPE      = "@17";
const char * COLOR_PRAGMA    = "@17";
const char * COLOR_NUMERAL   = "@17";
const char * COLOR_SELECTFG  = "@0";
const char * COLOR_SELECTBG  = "@17";
const char * COLOR_RED       = "@1";
const char * COLOR_GREEN     = "@2";
const char * COLOR_BOLD      = "@17";
const char * COLOR_LINK      = "@17";
const char * COLOR_ESCAPE    = "@17";
const char * current_theme = "none";

/**
 * Themes
 */

/* 16-color theme, default */
void load_colorscheme_ansi(void) {
	COLOR_FG        = global_config.can_bright ? "@17" : "@7";
	COLOR_BG        = global_config.can_bright ? "@9"  : "@0";
	COLOR_ALT_FG    = global_config.can_bright ? "@10" : "@5";
	COLOR_ALT_BG    = "@9";
	COLOR_NUMBER_FG = "@3";
	COLOR_NUMBER_BG = "@9";
	COLOR_STATUS_FG = global_config.can_bright ? "@17" : "@7";
	COLOR_STATUS_BG = "@4";
	COLOR_TABBAR_BG = "@4";
	COLOR_TAB_BG    = "@4";
	COLOR_KEYWORD   = global_config.can_bright ? "@14" : "@4";
	COLOR_STRING    = "@2";
	COLOR_COMMENT   = global_config.can_bright ? "@10" : "@5";
	COLOR_TYPE      = "@3";
	COLOR_PRAGMA    = "@1";
	COLOR_NUMERAL   = "@1";

	COLOR_ERROR_FG  = global_config.can_bright ? "@17" : "@7";
	COLOR_ERROR_BG  = "@1";
	COLOR_SEARCH_FG = "@0";
	COLOR_SEARCH_BG = global_config.can_bright ? "@13" : "@3";

	COLOR_SELECTBG  = global_config.can_bright ? "@17" : "@7";
	COLOR_SELECTFG  = "@0";

	COLOR_RED       = "@1";
	COLOR_GREEN     = "@2";

	COLOR_BOLD      = COLOR_FG; /* @ doesn't support extra args; FIXME */
	COLOR_LINK      = global_config.can_bright ? "@14" : "@4";
	COLOR_ESCAPE    = global_config.can_bright ? "@12" : "@2";

	current_theme = "ansi";
}

/* Based on the wombat256 theme for vim */
void load_colorscheme_wombat(void) {
	if (!global_config.can_256color) return;
	COLOR_FG        = "5;230";
	COLOR_BG        = "5;235";
	COLOR_ALT_FG    = "5;244";
	COLOR_ALT_BG    = "5;236";
	COLOR_NUMBER_BG = "5;232";
	COLOR_NUMBER_FG = "5;101";
	COLOR_STATUS_FG = "5;230";
	COLOR_STATUS_BG = "5;238";
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

/* "City Lights" based on citylights.xyz */
void load_colorscheme_citylights(void) {
	if (!global_config.can_24bit) return;
	COLOR_FG        = "2;151;178;198";
	COLOR_BG        = "2;29;37;44";
	COLOR_ALT_FG    = "2;45;55;65";
	COLOR_ALT_BG    = "2;33;42;50";
	COLOR_NUMBER_FG = "2;71;89;103";
	COLOR_NUMBER_BG = "2;37;47;56";
	COLOR_STATUS_FG = "2;116;144;166";
	COLOR_STATUS_BG = "2;53;67;78";
	COLOR_TABBAR_BG = "2;37;47;56";
	COLOR_TAB_BG    = "2;29;37;44";
	COLOR_KEYWORD   = "2;94;196;255";
	COLOR_STRING    = "2;83;154;252";
	COLOR_COMMENT   = "2;107;133;153;3";
	COLOR_TYPE      = "2;139;212;156";
	COLOR_PRAGMA    = "2;0;139;148";
	COLOR_NUMERAL   = "2;207;118;132";

	COLOR_ERROR_FG  = "5;15";
	COLOR_ERROR_BG  = "5;196";
	COLOR_SEARCH_FG = "5;234";
	COLOR_SEARCH_BG = "5;226";

	COLOR_SELECTFG  = "2;29;37;44";
	COLOR_SELECTBG  = "2;151;178;198";

	COLOR_RED       = "2;222;53;53";
	COLOR_GREEN     = "2;55;167;0";

	COLOR_BOLD      = "2;151;178;198;1";
	COLOR_LINK      = "2;94;196;255;4";
	COLOR_ESCAPE    = "2;133;182;249";

	current_theme = "citylights";
}

/* Solarized Dark, popular theme */
void load_colorscheme_solarized_dark(void) {
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


void load_colorscheme_sunsmoke256(void) {
	if (!global_config.can_256color) return;
	COLOR_FG        = "5;188";
	COLOR_BG        = "5;234";
	COLOR_ALT_FG    = "5;244";
	COLOR_ALT_BG    = "5;236";
	COLOR_NUMBER_FG = "5;101";
	COLOR_NUMBER_BG = "5;232";
	COLOR_STATUS_FG = "5;188";
	COLOR_STATUS_BG = "5;59";
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
void load_colorscheme_sunsmoke(void) {
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

struct theme_def themes[] = {
	{"wombat", load_colorscheme_wombat},
	{"citylights", load_colorscheme_citylights},
	{"solarized-dark", load_colorscheme_solarized_dark},
	{"ansi", load_colorscheme_ansi},
	{"sunsmoke", load_colorscheme_sunsmoke},
	{"sunsmoke256", load_colorscheme_sunsmoke256},
	{NULL, NULL}
};

