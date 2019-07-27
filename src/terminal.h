#ifndef _BIM_TERMINAL_H
#define _BIM_TERMINAL_H

#define ENTER_KEY     '\r'
#define LINE_FEED     '\n'
#define BACKSPACE_KEY 0x08
#define DELETE_KEY    0x7F

extern void get_initial_termios(void);
extern void set_unbuffered(void);
extern void set_buffered(void);
extern void place_cursor(int x, int y);
extern void set_colors(const char * fg, const char * bg);
extern void set_fg_color(const char * fg);
extern void clear_to_end(void);
extern void paint_line(const char * bg);
extern void set_bold(void);
extern void unset_bold(void);
extern void set_underline(void);
extern void unset_underline(void);
extern void reset(void);
extern void clear_screen(void);
extern void hide_cursor(void);
extern void show_cursor(void);
extern void mouse_enable(void);
extern void mouse_disable(void);
extern void shift_up(int amount);
extern void shift_down(int amount);
extern void set_alternate_screen(void);
extern void unset_alternate_screen(void);
extern void update_title(void);
extern void init_terminal(void);
extern void detect_weird_terminals(void);

#endif // _BIM_TERMINAL_H
