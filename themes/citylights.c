#include "bim-core.h"
#include "bim-functions.h"
#include "bim-theme.h"

/* "City Lights" based on citylights.xyz */
BIM_THEME(citylights) {
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

