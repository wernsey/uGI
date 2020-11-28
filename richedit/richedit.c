/*
I didn't set out initially to create a text editor, so
please excuse me that the code in here is a bit rough.

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

static void do_lexer(uWidget *W) {
    GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
    int len = gb_strlen(g);
    unsigned char *colors = malloc(len + 1);
    memset(colors, 0, len + 1);

    lexer_fun lexer = uu_get_attr_p(W, "lexer");
    if(!lexer)
        return;

    lexer(W, colors, len);
    void *p = uu_get_attr_p(W, "_colors");
    if(p)
        free(p);
    uu_set_attr_p(W, "_colors", colors);
}

void rt_set_text(uWidget *W, const char *txt) {
    assert(uu_get_widget_fun(W) == uw_richedit);
    GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
    gb_free(g);
    g = gb_make(txt);
    gb_bof(g);
    uu_set_attr_p(W, "_gapbuffer", g);
    do_lexer(W);
}

char rt_char(uWidget *W, unsigned int i) {
    GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
    return gb_char(g, i);
}

int uw_richedit(uWidget *W, int msg, int param) {

    int cw = ud_text_width(uu_get_font(W), " ");
    int ch = ud_text_height(uu_get_font(W), " ");

    uRect pos = uu_get_position(W);

    if(msg == UM_START) {
        uu_set_attr_i(W, "scroll", 0);
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "blink", 0);

        uu_set_attr_p(W, "_gapbuffer", gb_create());

        return UW_OK;
    } else if(msg == UM_END) {
        void *p = uu_get_attr_p(W, "_colors");
        if(p)
            free(p);
        GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
        gb_free(g);
    } else if(msg == UM_DRAW) {

        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        int scroll = uu_get_attr_i(W, "scroll");
        int blink = uu_get_attr_i(W, "blink");

        GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
        unsigned int cursor = gb_get_cursor(g);
        int len = gb_strlen(g);

        uu_clear_widget(W, bg);

        unsigned int n = gb_lines(g);

        unsigned char *colors = uu_get_attr_p(W, "_colors");

        int x = pos.x + 2;
        int y = pos.y + 2;

        ud_clip(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);

        char c;
        int i = gb_start_of_line(g, scroll);
        while(i < len && (c = gb_char(g, i))) {
            int ox = x, oy = y;

            unsigned int fg = LIGHT_BLUE, bg = 0;
            if(colors) {
                fg = palette[colors[i] & 0x0F];
                bg = palette[(colors[i] >> 4) & 0x0F];
            }

            if(c == '\n') {
                x = pos.x + 2;
                y += ch + 2;
                if(y > pos.y + pos.h) break;
            } else if(c == '\t') {
                if(bg) {
                    // Unfortunately you cannot use black in the background
                    ud_set_color(bg);
                    ud_fill_box(x,y,x+cw * 4,y+ch+1);
                }
                x += cw * 4;
                if(y > pos.y + pos.h) break;
            } else if(c == '\r') {
            } else {
                    if(bg) {
                        // Unfortunately you cannot use black in the background
                        ud_set_color(bg);
                        ud_fill_box(x,y-1,x+cw,y+ch);
                    }
                    ud_set_color(fg);

                uu_printf(x, y, "%c", c);
                x += cw;
            }
            if(uu_get_flag(W,UA_FOCUS) && i == cursor && blink) {
                ud_set_color(palette[LIGHT_BLUE]);
                ud_fill_box(ox, oy - 2, ox+2, oy + 8);
            }
            i++;
        }
        uu_unclip();

        ud_set_color(fg);
        if(uu_get_flag(W,UA_FOCUS) && i == cursor && blink) {
            // Cursor at end of text
            ud_set_color(palette[LIGHT_BLUE]);
            ud_fill_box(x, y - 2, x+2, y + 8);
        }

        uu_draw_scrollbar(pos.x + pos.w - 6, pos.y + 1, 6, pos.h-2, scroll, n, ch + 2);

        ud_set_color(fg);
        if(uu_get_flag(W, UA_FOCUS))
            uu_highlight_widget(W);

        ud_box(pos.x, pos.y, pos.x + pos.w - 1, pos.y + pos.h - 1);

    } else if(msg == UM_CLICK) {

        GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;
        int scroll = uu_get_attr_i(W, "scroll");
        unsigned int nl = gb_lines(g);

        int x = pos.x + pos.w - 6 - 1;
        if(mx > x) {

            scroll = uu_click_scrollbar(mx, my, pos.x + pos.w - 6, pos.y, 6, pos.h, scroll, nl, ch + 2);
            uu_set_attr_i(W, "scroll", scroll);

        } else {
            int line = (my - pos.y)/(ch + 2) + scroll;

            if(line >= nl)
                line = nl;

            unsigned int p = gb_start_of_line(g, line);
            int col = 0;
            while(col < (mx - pos.x - 2)/cw) {
                char c = gb_char(g, p);
                if(c == '\n')
                    break;
                else if(c == '\t')
                    col += 4;
                else
                    col++;
                p++;
            }

            gb_set_cursor(g, p);
        }

        uu_set_attr_i(W, "time", 1<<8); /* shows the cursor */
        return UW_HANDLED;
    } else if(msg == UM_KEY || msg ==  UM_CHAR) {

        GapBuffer *g = uu_get_attr_p(W, "_gapbuffer");;

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
                    do_lexer(W);
                break;
            }
        } else {

            if(param == '\r')  param = '\n';

            if(param == '\b') {
                gb_backspace(g);
                do_lexer(W);
            } else if(isprint(param) || isspace(param)) {
                gb_insert(g, param);
                do_lexer(W);
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
        if(uu_get_flag(W, UA_FOCUS)) {
            int elapsed = uu_get_attr_i(W, "time") + param;
            uu_set_attr_i(W, "time", elapsed);
            int blink = uu_get_attr_i(W, "blink");
            int nblink = (elapsed >> 8) & 0x01;
            if(blink ^ nblink) {
                uu_set_attr_i(W, "blink", nblink);
                return UW_DIRTY;
            }
        }
    } else {
        return uw_button(W, msg, param);
    }
    return UW_OK;
}
