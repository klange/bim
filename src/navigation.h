#ifndef _BIM_NAVIGATION_H
#define _BIM_NAVIGATION_H

extern void highlight_matching_paren(void);
extern void place_cursor_actual(void);
extern void goto_line(int line);
extern void find_matching_paren(int * out_line, int * out_col, int in_col);
extern void set_preferred_column(void);
extern void cursor_down(void);
extern void cursor_up(void);
extern void cursor_left(void);
extern void cursor_right(void);
extern void cursor_home(void);
extern void cursor_end(void);
extern void previous_tab(void);
extern void next_tab(void);
extern void handle_mouse(void);
extern void word_left(void);
extern void big_word_left(void);
extern void word_right(void);
extern void big_word_right(void);


#endif // _BIM_NAVIGATION_H
