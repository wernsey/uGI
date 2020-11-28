/*
 * https://en.wikipedia.org/wiki/Gap_buffer
 * https://www.codeproject.com/Articles/20910/Generic-Gap-Buffer - old, but had some nice pictures
 */

#ifndef GAP_H
#define GAP_H

typedef struct GapBuffer GapBuffer;

GapBuffer *gb_create();

GapBuffer *gb_make(const char *s);

void gb_free(GapBuffer *g);

int gb_insert(GapBuffer *g, char c);

int gb_insert_str(GapBuffer *g, const char *s);

void gb_backspace(GapBuffer *g);

void gb_delete(GapBuffer *g);

void gb_left(GapBuffer *g);

void gb_right(GapBuffer *g);

void gb_up(GapBuffer *g);

void gb_down(GapBuffer *g);

void gb_up_n(GapBuffer *g, unsigned int n);

void gb_down_n(GapBuffer *g, unsigned int n);

void gb_home(GapBuffer *g);

void gb_end(GapBuffer *g);

void gb_set_cursor(GapBuffer *g, unsigned int pos);

unsigned int gb_get_cursor(GapBuffer *g);

unsigned int gb_get_column(GapBuffer *g);

unsigned int gb_get_line(GapBuffer *g);

unsigned int gb_lines(GapBuffer *g);

unsigned int gb_start_of_line(GapBuffer *g, int line);

unsigned int gb_line_len(GapBuffer *g, int line);

void gb_bof(GapBuffer *g);

void gb_eof(GapBuffer *g);

const char *gb_text(GapBuffer *g);

unsigned int gb_strlen(GapBuffer *g);

char gb_char(GapBuffer *g, unsigned int i);

#endif // GAP_H