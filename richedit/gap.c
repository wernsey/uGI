#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gap.h"

#define DEFAULT_SIZE    4

#if 1
#define TRACE(x)
#else
#define TRACE(x) do{x;}while(0)
#endif

struct GapBuffer {
    char *b;        // The buffer itself
    unsigned int a;       // The size allocated for the buffer
    unsigned int s, e;    // The start and end indexes of the gap
};

static GapBuffer *create(unsigned int a) {
    GapBuffer *g = malloc(sizeof *g);
    if(!g)
        return NULL;
    g->a = a;
    g->b = malloc(g->a + 1);
    if(!g->b) {
        free(g);
        return NULL;
    }
    g->b[g->a] = '\0';
    g->s = 0;
    g->e = g->a;

    return g;
}

GapBuffer *gb_create() {
    return create(DEFAULT_SIZE);
}

GapBuffer *gb_make(const char *s) {
    unsigned int len = strlen(s);
    unsigned int a = DEFAULT_SIZE;
    while(a < len) a <<= 1;
    GapBuffer *g = create(a);
    memcpy(g->b, s, len);
    g->s = len;
    return g;
}

void gb_free(GapBuffer *g) {
    free(g->b);
    free(g);
}

static int grow(GapBuffer *g, unsigned int new_a) {
    unsigned int len = g->a - g->e;
    char *new_b = realloc(g->b, new_a + 1);
    if(!new_b)
        return 0;
    new_b[new_a] = '\0';
    g->b = new_b;
    memmove(g->b + new_a - len, g->b + g->e, len);
    g->e = new_a - len;
    g->a = new_a;
    return 1;
}

int gb_insert(GapBuffer *g, char c) {
    TRACE(printf("insert '%c'\n", c < ' '?'*':c));
    assert(g->s <= g->e);
    if(g->s == g->e) {
        if(!grow(g, g->a << 1))
            return 0;
    }
    g->b[g->s++] = c;
    return 1;
}

int gb_insert_str(GapBuffer *g, const char *s) {
    TRACE(printf("insert '%s'\n", s));
    unsigned int len = strlen(s);
    unsigned int gs = g->e - g->s;
    if(len > gs) {
        unsigned int a = g->a << 1;
        unsigned int n = g->a - gs;
        while(n + len > a)
            a <<= 1;
        if(!grow(g, a))
            return 0;
    }
    memcpy(g->b + g->s, s, len);
    g->s += len;

    return 1;
}

void gb_backspace(GapBuffer *g) {
    TRACE(printf("backspace\n"));
    if(g->s > 0) {
        g->s--;
    }
}

void gb_delete(GapBuffer *g) {
    TRACE(printf("delete\n"));
    if(g->e < g->a) {
        g->e++;
    }
}

void gb_left(GapBuffer *g) {
    TRACE(printf("left\n"));
    if(g->s > 0) {
        g->b[--g->e] = g->b[--g->s];
    }
}

void gb_right(GapBuffer *g) {
    TRACE(printf("right\n"));
    if(g->e < g->a) {
        g->b[g->s++] = g->b[g->e++];
    }
}

void gb_up_n(GapBuffer *g, unsigned int n) {
    TRACE(printf("up %u\n", n));
    unsigned int c = 0, i = g->s;
    while(i > 0 && g->b[i-1] != '\n') {
        i--;
        c++;
    }
    if(i == 0)
        return;
    while(n > 0 && --i > 0) {
        if(g->b[i-1] == '\n')
            n--;
    }
    while(c > 0) {
        if(g->b[i] == '\n')
            break;
        i++;
        c--;
    }
    //gb_set_cursor(g, i);
    unsigned int len = g->s - i;
    memmove(g->b + g->e - len, g->b + i, len);
    g->s -= len;
    g->e -= len;
}

void gb_up(GapBuffer *g) {
    gb_up_n(g, 1);
}

void gb_down_n(GapBuffer *g, unsigned int n) {
    TRACE(printf("down %u\n", n));
    unsigned int c = 0, i = g->s;
    while(i > 0 && g->b[i-1] != '\n') {
        i--;
        c++;
    }
    unsigned int ll = i;
    i = g->e;
    while(n > 0) {
        if(i == g->a) {
            i = ll;
            if(i > g->e) {
                i -= (g->e - g->s);
                gb_set_cursor(g, i);
            }
            return;
        }
        if(g->b[i] == '\n') {
            n--;
            ll = i+1;
        }
        i++;
    }
    while(c > 0 && i < g->a) {
        if(g->b[i] == '\n')
            break;
        i++;
        c--;
    }
    //gb_set_cursor(g, i - (g->e - g->s));
    unsigned int len = i - g->e;
    memmove(g->b + g->s, g->b + g->e, len);
    g->s += len;
    g->e += len;
}

void gb_down(GapBuffer *g) {
    gb_down_n(g, 1);
}

void gb_home(GapBuffer *g) {
    TRACE(printf("home\n"));
    while(g->s > 0) {
        if(g->b[g->s - 1] == '\n')
            break;
        g->b[--g->e] = g->b[--g->s];
    }
}

void gb_end(GapBuffer *g) {
    TRACE(printf("end\n"));
    while(g->e < g->a) {
        if(g->b[g->e] == '\n')
            break;
        g->b[g->s++] = g->b[g->e++];
    }
}

void gb_bof(GapBuffer *g) {
    TRACE(printf("bof\n"));
    if(g->s > 0) {
        unsigned int len = g->s;
        memmove(g->b + g->e - len, g->b, len);
        g->s -= len;
        g->e -= len;
    }
}

void gb_eof(GapBuffer *g) {
    TRACE(printf("eof\n"));
    if(g->a - g->e > 0) {
        unsigned int len = g->a - g->e;
        memmove(g->b + g->s, g->b + g->e, len);
        g->s += len;
        g->e += len;
    }
}

void gb_set_cursor(GapBuffer *g, unsigned int pos) {
    TRACE(printf("cursor %u\n", pos));
    unsigned int len = g->e - g->s;
    if(pos > g->a - len)
        pos = g->a - len;
    if(pos < g->s) {
        len = g->s - pos;
        memmove(g->b + g->e - len, g->b + pos, len);
        g->s -= len;
        g->e -= len;
    } else {
        len = pos - g->s;
        memmove(g->b + g->s, g->b + g->e, len);
        g->s += len;
        g->e += len;
    }
}

unsigned int gb_get_cursor(GapBuffer *g) {
    return g->s;
}

unsigned int gb_get_column(GapBuffer *g) {
    unsigned int c = 0, i = g->s;
    while(i > 0 && g->b[i-1] != '\n') {
        i--;
        c++;
    }
    return c;
}

unsigned int gb_get_line(GapBuffer *g) {
    unsigned int r = 0, i = g->s;
    while(i > 0) {
        if(g->b[i-1] == '\n')
            r++;
        i--;
    }
    return r;
}

const char *gb_text(GapBuffer *g) {
    gb_eof(g);
    g->b[g->s] = '\0';
    return g->b;
}

unsigned int gb_strlen(GapBuffer *g) {
    return g->a - (g->e - g->s);
}

unsigned int gb_lines(GapBuffer *g) {
    unsigned int i = 0, n = 0;
    while(i < g->a) {
        if(i == g->s)
            i = g->e;
        if(g->b[i] == '\n')
            n++;
        i++;
    }
    return n;
}

unsigned int gb_start_of_line(GapBuffer *g, int line) {
    unsigned int n = 0, i = 0;
    if(line == 0) return 0;
    while(i < g->a) {
        if(i == g->s)
            i = g->e;
        if(g->b[i] == '\n') {
            n++;
            if(n == line) {
                if(i > g->s)
                    return i - (g->e - g->s) + 1;
                else
                    return i + 1;
            }
        }
        i++;
    }
    return 0;
}

unsigned int gb_line_len(GapBuffer *g, int line) {
    unsigned int i = gb_start_of_line(g, line), c = 0;
    while(i < gb_strlen(g) && gb_char(g,i) != '\n')
        i++, c++;
    return c;
}

char gb_char(GapBuffer *g, unsigned int i) {
    if(i >= g->s) {
        i += g->e - g->s;
        if(i >= g->a)
            return '\0';
    }
    return g->b[i];
}

#ifdef TEST

void gb_show(GapBuffer *g) {
    unsigned int i;
    for(i = 0; i < g->a; i++) {
        if(i >= g->s && i < g->e)
            putchar('_');
        else if(g->b[i] < ' ')
            putchar('*');
        else
            putchar(g->b[i]);
    }
    putchar('\n');
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    GapBuffer *g = gb_create();
#if 0

    gb_free(g);
    g = gb_make("line 1\nline 2\nline 3\nline 4");

    gb_set_cursor(g, 17);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));

    /*
    gb_set_cursor(g, 2);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));

    gb_set_cursor(g, 14);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));
    */

    gb_home(g);
    gb_up_n(g, 1);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));

    gb_up(g);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));

    gb_end(g);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));

    /*
    gb_up_n(g,6);
    gb_show(g);
    printf("line: %u; col: %u\n", gb_get_line(g), gb_get_column(g));
    */


#elif 0
    gb_insert(g, 'A');
    gb_show(g);
    gb_insert(g, 'B');
    gb_show(g);
    gb_insert(g, 'C');
    gb_left(g);
    gb_left(g);
    gb_show(g);
    gb_insert_str(g, "Hello World To You");
    gb_show(g);
    gb_insert_str(g, "------------");
    gb_show(g);
#else
    gb_show(g);
    gb_insert(g, 'A');
    gb_show(g);
    gb_insert(g, 'B');
    gb_show(g);
    gb_insert(g, 'C');
    gb_show(g);
    gb_insert(g, 'D');
    gb_show(g);
    gb_backspace(g);
    gb_show(g);
    gb_insert(g, 'D');
    gb_show(g);
    gb_insert(g, 'E');
    gb_show(g);
    gb_left(g);
    gb_show(g);
    gb_insert(g, 'F');
    gb_show(g);
    gb_left(g);
    gb_left(g);
    gb_left(g);
    gb_show(g);
    gb_insert(g, 'G');
    gb_show(g);
    gb_insert(g, '\n');
    gb_show(g);
    gb_insert(g, 'H');
    gb_show(g);
    gb_insert(g, 'I');
    gb_show(g);
    gb_right(g);
    gb_show(g);
    gb_delete(g);
    gb_show(g);
    gb_home(g);
    gb_show(g);
    gb_left(g);
    gb_show(g);
    gb_home(g);
    gb_show(g);
    gb_end(g);
    gb_show(g);
    gb_right(g);
    gb_show(g);
    gb_end(g);
    gb_show(g);
    gb_set_cursor(g, 2);
    gb_show(g);
    gb_set_cursor(g, 4);
    gb_show(g);
    gb_set_cursor(g, 19);
    gb_show(g);
    gb_set_cursor(g, 0);
    gb_show(g);
    gb_set_cursor(g, 5);
    gb_show(g);
    gb_bof(g);
    gb_show(g);
    gb_eof(g);
    gb_show(g);

    printf("text: \"%s\"\n", gb_text(g));

    gb_set_cursor(g, 5);
    printf("strlen: %u\n", gb_strlen(g));
    printf("cursor: %u\n", gb_get_cursor(g));
    gb_show(g);
#endif

    /*
    int i;
    for(i = 0; i < gb_strlen(g); i++)
        printf("char at(%2d): '%c'\n", i,  gb_char(g, i));
    */

    gb_free(g);
    return 0;
}
#endif
