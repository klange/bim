#ifndef _BIM_SEARCH_H
#define _BIM_SEARCH_H

#include <stdint.h>
#include "line.h"

extern int search_matches(uint32_t a, uint32_t b, int mode);
extern void perform_replacement(int line_no, uint32_t * needle, uint32_t * replacement, int col, int ignorecase, int *out_col);
extern int smart_case(uint32_t * str);
extern void find_match(int from_line, int from_col, int * out_line, int * out_col, uint32_t * str);
extern void find_match_backwards(int from_line, int from_col, int * out_line, int * out_col, uint32_t * str);
extern void rehighlight_search(line_t * line);
extern void draw_search_match(uint32_t * buffer, int redraw_buffer);
extern void search_next(void);
extern void search_prev(void);
extern void search_mode(int direction);

#endif // _BIM_SEARCH_H
