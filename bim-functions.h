#ifndef _BIM_FUNCTIONS_H
#define _BIM_FUNCTIONS_H

extern const char * flag_to_color(int _flag);
extern void redraw_line(int x);
extern int git_examine(char * filename);
extern void search_next(void);
extern void set_preferred_column(void);
extern void quit(const char * message);
extern void close_buffer(void);
extern void set_syntax_by_name(const char * name);
extern void rehighlight_search(line_t * line);
extern void try_to_center();
extern int read_one_character(char * message);
extern void bim_unget(int c);
#define bim_getch() bim_getch_timeout(200)
extern int bim_getch_timeout(int timeout);
extern buffer_t * buffer_new(void);
extern FILE * open_biminfo(void);
extern int fetch_from_biminfo(buffer_t * buf);
extern int update_biminfo(buffer_t * buf);
extern buffer_t * buffer_close(buffer_t * buf);

extern void add_colorscheme(const char * name, void (*load)(void));
extern void add_syntax(struct syntax_definition def);

#endif /* _BIM_FUNCTIONS_H */
