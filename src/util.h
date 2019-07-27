#ifndef _BIM_UTIL_H
#define _BIM_UTIL_H

#define bim_getch() bim_getch_timeout(200)
extern int bim_getch_timeout(int timeout);
extern void bim_unget(int c);
extern char * file_basename(char * file);

extern void rehighlight_search(line_t * line);

#define NAV_BUFFER_MAX 10
extern char nav_buf[NAV_BUFFER_MAX+1];
extern int nav_buffer;

extern int git_examine(char * filename);
extern int read_one_character(char * message);
extern int is_all_numbers(const char * c);

extern int is_whitespace(int codepoint);
extern int is_normal(int codepoint);
extern int is_special(int codepoint);

#define BLOCK_SIZE 4096

#endif // _BIM_UTIL_H
