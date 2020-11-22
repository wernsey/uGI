/*
I didn't set out initially to create a text editor, so
please excuse me that the code in here is a bit rough.

There are a couple of features I'd like to see:
   - Using proper data structures for the buffer
      - see http://stackoverflow.com/a/4200549/115589
   - Auto-indent when pressing enter
   - The cursor should remember your column when you
        move up and down over empty lines
   - Also, tabs really mess up the UP/DOWN key handling.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../ugi.h"
#include "richedit.h"

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

static void dummy_lexer(const char *text, char *colors, int len) {
    int i, sol = 1;
    for(i = 0; i < len; i++) {
        if(sol) {
            while(i < len - 1 && isspace(text[i])) i++;
            if(i < len && text[i] == '#') {
                while(i < len && text[i] != '\n')
                    colors[i++] = GREEN;
            continue;
            }
        }

        if(strchr("\"'", text[i])) {
            char term = text[i];
            colors[i++] = RED;
            while(text[i] && text[i] != term && text[i] != '\n') {
                if(text[i] == '\\')
                    colors[i++] = RED;
                colors[i++] = RED;
            }
            colors[i] = RED;
        } else if(!memcmp(text + i, "//", 2)) {
            colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            while(text[i] && text[i] != '\n')
                colors[i++] = LIGHT_GREY;
        } else if(!memcmp(text + i, "/*", 2)) {
            colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            while(text[i] && memcmp(text + i, "*/", 2))
                colors[i++] = LIGHT_GREY;
            colors[i++] = LIGHT_GREY;
            colors[i] = LIGHT_GREY;
        } else if(isalpha(text[i]) || text[i] == '_') {
            int s = i, j, len, k = LIGHT_BLUE;
            while(text[i] && (isalnum(text[i]) || text[i] == '_'))
                i++;
            len = i - s;
            for(j = 0; keywords[j]; j++) {
                if(len == strlen(keywords[j]) && !memcmp(keywords[j], text+s, len)) {
                    k = CYAN;
                    break;
                }
            }
            while(s < i)
                colors[s++] = k;
            i--;
        } else if(isdigit(text[i])) {
            if(text[i] == '0' && tolower(text[i+1]) == 'x') {
                colors[i++] = ORANGE;
                colors[i++] = ORANGE;
                while(text[i] && strchr("1234567890abcdef", tolower(text[i])))
                    colors[i++] = ORANGE;
            } else {
                while(text[i] && strchr("1234567890.eE", text[i]))
                    colors[i++] = ORANGE;
            }
        } else
            colors[i] = LIGHT_GREY;
        sol = (text[i] == '\n');
    }
}


int uw_richedit(uWidget *W, int msg, int param) {

    int cw = ud_text_width(uu_get_font(W), " ");
    int ch = ud_text_height(uu_get_font(W), " ");

    uRect pos = uu_get_position(W);

    if(msg == UM_START) {
        uu_set_attr_i(W, "cursor", 0);
        uu_set_attr_i(W, "scroll", 0);
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "blink", 0);
        uu_set_attr(W, "text", "");

        uu_set_action(W, dummy_lexer);

        return UW_OK;
    } else if(msg == UM_DRAW) {

        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        int i, n, x, y;

        int cursor = uu_get_attr_i(W, "cursor");
        int scroll = uu_get_attr_i(W, "scroll");
        const char *text = uu_get_attr(W, "text");
        int blink = uu_get_attr_i(W, "blink");
        if(!text) text = "";

        uu_clear_widget(W, bg);

        n = uu_count_lines(text);

        int line = 0;
        for(i = 0; text[i] && line != scroll; i++) {
            if(text[i] == '\n')
                line++;
        }

        x = pos.x + 2;
        y = pos.y + 2;

        /* This is nasty */
        int len = strlen(text);
        char *colors = malloc(len + 1);
        memset(colors, 0, len + 1);

        lexer_fun lexer = uu_get_action(W);
        if(lexer)
            lexer(text, colors, len);

        x = pos.x + 2;
        y = pos.y + 2;

        ud_clip(pos.x, pos.y, pos.x + pos.w, pos.y + pos.h);
        while(i < len && text[i]) {
            if(uu_get_flag(W,UF_FOCUS) && i == cursor && blink) {
                ud_set_color(fg);
                ud_fill_box(x, y - 2, x+2, y + 8);
            }
            if(text[i] == '\n') {
                x = pos.x + 2;
                y += ch + 2;
                if(y > pos.y + pos.h) break;
            } else if(text[i] == '\t') {
                x += cw * 4;
                if(y > pos.y + pos.h) break;
            } else if(text[i] == '\r') {
            } else {
                if(colors[i]) {
                    unsigned int fg = palette[colors[i] & 0x0F];
                    unsigned int bg = palette[(colors[i] >> 8) & 0x0F];
                    (void)bg; // FIXME: background
                    ud_set_color(fg);
                } else {
                    ud_set_color(ugi_get_default_foreground());
                }
                uu_printf(x, y, "%c", text[i]);
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

        ud_set_color(fg);
        if(uu_get_flag(W, UF_FOCUS))
            uu_highlight_widget(W);

        ud_box(pos.x, pos.y, pos.x + pos.w - 1, pos.y + pos.h - 1);


    } else if(msg == UM_CLICK) {

        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;

        int scroll = uu_get_attr_i(W, "scroll");
        const char *text = uu_get_attr(W, "text");
        if(!text) text = "";
        int n;
        n = uu_count_lines(text);

        int x = pos.x + pos.w - 6 - 1;
        if(mx > x) {
            int nlf = pos.h/(ch + 2) - 1;
            if(nlf < n) {
                int nl = n - nlf;
                int ph = pos.h * pos.h / ((ch + 2) * n);
                int pp = scroll * (pos.h - ph) / nl;

                if(ph > pos.h - 2) {
                    ph = pos.h - 1;
                    pp = 0;
                }
                if(my - pos.y < pp) {
                    scroll -= nlf - 1;
                } else if(my - pos.y >= pp + ph) {
                    scroll += nlf - 1;
                }
                if(scroll < 0) scroll = 0;
                if(scroll > n - nlf) scroll = n - nlf;
                uu_set_attr_i(W, "scroll", scroll);
            }

        } else {
            int y = (my - pos.y)/(ch + 2) + scroll;
            if(y >= n) y = n - 1;
            int i = uu_start_of_line(text, y);
            x = (mx - pos.x - 2)/cw;
            while(x > 0 && text[i]) {
                char c = text[i];
                if(c == '\r' || c == '\n') break;
                if(c == '\t') x -= 4;
                else x--;
                i++;
            }
            uu_set_attr_i(W, "cursor", i);
        }
        uu_set_attr_i(W, "time", 1<<8); /* shows the cursor */
        return UW_HANDLED;
    } else if(msg == UM_KEY || msg ==  UM_CHAR) {
        const char *text = uu_get_attr(W, "text");
        if(!text) return UW_HANDLED;
        int cursor, line, col;
        int scroll = uu_get_attr_i(W, "scroll");
        int len, i;

        assert(text);

        len = strlen(text);
        int n = uu_count_lines(text);

        /* Where is the cursor? */
        cursor = uu_get_attr_i(W, "cursor");
        for(i = 0, line = 0, col = 0; text[i] && i < cursor; i++ ) {
            if(text[i] == '\n') {
                line++;
                col = 0;
            } else
                col++;
        }

        if(cursor < 0) cursor = 0;
        if(cursor > len) cursor = len;

        if(msg == UM_KEY) {
            switch(param)  {
                /* TODO: Tabs really mess up the UP/DOWN key handling. */
                case UK_UP:
                    line--;
                    if(line < 0) line = 0;
                    cursor = uu_start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_DOWN:
                    line++;
                    if(line > n - 1) line = n - 1;
                    cursor = uu_start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_PAGEUP:
                    line -= pos.h / (ch + 2) - 2;
                    if(line < 0) line = 0;
                    cursor = uu_start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_PAGEDOWN:
                    line += pos.h / (ch + 2) - 2;
                    if(line > n - 1) line = n - 1;
                    cursor = uu_start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_LEFT:
                    if(cursor > 0)
                        uu_set_attr_i(W, "cursor", cursor - 1);
                    break;
                case UK_RIGHT:
                    if(cursor < len)
                        uu_set_attr_i(W, "cursor", cursor + 1);
                break;
                case UK_HOME:
                    cursor = uu_start_of_line(text, line);
                    uu_set_attr_i(W, "cursor", cursor);
                    break;
                case UK_END:
                    cursor = uu_start_of_line(text, line);
                    for(; text[cursor] && text[cursor] != '\n'; cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                    break;
                case UK_DELETE: {
                    if(cursor < len) {
                        char *buffer = malloc(len);
                        assert(buffer);

                        strncpy(buffer, text, cursor);
                        strncpy(buffer + cursor, text + cursor + 1, len - cursor - 1);
                        buffer[len - 1] = '\0';
                        uu_set_attr(W, "text", buffer);
                        free(buffer);
                    }
                } break;
            }
        } else {

            if(param == '\r')  param = '\n';

            /*if(param == '\t') {
                if(keys[KCODE(LSHIFT)] || keys[KCODE(RSHIFT)])
                    return W_UNFOCUS_PREV;
                return W_UNFOCUS;
            } else*/ if(param == '\b') {
                if(cursor > 0) {
                    char *buffer = malloc(len);
                    assert(buffer);

                    strncpy(buffer, text, cursor - 1);
                    strncpy(buffer + cursor - 1, text + cursor, len - cursor);
                    buffer[len - 1] = '\0';
                    uu_set_attr(W, "text", buffer);
                    uu_set_attr_i(W, "cursor", cursor - 1);
                    free(buffer);
                }
            } else if(isprint(param) || isspace(param)) {

                char *buffer = malloc(len + 2);
                strncpy(buffer, text, cursor);
                buffer[cursor] = param;
                strncpy(buffer + cursor + 1, text + cursor, len - cursor);
                buffer[len + 1] = '\0';
                uu_set_attr(W, "text", buffer);
                uu_set_attr_i(W, "cursor", cursor + 1);
                free(buffer);
            }

        }

        /* Where is the cursor now? */
        text = uu_get_attr(W, "text");
        cursor = uu_get_attr_i(W, "cursor");

        for(i = 0, line = 0; text[i] && i < cursor; i++ ) {
            if(text[i] == '\n')
                line++;
        }

        /* Ensure the cursor is scrolled in view */
        int e = (pos.h - 2) / (ch + 2) - 1;
        if(e >= n -1) {
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
