#include "bim-core.h"
#include "bim-functions.h"
#include "bim-syntax.h"

uint32_t state = 0;

/**
 * Convert codepoint to utf-8 string
 */
int to_eight(uint32_t codepoint, char * out) {
	memset(out, 0x00, 7);

	if (codepoint < 0x0080) {
		out[0] = (char)codepoint;
	} else if (codepoint < 0x0800) {
		out[0] = 0xC0 | (codepoint >> 6);
		out[1] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x10000) {
		out[0] = 0xE0 | (codepoint >> 12);
		out[1] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[2] = 0x80 | (codepoint & 0x3F);
	} else if (codepoint < 0x200000) {
		out[0] = 0xF0 | (codepoint >> 18);
		out[1] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[3] = 0x80 | ((codepoint) & 0x3F);
	} else if (codepoint < 0x4000000) {
		out[0] = 0xF8 | (codepoint >> 24);
		out[1] = 0x80 | (codepoint >> 18);
		out[2] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[4] = 0x80 | ((codepoint) & 0x3F);
	} else {
		out[0] = 0xF8 | (codepoint >> 30);
		out[1] = 0x80 | ((codepoint >> 24) & 0x3F);
		out[2] = 0x80 | ((codepoint >> 18) & 0x3F);
		out[3] = 0x80 | ((codepoint >> 12) & 0x3F);
		out[4] = 0x80 | ((codepoint >> 6) & 0x3F);
		out[5] = 0x80 | ((codepoint) & 0x3F);
	}

	return strlen(out);
}

/**
 * Get the presentation width of a codepoint
 */
int codepoint_width(wchar_t codepoint) {
	if (codepoint == '\t') {
		return 1; /* Recalculate later */
	}
	if (codepoint < 32) {
		/* We render these as ^@ */
		return 2;
	}
	if (codepoint == 0x7F) {
		/* Renders as ^? */
		return 2;
	}
	if (codepoint > 0x7f && codepoint < 0xa0) {
		/* Upper control bytes <xx> */
		return 4;
	}
	if (codepoint == 0xa0) {
		/* Non-breaking space _ */
		return 1;
	}
	/* Skip wcwidth for anything under 256 */
	if (codepoint > 256) {
		if (global_config.can_unicode) {
			/* Higher codepoints may be wider (eg. Japanese) */
			int out = wcwidth(codepoint);
			if (out >= 1) return out;
		}
		/* Invalid character, render as [U+ABCD] or [U+ABCDEF] */
		return (codepoint < 0x10000) ? 8 : 10;
	}
	return 1;
}

