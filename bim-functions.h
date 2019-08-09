#ifndef _BIM_FUNCTIONS_H
#define _BIM_FUNCTIONS_H

extern const char * flag_to_color(int _flag);
extern void redraw_line(int x);
extern int git_examine(char * filename);
extern void search_next(void);
extern void set_preferred_column(void);
extern void quit(const char * message);
extern void rehighlight_search(line_t * line);
extern void try_to_center();
extern int read_one_character(char * message);

extern void render_error(char * message, ...);
extern void render_status_message(char * message, ...);
extern void render_commandline_message(char * message, ...);

extern void redraw_all(void);
extern void redraw_most(void);
extern void unsplit(void);
extern void set_modified(void);
extern void update_split_size(void);
extern void place_cursor_actual(void);

extern void add_colorscheme(const char * name, void (*load)(void));

/* biminfo */
extern FILE * open_biminfo(void);
extern int fetch_from_biminfo(buffer_t * buf);
extern int update_biminfo(buffer_t * buf);

/* buffer */
extern buffer_t * buffer_new(void);
extern buffer_t * buffer_close(buffer_t * buf);
extern line_t * line_insert(line_t * line, char_t c, int offset, int lineno);
extern void line_delete(line_t * line, int offset, int lineno);
extern void line_replace(line_t * line, char_t _c, int offset, int lineno);
extern line_t ** remove_line(line_t ** lines, int offset);
extern line_t ** add_line(line_t ** lines, int offset);
extern void replace_line(line_t ** lines, int offset, line_t * replacement);
extern line_t ** merge_lines(line_t ** lines, int lineb);
extern line_t ** split_line(line_t ** lines, int line, int split);
extern int line_ends_with_brace(line_t * line);
extern int line_is_comment(line_t * line);
extern void add_indent(int new_line, int old_line, int ignore_brace);
extern void setup_buffer(buffer_t * env);
extern void add_buffer(uint8_t * buf, int size);
extern void close_buffer(void);

/* history */
extern void recursive_history_free(history_t * root);
extern void set_history_break(void);
extern void undo_history(void);
extern void redo_history(void);

/* syntax */
extern int find_keywords(struct syntax_state * state, char ** keywords, int flag, int (*keyword_qualifier)(int c));
extern int match_and_paint(struct syntax_state * state, const char * keyword, int flag, int (*keyword_qualifier)(int c));
extern void paint_simple_string(struct syntax_state * state);
extern int common_comment_buzzwords(struct syntax_state * state);
extern int paint_comment(struct syntax_state * state);
extern int match_forward(struct syntax_state * state, char * c);
extern struct syntax_definition * find_syntax_calculator(const char * name);
extern void recalculate_syntax(line_t * line, int line_no);
extern void recalculate_tabs(line_t * line);
extern struct syntax_definition * match_syntax(char * file);
extern void set_syntax_by_name(const char * name);
extern void add_syntax(struct syntax_definition def);

/* terminal */
extern void bim_unget(int c);
#define bim_getch() bim_getch_timeout(200)
extern int bim_getch_timeout(int timeout);
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
extern void store_cursor(void);
extern void restore_cursor(void);
extern void mouse_enable(void);
extern void mouse_disable(void);
extern void shift_up(int amount);
extern void shift_down(int amount);
extern void set_alternate_screen(void);
extern void unset_alternate_screen(void);
extern void update_title(void);
extern void update_screen_size(void);
extern void SIGWINCH_handler(int sig);
extern void SIGTSTP_handler(int sig);
extern void SIGCONT_handler(int sig);

/* themes */
extern void add_colorscheme(const char * name, void (*load)(void));
extern const char * flag_to_color(int _flag);

/* unicode */
extern int to_eight(uint32_t codepoint, char * out);
extern int codepoint_width(wchar_t codepoint);

#endif /* _BIM_FUNCTIONS_H */
