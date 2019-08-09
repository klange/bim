#include "bim-core.h"
#include "bim-theme.h"

/* 16-color theme, default */
BIM_THEME(ansi) {
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

