/*
I didn't set out initially to create a text editor, so
please excuse me that the code in here is a bit rough.

TODO:
    * If you click on a line containing tabs it really messes up the cursor position
    * The dummy lexer is borked, and the API needs some work
    * clicking on the scrollbar does nothing

There are a couple of features I'd like to see:
   - Auto-indent when pressing enter
   - Also, tabs really mess up the UP/DOWN key handling.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../ugi.h"
#include "richedit.h"

#include "gap.h"

// Commodore-64 palette;
// See https://en.wikipedia.org/wiki/List_of_8-bit_computer_hardware_palettes#C-64
static unsigned int palette[] = {
    0x000000, // 0 - black
    0xFFFFFF, // 1 - white
    0x883932, // 2 - red
    0x67B6BD, // 3 - cyan
    0x8B3F96, // 4 - purple
    0x55A049, // 5 - green
    0x40318D, // 6 - blue
    0xBFCE72, // 7 - yellow
    0x8B5429, // 8 - orange
    0x574200, // 9 - brown
    0xB86962, // 10 - light red
    0x505050, // 11 - dark grey
    0x787878, // 12 - grey
    0x94E089, // 13 - light green
    0x7869C4, // 14 - light blue
    0x9F9F9F, // 15 - light grey
};

static const char *keywords[] = {
    /* C Keywords: http://www.c4learn.com/c-programming/c-keywords/ */
    "auto", "double", "int", "struct",
    "break", "else", "long", "switch",
    "case", "enum", "register", "typedef",
    "char", "extern", "return", "union",
    "const", "float", "short", "unsigned",
    "continue", "for", "signed", "void",
    "default", "goto", "sizeof", "volatile",
    "do", "if", "static", "while",
    NULL
};

static void dummy_lexer(GapBuffer *g, char *colors, int len) {
    int i, sol = 1;
    for(i = 0; i < len; i++) {
        char c = gb_char(g, i);
        if(sol) {
            while(i < len - 1 && isspace(c)) i++;
            if(i < len && c == '#') {
                while(i < len && c != '\n')
                    colors[i++] = GREEN;
            continue;
            }
        }

        if(strchr("\"'", c)) {
            char term = c;
            colors[i] = RED;
            c = gb_char(g, i++);
            while(c && c != term && c != '\n') {
                if(c == '\\') {
                    colors[i] = RED;
                    c = gb_char(g, i++);
                }
                colors[i] = RED;
                c = gb_char(g, i++);
            }
            colors[i] = RED;
        } else if(c == '/' && gb_char(g, i + 1) == '/') {
            colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            while((c = gb_char(g, i)) && c != '\n') {
                colors[i++] = LIGHT_GREY;
            }
        } else if(c == '/' && gb_char(g, i + 1) == '*') {
            colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            while(gb_char(g,i) == '*' && gb_char(g,i+1) == '/')
                colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            colors[i] = LIGHT_GREY;
        } else if(isalpha(c) || c == '_') {
            int s = i, k = LIGHT_BLUE;
            while((c = gb_char(g, i)) && (isalnum(c) || c == '_'))
                i++;

            /* FIXME: I need some gb_memcmp()
            int len = i - s, j;
            for(j = 0; keywords[j]; j++) {
                if(len == strlen(keywords[j]) && !memcmp(keywords[j], text+s, len)) {
                    k = CYAN;
                    break;
                }
            }
            */
           (void)keywords;

            while(s < i)
                colors[s++] = k;
            i--;
        } else if(isdigit(c)) {
            if(c == '0' && tolower(gb_char(g, i+1) == 'x')) {
                colors[i++] = ORANGE;
                colors[i++] = ORANGE;
                while((c = gb_char(g, i)) && strchr("1234567890abcdef", tolower(c)))
                    colors[i++] = ORANGE;
            } else {
                while((c = gb_char(g, i)) && strchr("1234567890.eE", c))
                    colors[i++] = ORANGE;
            }
        } else
            colors[i] = LIGHT_GREY;
        sol = (c == '\n');
    }
}

void rt_set_text(uWidget *W, const char *txt) {
    assert(uu_get_widget_fun(W) == uw_richedit);
    GapBuffer *g = uu_get_data(W);
    gb_free(g);
    g = gb_make(txt);
    gb_bof(g);
    uu_set_data(W, g);
}

int uw_richedit(uWidget *W, int msg, int param) {

    int cw = ud_text_width(uu_get_font(W), " ");
    int ch = ud_text_height(uu_get_font(W), " ");

    uRect pos = uu_get_position(W);

    if(msg == UM_START) {
        uu_set_attr_i(W, "scroll", 0);
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "blink", 0);

        //uu_set_action(W, dummy_lexer);
        (void)dummy_lexer;

        uu_set_data(W, gb_create());

        return UW_OK;
    } else if(msg == UM_DRAW) {

        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        int scroll = uu_get_attr_i(W, "scroll");
        int blink = uu_get_attr_i(W, "blink");

        GapBuffer *g = uu_get_data(W);
        unsigned int cursor = gb_get_cursor(g);

        uu_clear_widget(W, bg);

        unsigned int n = gb_lines(g);

        /* This is nasty */
        int len = gb_strlen(g);
        char *colors = malloc(len + 1);
        memset(colors, 0, len + 1);

        lexer_fun lexer = uu_get_action(W);
        if(lexer)
            lexer(g, colors, len);

        int x = pos.x + 2;
        int y = pos.y + 2;

        ud_clip(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);

        char c;
        int i = gb_start_of_line(g, scroll);
        while(i < len && (c = gb_char(g, i))) {
            if(uu_get_flag(W,UF_FOCUS) && i == cursor && blink) {
                ud_set_color(fg);
                ud_fill_box(x, y - 2, x+2, y + 8);
            }
            if(c == '\n') {
                x = pos.x + 2;
                y += ch + 2;
                if(y > pos.y + pos.h) break;
            } else if(c == '\t') {
                x += cw * 4;
                if(y > pos.y + pos.h) break;
            } else if(c == '\r') {
            } else {
                if(colors[i]) {
                    unsigned int fg = palette[colors[i] & 0x0F];
                    unsigned int bg = palette[(colors[i] >> 8) & 0x0F];
                    (void)bg; // FIXME: background
                    ud_set_color(fg);
                } else {
                    ud_set_color(ugi_get_default_foreground());
                }
                uu_printf(x, y, "%c", c);
                x += cw;
            }
            i++;
        }
        uu_unclip();

        free(colors);

        ud_set_color(fg);
        if(uu_get_flag(W,UF_FOCUS) && i == cursor && blink) {
            // Cursor at end of text
            ud_fill_box(x, y - 2, x+2, y + 8);
        }

        /*
        ud_dither_box(pos.x + pos.w - 6, pos.y + 1, pos.x + pos.w, pos.y + pos.h);

        int nlf = pos.h/(ch + 2) - 1;
        if(nlf < n) {
            int nl = n - nlf;
            int ph = pos.h * pos.h / ((ch + 2) * n);
            int pp = scroll * (pos.h - ph) / nl;
            if(ph > pos.h - 2) {
                ph = pos.h - 1;
                pp = 0;
            }
            x = pos.x + pos.w - 6;
            ud_fill_box(x, pos.y + pp + 1, x + 6 - 2, pos.y + pp + ph - 2);
        }
        */
       uu_draw_scrollbar(pos.x + pos.w - 6, pos.y + 1, 6, pos.h-2, scroll, n, ch + 2);

        ud_set_color(fg);
        if(uu_get_flag(W, UF_FOCUS))
            uu_highlight_widget(W);

        ud_box(pos.x, pos.y, pos.x + pos.w - 1, pos.y + pos.h - 1);

    } else if(msg == UM_CLICK) {

        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;
        int scroll = uu_get_attr_i(W, "scroll");

        GapBuffer *g = uu_get_data(W);
        int line = (my - pos.y)/(ch + 2) + scroll;

        unsigned int nl = gb_lines(g);
        if(line >= nl)
            line = nl;

        unsigned int ll = gb_line_len(g, line);
        int col = (mx - pos.x - 2)/(cw);

        //SDL_Log("Click: line: %d col: %d/%u", line, col, ll);
        if(col > ll)
            col = ll;

        unsigned int p = gb_start_of_line(g, line) + col;
        gb_set_cursor(g, p);

        uu_set_attr_i(W, "time", 1<<8); /* shows the cursor */
        return UW_HANDLED;
    } else if(msg == UM_KEY || msg ==  UM_CHAR) {

        //int cursor, line, col;
        //int scroll = uu_get_attr_i(W, "scroll");
        //int len, i;

        GapBuffer *g = uu_get_data(W);

        if(msg == UM_KEY) {
            switch(param)  {
                /* TODO: Tabs really mess up the UP/DOWN key handling. */
                case UK_UP:
                    gb_up(g);
                break;
                case UK_DOWN:
                    gb_down(g);
                break;
                case UK_PAGEUP:
                    gb_up_n(g, pos.h / (ch + 2) - 2);
                break;
                case UK_PAGEDOWN:
                    gb_down_n(g, pos.h / (ch + 2) - 2);
                break;
                case UK_LEFT:
                    gb_left(g);
                break;
                case UK_RIGHT:
                    gb_right(g);
                break;
                case UK_HOME:
                    gb_home(g);
                break;
                case UK_END:
                    gb_end(g);
                break;
                case UK_DELETE:
                    gb_delete(g);
                break;
            }
        } else {

            if(param == '\r')  param = '\n';

            if(param == '\b') {
                gb_backspace(g);
            } else if(isprint(param) || isspace(param)) {
                gb_insert(g, param);
            }

        }

        /* Ensure the cursor is scrolled in view */
        int e = (pos.h - 2) / (ch + 2) - 1;
        unsigned int line = gb_get_line(g);
        unsigned int n = gb_lines(g);
        int scroll = uu_get_attr_i(W, "scroll");
        if(e >= n - 1) {
            uu_set_attr_i(W, "scroll", 0);
        } else if(line < scroll) {
            uu_set_attr_i(W, "scroll", line);
        } else if(line > scroll + e) {
            scroll = line - e;
            if(scroll < 0)
                scroll = 0;
            uu_set_attr_i(W, "scroll", scroll);
        }

        uu_set_attr_i(W, "time", 1<<8);

        return UW_HANDLED;
    } else if(msg == UM_TICK) {
        if(uu_get_flag(W, UF_FOCUS)) {
            int elapsed = uu_get_attr_i(W, "time") + param;
            uu_set_attr_i(W, "time", elapsed);
            int blink = uu_get_attr_i(W, "blink");
            int nblink = (elapsed >> 8) & 0x01;
            if(blink ^ nblink) {
                uu_set_attr_i(W, "blink", nblink);
                return UW_OK;
            }
        }
    } else {
        return uw_button(W, msg, param);
    }
    return UW_OK;
}
