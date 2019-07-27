#ifndef _BIM_DISPLAY_H
#define _BIM_DISPLAY_H

#include "buffer.h"
#include "line.h"

extern void try_to_center();
extern int draw_tab_name(buffer_t * _env, char * out);
extern void redraw_tabbar(void);
extern int log_base_10(unsigned int v);
extern void render_line(line_t * line, int width, int offset, int line_no);
extern int num_width(void);
extern void draw_line_number(int x);
extern void recalculate_current_line(void);
extern void redraw_line(int j, int x);
extern void draw_excess_line(int j);
extern void redraw_text(void);
extern void redraw_alt_buffer(buffer_t * buf);
extern void redraw_statusbar(void);
extern void redraw_nav_buffer();
extern void redraw_commandline(void);
extern void render_commandline_message(char * message, ...);
extern void redraw_all(void);
extern void redraw_most(void);
extern void unsplit(void);
extern void use_left_buffer(void);
extern void use_right_buffer(void);
extern void render_status_message(char * message, ...);
extern void render_error(char * message, ...);
extern void update_split_size(void);

#endif // _BIM_DISPLAY_H
