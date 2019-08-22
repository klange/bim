#include "bim-core.h"
#include "bim-syntax.h"

static char * html_elements[] = {
	"a","abbr","address","area","article","aside","audio",
	"b","base","bdi","bdo","blockquote","body","br","button",
	"canvas","cite","code","col","colgroup","data","datalist",
	"dd","del","details","dfn","dialog","div","dl","dt","em",
	"embed","fieldset","figcaption","figure","footer","form",
	"h1","h2","h3","h4","h5","h6","head","header","hr","html",
	"i","iframe","img","input","ins","kbd","label","legend",
	"li","link","main","map","mark","meta","meter","nav",
	"noscript","object","ol","optgroup","option","output",
	"p","param","picture","pre","progress","q","rp","rt",
	"ruby","s","samp","script","section","select","small",
	"source","span","strong","style","sub","summary","sup",
	"svg","table","tbody","td","template","textarea","tfoot",
	"th","thead","time","title","tr","track","u","ul","var",
	"video","wbr","hgroup","*",
	NULL
};

static char * css_properties[] = {
	"align-content","align-items","align-self","all","animation",
	"animation-delay","animation-direction","animation-duration",
	"animation-fill-mode","animation-iteration-count","animation-name",
	"animation-play-state","animation-timing-function","backface-visibility",
	"background","background-attachment","background-blend-mode","background-clip",
	"background-color","background-image","background-origin","background-position",
	"background-repeat","background-size","border","border-bottom","border-bottom-color",
	"border-bottom-left-radius","border-bottom-right-radius","border-bottom-style",
	"border-bottom-width","border-collapse","border-color","border-image","border-image-outset",
	"border-image-repeat","border-image-slice","border-image-source","border-image-width",
	"border-left","border-left-color","border-left-style","border-left-width",
	"border-radius","border-right","border-right-color","border-right-style","border-right-width",
	"border-spacing","border-style","border-top","border-top-color","border-top-left-radius",
	"border-top-right-radius","border-top-style","border-top-width","border-width",
	"bottom","box-decoration-break","box-shadow","box-sizing","break-after",
	"break-before","break-inside","caption-side","caret-color","@charset",
	"clear","clip","color","column-count","column-fill","column-gap","column-rule","column-rule-color",
	"column-rule-style","column-rule-width","column-span","column-width","columns","content",
	"counter-increment","counter-reset","cursor","direction","display","empty-cells",
	"filter","flex","flex-basis","flex-direction","flex-flow","flex-grow","flex-shrink",
	"flex-wrap","float","font","@font-face","font-family","font-feature-settings","@font-feature-values",
	"font-kerning","font-language-override","font-size","font-size-adjust","font-stretch","font-style",
	"font-synthesis","font-variant","font-variant-alternates","font-variant-caps","font-variant-east-asian",
	"font-variant-ligatures","font-variant-numeric","font-variant-position","font-weight",
	"grid","grid-area","grid-auto-columns","grid-auto-flow","grid-auto-rows","grid-column",
	"grid-column-end","grid-column-gap","grid-column-start","grid-gap","grid-row","grid-row-end",
	"grid-row-gap","grid-row-start","grid-template","grid-template-areas","grid-template-columns",
	"grid-template-rows","hanging-punctuation","height","hyphens","image-rendering","@import",
	"isolation","justify-content","@keyframes","left","letter-spacing","line-break","line-height",
	"list-style","list-style-image","list-style-position","list-style-type","margin","margin-bottom",
	"margin-left","margin-right","margin-top","max-height","max-width","@media","min-height",
	"min-width","mix-blend-mode","object-fit","object-position","opacity","order","orphans",
	"outline","outline-color","outline-offset","outline-style","outline-width","overflow",
	"overflow-wrap","overflow-x","overflow-y","padding","padding-bottom","padding-left","padding-right",
	"padding-top","page-break-after","page-break-before","page-break-inside","perspective",
	"perspective-origin","pointer-events","position","quotes","resize","right","scroll-behavior",
	"tab-size","table-layout","text-align","text-align-last","text-combine-upright","text-decoration",
	"text-decoration-color","text-decoration-line","text-decoration-style","text-indent","text-justify",
	"text-orientation","text-overflow","text-shadow","text-transform","text-underline-position",
	"top","transform","transform-origin","transform-style","transition","transition-delay",
	"transition-duration","transition-property","transition-timing-function","unicode-bidi",
	"user-select","vertical-align","visibility","white-space","widows","width","word-break",
	"word-spacing","word-wrap","writing-mode",
	NULL
};

static char * css_values[] = {
	"inline","block","inline-block","none",
	"transparent","thin","dotted","sans-serif",
	"rgb","rgba","bold","italic","underline","context-box",
	"monospace","serif","sans-serif","pre-wrap",
	"relative","baseline","hidden","solid","inherit","normal",
	"button","pointer","border-box","default","textfield",
	"collapse","top","bottom","avoid","table-header-group",
	"middle","absolute","rect","left","center","right",
	"ellipsis","nowrap","table","both","uppercase","lowercase","help",
	"static","table-cell","table-column","scroll","touch","auto",
	"not-allowed","inset","url","fixed","translate","alpha","fixed","device-width",
	"table-row",
	NULL
};

static char * css_states[] = {
	"focus","active","hover","link","visited","before","after",
	"left","right","root","empty","target","enabled","disabled","checked","invalid",
	"first-child","nth-child","not","last-child",
	NULL
};

int css_property_qualifier(int c) {
	return isalnum(c) || c == '-' || c == '@' || c == '*' || c == '!';
}

int match_prefix(struct syntax_state * state, char * prefix) {
	int i = 0;
	while (1) {
		if (prefix[i] == '\0') return 1;
		if (prefix[i] != charrel(i)) return 0;
		if (charrel(i) == -1) return 0;
		i++;
	}
}

int syn_css_calculate(struct syntax_state * state) {
	if (state->state < 1) {
		if (charat() == '/' && nextchar() == '*') {
			if (paint_c_comment(state) == 1) return 1;
		} else if (charat() == '"') {
			paint_simple_string(state);
			return 0;
		} else if (lastchar() != '.' && find_keywords(state,html_elements,FLAG_KEYWORD,css_property_qualifier)) {
			return 0;
		} else if (lastchar() != '.' && find_keywords(state,css_properties,FLAG_TYPE,css_property_qualifier)) {
			return 0;
		} else if (match_prefix(state,"-moz-")) {
			paint(5,FLAG_ESCAPE);
			while (charat() != -1 && css_property_qualifier(charat())) paint(1, FLAG_TYPE);
		} else if (match_prefix(state,"-webkit-")) {
			paint(8,FLAG_ESCAPE);
			while (charat() != -1 && css_property_qualifier(charat())) paint(1, FLAG_TYPE);
		} else if (match_prefix(state,"-ms-")) {
			paint(4,FLAG_ESCAPE);
			while (charat() != -1 && css_property_qualifier(charat())) paint(1, FLAG_TYPE);
		} else if (match_prefix(state,"-o-")) {
			paint(3,FLAG_ESCAPE);
			while (charat() != -1 && css_property_qualifier(charat())) paint(1, FLAG_TYPE);
		} else if (charat() == ':') {
			skip();
			if (find_keywords(state, css_states, FLAG_PRAGMA, css_property_qualifier)) return 0;
			while (charat() != -1 && charat() != ';') {
				if (find_keywords(state, css_values, FLAG_NUMERAL, css_property_qualifier)) {
					continue;
				} else if (charat() == '"') {
					paint_simple_string(state);
					continue;
				} else if (charat() == '{') {
					skip();
					return 0;
				} else if (charat() == '#') {
					paint(1, FLAG_NUMERAL);
					while (isxdigit(charat())) paint(1, FLAG_NUMERAL);
				} else if (isdigit(charat())) {
					while (isdigit(charat())) paint(1, FLAG_NUMERAL);
					if (charat() == '.') {
						paint(1, FLAG_NUMERAL);
						while (isdigit(charat())) paint(1, FLAG_NUMERAL);
					}
					if (charat() == '%') paint(1, FLAG_NUMERAL);
					else if (charat() == 'p' && (nextchar() == 't' || nextchar() == 'x' || nextchar() == 'c')) paint(2, FLAG_NUMERAL);
					else if ((charat() == 'e' || charat() == 'c' || charat() == 'm') && nextchar() == 'm') paint(2, FLAG_NUMERAL);
					else if (charat() == 'e' && nextchar() == 'x') paint(2, FLAG_NUMERAL);
					else if (charat() == 'i' && nextchar() == 'n') paint(2, FLAG_NUMERAL);
					else if (charat() == 'v' && (nextchar() == 'w' || nextchar() == 'h')) paint(2, FLAG_NUMERAL);
					else if (charat() == 'c' && nextchar() == 'h') paint(2, FLAG_NUMERAL);
					else if (charat() == 'r' && nextchar() == 'e' && charrel(2) == 'm') paint(3, FLAG_NUMERAL);
					else if (charat() == 'v' && nextchar() == 'm' && ((charrel(2) == 'i' && charrel(3) == 'n') || (charrel(2) == 'a' && charrel(3) == 'x'))) paint(4, FLAG_NUMERAL);
					else if (charat() == 's') paint(1, FLAG_NUMERAL);
				} else if (match_and_paint(state,"!important",FLAG_PRAGMA,css_property_qualifier)) {
					continue;
				} else if (charat() != -1) {
					skip();
				}
			}
			return 0;
		} else if (charat() == -1) {
			return -1;
		} else {
			skip();
		}
		return 0;
	} else if (state->state == 1) {
		if (paint_c_comment(state) == 1) return 1;
		return 0;
	}
	return -1;
}

char * syn_css_ext[] = {".css",NULL};

BIM_SYNTAX(css, 1)
