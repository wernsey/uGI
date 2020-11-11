#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include "ugi.h"

unsigned int ugi_screen_width = 0;
unsigned int ugi_screen_height = 0;

/* Colors */
unsigned int ugc_background = 0x40318D;
unsigned int ugc_foreground = 0x7869C4;

void *ugi_font = NULL;

static void unclip() {
    ud_clip(0,0, ugi_screen_width, ugi_screen_height);
}

static void uprintf(int x, int y, const char *fmt, ...) {
    char buffer[256];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, arg);
    va_end(arg);
    ud_text(x, y, buffer);
}

void uu_highlight_widget(uWidget *W) {
    ud_highlight_box(W->x-1, W->y-1, W->x + W->w, W->y + W->h);
}

void uu_clear_widget(uWidget *W, unsigned int bg) {
    ud_set_color(bg);
    ud_fill_box(W->x-1, // TODO: Why was it like this in the original?
        W->y-1,
        W->x + W->w,
        W->y + W->h);
}

static int widget_msg(uWidget *W, int msg, int param);

const char *uu_get_attr(uWidget *W, const char *key) {
    int i;
    for(i = 0; i < W->n; i++) {
        if(!strcmp(W->attrs[i].key, key))
            return W->attrs[i].val;
    }
    return NULL;
}

// Be careful calling this in a MSG_CHANGE, because
// it will send a MSG_CHANGE again, leading to recursion.
void uu_set_attr(uWidget *W, const char *key, const char *val) {
    uAttr *A = NULL;
    int i;
    for(i = 0; i < W->n; i++) {
        if(!strcmp(W->attrs[i].key, key)) {
            A = &W->attrs[i];
            free(A->val);
            break;
        }
    }
    if(!A) {
        if(W->n == W->aa) {
            W->aa <<= 1;
            W->attrs = realloc(W->attrs, W->aa * sizeof *W->attrs);
        }
        i = W->n++;
        A = &W->attrs[i];
        A->key = strdup(key);
    }
    assert(A);
    A->val = strdup(val);
    widget_msg(W, MSG_CHANGE, i);
}

void uu_set_attr_i(uWidget *W, const char *key, int val) {
    char buf[20];
    snprintf(buf, sizeof buf - 1, "%d", val);
    uu_set_attr(W, key, buf);
}

int uu_get_attr_i(uWidget *W, const char *key) {
    const char *sval = uu_get_attr(W, key);
    if(sval) return atoi(sval);
    return 0;
}

void uu_set_data(uWidget *W, void *val) {
    W->data = val;
}

void *uu_get_data(uWidget *W) {
    return W->data;
}

void uu_set_action(uWidget *W, void *val) {
    W->action = val;
}

void *uu_get_action(uWidget *W) {
    return W->action;
}

void uu_set_font(uWidget *W, void *font) {
    W->font = font;
}

void *uu_get_font(uWidget *W) {
    return W->font;
}

static int widget_msg(uWidget *W, int msg, int param) {
    if(!W->fun) return W_OK;
    int r = (*W->fun)(W, msg, param);
    if(r == W_HANDLED || r == W_DIRTY)
        W->flags |= FLAG_DIRTY;
    return r;
}

static void w_focus(uDialog *D, uWidget *W) {
    int j;
    for(j = 0; j < D->n; j++) {
        if(D->widgets[j].flags & FLAG_FOCUS) {
            widget_msg(&D->widgets[j], MSG_LOSEFOCUS, 0);
        }
    }
    widget_msg(W, MSG_GETFOCUS, 0);
}

int uu_in_widget(uWidget *W, int x, int y) {
    assert(W);
    return x >= W->x && x < W->x + W->w && y >= W->y && y < W->y + W->h;
}

void uu_get_color_attrs(uWidget *W, unsigned int *bgp, unsigned int *fgp) {
    if(bgp) {
        const char *attr = uu_get_attr(W, "background");
        if(attr)
            *bgp = 0x000000; // FIXME: bm_atoi(attr);
        else
            *bgp = ugc_background;
    }
    if(fgp) {
        const char *attr = uu_get_attr(W, "foreground");
        if(attr)
            *fgp = 0xFFFFFF; // FIXME: bm_atoi(attr);
        else
            *fgp = ugc_foreground;
    }
}

int uw_clear(uWidget *W, int msg, int param) {
    if(msg == MSG_DRAW) {
        unsigned int bg;
        uu_get_color_attrs(W, &bg, NULL);

        ud_set_color(bg);
        ud_box(0, 0, ugi_screen_width, ugi_screen_height);
    }
    return W_OK;
}

int uw_clear_dith(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        if(!W->w || !W->h) {
            W->x = 0; W->y = 0;
            W->w = ugi_screen_width;
            W->h = ugi_screen_height;
        }
    } else if(msg == MSG_DRAW) {
        unsigned int bg;
        uu_get_color_attrs(W, &bg, NULL);
        ud_set_color(bg);
        ud_dither_box(W->x, W->y, W->x + W->w, W->y + W->h);
    }
    return W_OK;
}

int uw_frame(uWidget *W, int msg, int param) {
    int x; //, y;
    if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        ud_box(W->x, W->y, W->x + W->w, W->y + W->h);

        ud_line(W->x, W->y + 10, W->x + W->w - 2, W->y + 10);

        // Close button
        x = W->x + W->w - 10;
        ud_line(x + 3, W->y + 4, x + 10 - 4, W->y + 10 - 3);
        ud_line(x + 10 - 4, W->y + 4, x + 3, W->y + 10 - 3);

        const char *lbl = uu_get_attr(W, "label");
        if(lbl) {
            ud_set_color(fg);
            ud_set_font(W->font);
            int wid = ud_text_width(W->font, lbl);
            int tx = W->x + (W->w - wid) / 2;
            ud_text(tx, W->y + 2, lbl);

            ud_dither_box(W->x + 10, W->y + 1, tx - 2, W->y + 10);
            ud_dither_box(tx + wid + 2, W->y + 1, W->x + W->w - 10, W->y + 10);

            /*
            int y;
            for(y = W->y + 1; y < W->y + 10 - 2; y += 2) {
                for(x = W->x + 10; x < tx - 2; x+= 2)
                    bm_putpixel(ugi_screen, x, y);
                for(x = tx + wid + 2; x < W->x + W->w - 8; x+= 2)
                    bm_putpixel(ugi_screen, x, y);
            }
            */
        } else {
            /*
            for(y = W->y + 1; y < W->y + 10 - 2; y += 2) {
                for(x = W->x + 2 + 10; x < W->x + W->w - 10; x += 2)
                    bm_putpixel(ugi_screen, x, y);
            }*/
        }
    } else if(msg == MSG_CLICK) {
        x = W->x + W->w - 10;
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;
        if(mx > x && mx < W->x + W->w && my > W->y && my < W->y + 10)
            return W_CLOSE;
    }
    return W_OK;
}

int uw_box(uWidget *W, int msg, int param) {
    if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        ud_box(W->x, W->y, W->x + W->w, W->y + W->h);
    }
    return W_OK;
}

int uw_menubar(uWidget *W, int msg, int param) {

    if(msg == MSG_START) {
        W->w = ugi_screen_width;
        W->h = ugi_screen_height;
        uu_set_data(W, NULL);
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        int ch = ud_text_height(W->font, " ");
        ud_set_font(W->font);

        ud_set_color(fg);
        int x = W->x + 2;
        ud_line(W->x, W->y + ch + 2, W->x + W->w, W->y + ch + 2);
        const uMenu *menu = uu_get_data(W);
        while(menu && menu->title) {
            ud_text(x, W->y + 2, menu->title);
            x += ud_text_width(W->font, menu->title) + 4;
            menu++;
        }
    } else if(msg == MSG_CLICK) {
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;

        if(my >= W->y && my < W->y + 10) {
            uMenu *menu = uu_get_data(W);
            int x = W->x + 2;
            while(menu && menu->title) {
                int w = ud_text_width(W->font, menu->title) + 4;
                if(mx >= x && mx < x + w) {
                    if(menu->submenus) {
                        ugi_menu_popup(menu->submenus, x, W->y+10-1, W);
                    } else if(menu->action) {
                        menu->action(menu, W);
                    }
                    return W_HANDLED;
                }
                x += w;
                menu++;
            }

        }
    }
    return W_OK;
}

/* Left-aligned label */
int uw_label(uWidget *W, int msg, int param) {
    if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        const char *lbl = uu_get_attr(W, "label");
        if(lbl) {
            uu_get_color_attrs(W, &bg, &fg);
            ud_set_font(W->font);
            int y = W->y + (W->h - ud_text_height(W->font, lbl))/2;
            ud_set_color(fg);
            ud_text(W->x + 2, y, lbl);
        }
    }
    return W_OK;
}

/* Centered label */
int uw_clabel(uWidget *W, int msg, int param) {
    if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        const char *lbl = uu_get_attr(W, "label");
        if(lbl) {
            uu_get_color_attrs(W, &bg, &fg);
            ud_set_font(W->font);
            int x = W->x + (W->w - ud_text_width(W->font, lbl))/2;
            int y = W->y + (W->h - ud_text_height(W->font, lbl))/2;
            ud_set_color(fg);
            ud_text(x, y, lbl);
        }
    }
    return W_OK;
}

int uw_button(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        uu_set_attr_i(W, "border", 1);
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);

        if(uu_get_attr_i(W, "border"))
            ud_box(W->x, W->y, W->x + W->w, W->y + W->h);

        const char *lbl = uu_get_attr(W, "label");
        if(lbl) {
            ud_set_font(W->font);
            int x = W->x + (W->w - ud_text_width(W->font, lbl))/2;
            int y = W->y + (W->h - ud_text_height(W->font, lbl))/2;
            ud_text(x, y, lbl);
        }

    } else if(msg == MSG_CLICK) {
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else if(msg == MSG_KEY) {
        if(param == UK_UP) {
            return W_UNFOCUS_PREV;
        } else if(param == UK_DOWN) {
            return W_UNFOCUS;
        }
    } else if(msg == MSG_CHAR) {
        if(param == '\t') {
            if(ud_get_key_state(UK_SHIFT))
                return W_UNFOCUS_PREV;
            return W_UNFOCUS;
        } else if(param == '\n' || param == '\r' || param == ' ') {
            return widget_msg(W, MSG_CLICK, 0);
        }
    } else if(msg == MSG_WANTFOCUS) {
        return W_WANTFOCUS;
    } else if(msg == MSG_GETFOCUS) {
        W->flags = W->flags | FLAG_FOCUS;
        return W_HANDLED;
    } else if(msg == MSG_LOSEFOCUS) {
        W->flags = W->flags & ~FLAG_FOCUS;
        return W_HANDLED;
    } else if(msg == MSG_CHANGE) {
        return W_DIRTY;
    }
    return W_OK;
}

int uw_icon(uWidget *W, int msg, int param) {
    return 0;
    /*
    // FIXME...
    if(msg == MSG_DRAW) {
        Bitmap *b = W->data;
        if(!b) {
            unsigned int fg;
            uu_get_color_attrs(W, NULL, &fg);
            box(W->x, W->y, W->x + W->w, W->y + W->h, fg);
        }
        int mask = uu_get_attr_i(W, "mask");
        int src_x = uu_get_attr_i(W, "src_x");
        int src_y = uu_get_attr_i(W, "src_y");
        int src_w = uu_get_attr_i(W, "src_w");
        int src_h = uu_get_attr_i(W, "src_h");
        if(!src_w) src_w = b->w;
        if(!src_h) src_h = b->h;
        bm_blit_ex(ugi_screen,  W->x, W->y, W->w, W->h, b, src_x, src_y, src_w, src_h, mask);
        return W_OK;
    }
    return uw_button(W, msg, param);
    */
}

int uw_checkbox(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        uu_set_attr_i(W, "value", 0);
    } else if(msg == MSG_DRAW) {

        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        char buffer[50];
        char value = uu_get_attr_i(W, "value")?'x':' ';
        const char *lbl = uu_get_attr(W, "label");
        snprintf(buffer, sizeof buffer, "[%c] %s", value, lbl?lbl:"---");
        int y = W->y + (W->h - ud_text_height(W->font, lbl))/2;

        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);
        ud_text(W->x, y, buffer);
    } else if(msg == MSG_CLICK) {
        int v = uu_get_attr_i(W, "value");
        uu_set_attr_i(W, "value", !v);
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

int uw_radio(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        uu_set_attr_i(W, "value", 0);
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);

        if(W->flags & FLAG_FOCUS)
           uu_highlight_widget(W);

        char buffer[50];
        char value = uu_get_attr_i(W, "value")?'o':' ';
        const char *lbl = uu_get_attr(W, "label");
        snprintf(buffer, sizeof buffer, "(%c) %s", value, lbl?lbl:"---");
        int y = W->y + (W->h - ud_text_height(W->font, lbl))/2;
        ud_text(W->x, y, buffer);

    } else if(msg == MSG_RADIO) {
        int grp = uu_get_attr_i(W, "group");
        if(param == grp && uu_get_attr_i(W, "value")) {
            uu_set_attr_i(W, "value", 0);
            return W_HANDLED;
        }
    } else if(msg == MSG_CLICK) {
        if(!uu_get_attr_i(W, "value")) {
            ugi_message(MSG_RADIO, uu_get_attr_i(W, "group"));
            uu_set_attr_i(W, "value", 1);
        }
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

int uw_slider(uWidget *W, int msg, int param) {

    int min, max, val;
    ugi_widget_action cb = W->action;
    if(msg == MSG_START) {
        uu_set_attr_i(W, "minimum", 0);
        uu_set_attr_i(W, "maximum", 100);
        uu_set_attr_i(W, "value", 50);
        uu_set_attr_i(W, "step", 10);
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        int cw = ud_text_width(W->font, " ");
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        int x;
        min = uu_get_attr_i(W, "minimum");
        max = uu_get_attr_i(W, "maximum");
        val = uu_get_attr_i(W, "value");

        if(val < min) val = min;
        if(val > max) val = max;
        x = (W->x + (cw + 2)) + (W->w - 2*(cw + 2)) * (val - min) / (max - min);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);

        ud_box(W->x + (cw + 2), W->y + 3, W->x + W->w - 2*(cw + 2), W->y + 3);

        ud_fill_box(x-1, W->y + 1, x+1, W->y + 6);

        ud_box(x-1, W->y + 1, W->x + 3, W->y + 6);

        ud_text(W->x + 1, W->y + 1, "-");
        ud_text(W->x + W->w - (cw + 2), W->y + 1, "+");

        uprintf(W->x + (W->w - 3*cw)/2, W->y + 7, "%d", val);

    } else if(msg == MSG_CLICK) {
        int cw = ud_text_width(W->font, " ");
        int mx = (param >> 16) & 0xFFFF;

        min = uu_get_attr_i(W, "minimum");
        max = uu_get_attr_i(W, "maximum");
        val = uu_get_attr_i(W, "value");
        if(mx < (W->x + (cw + 2)))
            val -= uu_get_attr_i(W, "step");
        else if(mx >= (W->x + W->w - (cw + 2))) {
            val += uu_get_attr_i(W, "step");
        } else
            val = (max - min) * (mx - (W->x + (cw + 2))) / (W->w - 2*(cw + 2)) + min;
        if(val < min) val = min;
        if(val > max) val = max;

        uu_set_attr_i(W, "value", val);
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else if(msg == MSG_CHAR) {
        if(param == '\t') {
            if(ud_get_key_state(UK_SHIFT))
                return W_UNFOCUS_PREV;
            return W_UNFOCUS;
        }
        int value = uu_get_attr_i(W, "value");

        if(param == '-') {
            uu_set_attr_i(W, "value", value - 1);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == '+') {
            uu_set_attr_i(W, "value", value + 1);
            if(cb) cb(W);
            return W_HANDLED;
        }
    } else if(msg == MSG_KEY) {
        int value = uu_get_attr_i(W, "value");
        if(param == UK_LEFT) {
            uu_set_attr_i(W, "value", value - 1);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_RIGHT) {
            uu_set_attr_i(W, "value", value + 1);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_PAGEDOWN) {
            uu_set_attr_i(W, "value", value - 10);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_PAGEUP) {
            uu_set_attr_i(W, "value", value + 10);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_HOME) {
            min = uu_get_attr_i(W, "minimum");
            uu_set_attr_i(W, "value", min);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_END) {
            max = uu_get_attr_i(W, "maximum");
            uu_set_attr_i(W, "value", max);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_UP) {
            return W_UNFOCUS_PREV;
        } else if(param == UK_DOWN) {
            return W_UNFOCUS;
        }
        return W_HANDLED;
    } else if(msg == MSG_CHANGE) {

        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int val = uu_get_attr_i(W, "value");
        if(val < min) {
            val = min;
            uu_set_attr_i(W, "value", val);
        }
        if(val > max) {
            val = max;
            uu_set_attr_i(W, "value", val);
        }

    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}


int uw_dial(uWidget *W, int msg, int param) {

    ugi_widget_action cb = W->action;

    if(msg == MSG_START) {
        int r = (W->w < W->h ? W->w : W->h)/2 - 5;
        uu_set_attr_i(W, "radius", r);
        uu_set_attr_i(W, "value", 0);
        uu_set_attr_i(W, "minimum", 0);
        uu_set_attr_i(W, "maximum", 100);
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);
        //bm_rect(ugi_screen, W->x, W->y, W->x + W->w - 1, W->y + W->h - 1);

        int x0, y0, r = uu_get_attr_i(W, "radius");
        x0 = W->x + W->w / 2;
        y0 = W->y + W->h / 2;

        ud_circle(x0, y0, r);

        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int val = uu_get_attr_i(W, "value");

        int a = 300 * (val - min)/(max - min) + 30 + 90;

        int y1 = sin(a * M_PI/180) * r;
        int x1 = cos(a * M_PI/180) * r;
        ud_line(x0 + x1, y0 + y1, x0 + x1/2, y0 + y1/2);

        y1 = sin((30 + 90) * M_PI/180) * (r + 5);
        x1 = cos((30 + 90) * M_PI/180) * (r + 5);
        ud_box(x0 + x1, y0 + y1, x0 + x1 + 1, y0 + y1 + 1);
        y1 = sin((330 + 90) * M_PI/180) * (r + 5);
        x1 = cos((330 + 90) * M_PI/180) * (r + 5);
        ud_box(x0 + x1, y0 + y1, x0 + x1 + 1, y0 + y1 + 1);

     } else if(msg == MSG_CLICK) {
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;
        int x0, y0;
        x0 = W->x + W->w / 2;
        y0 = W->y + W->h / 2;
        int dx = mx - x0;
        int dy = my - y0;

        int a = atan2(dy, dx) * 180 / M_PI - 90;
        while(a < 0) a += 360;
        while(a >= 360) a -= 360;
        if(a < 30) a = 30;
        if(a > 330) a = 330;

        int val = 100 * (a - 30) / 300;

        uu_set_attr_i(W, "value", val);

        if(cb) cb(W);

        return W_HANDLED;

    } else if(msg == MSG_KEY) {
        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int value = uu_get_attr_i(W, "value");
        if(param == UK_LEFT) {
            uu_set_attr_i(W, "value", value - 1);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_RIGHT) {
            uu_set_attr_i(W, "value", value + 1);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_PAGEDOWN) {
            uu_set_attr_i(W, "value", value - 10);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_PAGEUP) {
            uu_set_attr_i(W, "value", value + 10);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_HOME) {
            uu_set_attr_i(W, "value", min);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_END) {
            uu_set_attr_i(W, "value", max);
            if(cb) cb(W);
            return W_HANDLED;
        } else if(param == UK_UP) {
            return W_UNFOCUS_PREV;
        } else if(param == UK_DOWN) {
            return W_UNFOCUS;
        }
        return W_HANDLED;
    } else if(msg == MSG_CHANGE) {

        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int val = uu_get_attr_i(W, "value");
        if(val < min) {
            val = min;
            uu_set_attr_i(W, "value", val);
        }
        if(val > max) {
            val = max;
            uu_set_attr_i(W, "value", val);
        }

    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

int uw_text_input(uWidget *W, int msg, int param) {

    int cw = ud_text_width(W->font, " ");
    int ch = ud_text_height(W->font, " ");

    if(msg == MSG_START) {
        uu_set_attr(W, "text", "");
        uu_set_attr_i(W, "cursor", 0);
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "blink", 0);
        return W_OK;
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);

        const char *text = uu_get_attr(W, "text");
        int cursor = uu_get_attr_i(W, "cursor");

        ud_box(W->x, W->y, W->x + W->w, W->y + W->h);

        int y = W->y + (W->h - ch) / 2;
        ud_text(W->x + 2, y, text);

        int blink = uu_get_attr_i(W, "blink");

        if(blink && W->flags & FLAG_FOCUS) {
            ud_line(W->x + cursor*cw + 1, W->y, W->x + cursor*cw + 1, W->y + W->h - 1);
        }
    } else if(msg == MSG_CLICK) {
        int mx = (param >> 16) & 0xFFFF;
        int cursor = (mx - W->x - 2)/cw;
        const char *text = uu_get_attr(W, "text");
        if(cursor > strlen(text))
            cursor = strlen(text);
        if(cursor < 0)
            cursor = 0;
        uu_set_attr_i(W, "cursor", cursor);
        return W_HANDLED;
    } else if(msg == MSG_KEY || msg ==  MSG_CHAR) {
        const char *text = uu_get_attr(W, "text");
        int cursor = uu_get_attr_i(W, "cursor");
        int len;

        assert(text);

        len = strlen(text);

        if(cursor < 0) cursor = 0;
        if(cursor > len) cursor = len;

        if(msg == MSG_KEY) {
            switch(param)  {
                case UK_UP: return W_UNFOCUS_PREV;
                case UK_DOWN: return W_UNFOCUS;
                case UK_LEFT: if(cursor > 0) uu_set_attr_i(W, "cursor", cursor - 1); break;
                case UK_RIGHT: if(cursor < strlen(text)) uu_set_attr_i(W, "cursor", cursor + 1); break;
                case UK_HOME: uu_set_attr_i(W, "cursor", 0); break;
                case UK_END: uu_set_attr_i(W, "cursor", strlen(text)); break;
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

            if(param == '\t' || param == '\n' || param == '\r') {
                if(ud_get_key_state(UK_SHIFT))
                    return W_UNFOCUS_PREV;
                return W_UNFOCUS;
            } else if(param == '\b') {
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
            } else if(isprint(param)) {
                int len = strlen(text), maxlen = (W->w - 4)/4;
                if(len < maxlen) {
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

        }
        return W_HANDLED;
    } else if(msg == MSG_TICK) {
        if(W->flags & FLAG_FOCUS) {
            int elapsed = uu_get_attr_i(W, "time") + param;
            uu_set_attr_i(W, "time", elapsed);
            int blink = uu_get_attr_i(W, "blink");
            int nblink = (elapsed >> 8) & 0x01;
            if(blink ^ nblink) {
                uu_set_attr_i(W, "blink", nblink);
                return W_OK;
            }
        }
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

/* Draws a scroll bar at position `bx`,`by` of dimensions `bw`x`bh`,
    where `num` is the number of lines in the scrollable widget, and
    `lh` is the height of each line in pixels, and `scroll` is the current
    scroll bar position (line number).
*/
static void draw_scrollbar(int bx, int by, int bw, int bh, int scroll, int num, int lh) {
    ud_dither_box(bx, by, bx + bw, by + bh);
    int nlf = bh/lh - 1; // visible lines
    if(nlf < num) {
        int nl = num - nlf;
        int ph = bh * bh / (lh * num); // pip height
        int pp = scroll * (bh - ph) / nl; // pip position
        if(ph > bh - 2) {
            ph = bh - 1;
            pp = 0;
        }
        ud_fill_box(bx, by + pp + 1, bx + bw - 1, by + pp + ph - 2);
    }
}

/* Computes a new value for a scrollbar drawn with `draw_scrollbar()`
based on a mouse click at `mx`,`my` */
static int click_scrollbar(int mx, int my, int bx, int by, int bw, int bh, int scroll, int num, int lh) {
    if(mx < bx || mx >= bx + bw) return scroll;
    if(my < by || my >= by + bh) return scroll;
    int nlf = bh/lh - 1;
    if(nlf < num) {
        int nl = num - nlf;
        int ph = bh * bh / (lh * num);
        int pp = scroll * (bh - ph) / nl;

        if(ph > bh - 2) {
            ph = bh - 1;
            pp = 0;
        }
        if(my - by < pp) {
            scroll -= nlf - 1;
        } else if(my - by >= pp + ph) {
            scroll += nlf - 1;
        }
        if(scroll < 0) scroll = 0;
        if(scroll > num - nlf) scroll = num - nlf;
    }
    return scroll;
}

int uw_listbox(uWidget *W, int msg, int param) {

    ugi_widget_list_fun list = (ugi_widget_list_fun)W->data;
    int ch = ud_text_height(W->font, " ");

    if(msg == MSG_START) {
        uu_set_attr_i(W, "index", 0);
        uu_set_attr_i(W, "scroll", 0);
        return W_OK;
    } else if(msg == MSG_DRAW) {

        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);

        ud_box(W->x, W->y, W->x + W->w, W->y + W->h);

        int i, n, y;

        int idx = uu_get_attr_i(W, "index");
        int scroll = uu_get_attr_i(W, "scroll");

        if(!list) return W_OK;
        list(W, -1, &n);
        ud_clip(W->x, W->y, W->x + W->w, W->y + W->h);
        for(i = scroll, y = 2; i < n && y < W->h; i++, y += 8) {
            if(i == idx) {
                ud_set_color(fg);
                ud_fill_box(W->x + 1, W->y + y - 1, W->x + W->w - 6 - 1, W->y + y + 8 - 2);
                ud_set_color(bg);
                ud_text(W->x + 2, W->y + y, list(W, i, NULL));
            } else {
                ud_set_color(fg);
                ud_text(W->x + 2, W->y + y, list(W, i, NULL));
            }
        }
        ud_set_color(fg);
        draw_scrollbar(W->x + W->w - 6, W->y + 1, 6, W->h, scroll, n, ch + 2);
        unclip();

    } else if(msg == MSG_CLICK) {
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;

        int n;
        if(!list) return W_OK;
        list(W, -1, &n);
        int scroll = uu_get_attr_i(W, "scroll");

        int x = W->x + W->w - 6 - 1;
        if(mx > x) {
            scroll = click_scrollbar(mx, my, W->x + W->w - 6, W->y, 6, W->h, scroll, n, ch + 2);
            uu_set_attr_i(W, "scroll", scroll);
        } else {
            int y = (my - W->y)/8 + scroll;
            if(y > n - 1) y = n - 1;
            uu_set_attr_i(W, "index", y);
        }
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else if(msg == MSG_CHAR) {
        if(param == '\t') {
            if(ud_get_key_state(UK_SHIFT))
                return W_UNFOCUS_PREV;
            return W_UNFOCUS;
        }
    } else if(msg == MSG_KEY) {
        int idx = uu_get_attr_i(W, "index"), n;

        list(W, -1, &n);
        assert(n > 0);
        if(param == UK_DOWN) {
            if(ud_get_key_state(UK_SHIFT))
                return W_UNFOCUS;
            idx = (idx + 1) % n;
            uu_set_attr_i(W, "index", idx);
        } else if(param == UK_UP) {
            if(ud_get_key_state(UK_SHIFT))
                return W_UNFOCUS_PREV;
            idx = (idx - 1);
            if(idx < 0) idx = n - 1;
            uu_set_attr_i(W, "index", idx);
        } else if(param == UK_HOME) {
            idx = 0;
            uu_set_attr_i(W, "index", idx);
        } else if(param == UK_END) {
            idx = n - 1;
            uu_set_attr_i(W, "index", idx);
        }

        int scroll = uu_get_attr_i(W, "scroll");
        int e = W->h / 8 - 2;
        if(idx < scroll) {
            uu_set_attr_i(W, "scroll", idx);
        } else if(idx > scroll + e) {
            scroll = idx - e;
            if(scroll < 0)
                scroll = 0;
            uu_set_attr_i(W, "scroll", scroll);
        }
        if(W->action) {
            ugi_widget_action cb = W->action;
            cb(W);
        }
        return W_HANDLED;
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

int uw_combo(uWidget *W, int msg, int param) {
    if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(W->font);

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);

        ud_box(W->x, W->y, W->w, W->h);

        const char *val = uu_get_attr(W, "value");
        if(val) {
            int y = W->y + (W->h - ud_text_height(W->font, val))/2;
            ud_text(W->x + 2, y, val);
        }
        int x = W->x + W->w - 10;
        ud_line(x, W->y, x, W->y + W->h - 1);

        ud_line(x + 2, W->y + 2, x + 4, W->y + W->h - 3);
        ud_line(x + 10 - 3, W->y + 2, x + 5, W->y + W->h - 3);
        ud_line(x + 2, W->y + 2, x + 10 - 3, W->y + 2);
    } else if(msg == MSG_CLICK) {
        uMenu *menu = uu_get_data(W);
        if(menu) {
            ugi_menu_popup_ex(menu, W->x, W->y + W->h - 1, W->w, 5, W);
            return W_OK;
        }
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

void uu_combo_menu_action(struct uMenu *menu, void *udata) {
    uWidget *W = udata;
    assert(W->fun == uw_combo);
    uu_set_attr(W, "value", menu->title);
    if(W->action) {
        ugi_widget_action cb = W->action;
        cb(W);
    }
}

static int count_lines(const char *text) {
    int line = 1;
    while(text && *text) {
        if(text[0] == '\n')
            line++;
        text++;
    }
    return line;
}
static int start_of_line(const char *text, int line) {
    int n = 0, i;
    if(line == 0) return 0;
    for(i = 0; text[i]; i++) {
        if(text[i] == '\n') {
            n++;
            if(n == line) return i+1;
        }
    }
    return 0;
}

int uw_textbox(uWidget *W, int msg, int param) {
    int cw = ud_text_width(W->font, " ");
    int ch = ud_text_height(W->font, " ");

    if(msg == MSG_START) {
        uu_set_attr_i(W, "cursor", 2);
        uu_set_attr_i(W, "scroll", 0);
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "blink", 0);
        uu_set_attr(W, "text", "");
        return W_OK;
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);

        ud_set_font(W->font);

        int i, n, x, y;

        int cursor = uu_get_attr_i(W, "cursor");
        int scroll = uu_get_attr_i(W, "scroll");
        const char *text = uu_get_attr(W, "text");
        int blink = uu_get_attr_i(W, "blink");
        if(!text) text = "";

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(W->flags & FLAG_FOCUS)
            uu_highlight_widget(W);
        ud_box(W->x, W->y, W->x + W->w, W->y + W->h);

        n = count_lines(text);

        int line = 0;
        for(i = 0; text[i] && line != scroll; i++) {
            if(text[i] == '\n')
                line++;
        }

        x = W->x + 2;
        y = W->y + 2;

        ud_clip(W->x, W->y, W->x + W->w, W->y + W->h);
        while(text[i]) {
            if((W->flags & FLAG_FOCUS) && i == cursor && blink) {
                ud_line(x, y - 2, x, y + 6);
            }
            if(text[i] == '\n') {
                x = W->x + 2;
                y += ch + 2;
                if(y > W->y + W->h) break;
            } else if(text[i] == '\t') {
                x += cw * 4;
                if(y > W->y + W->h) break;
            } else if(text[i] == '\r') {
            } else {
                uprintf(x, y, "%c", text[i]);
                x += cw;
            }
            i++;
        }
        if((W->flags & FLAG_FOCUS) && i == cursor && blink) {
            // Cursor at end of text
            ud_line(x, y - 2, x, y + 6);
        }
        draw_scrollbar(W->x + W->w - 6, W->y + 1, 6, W->h, scroll, n, ch + 2);
        unclip();

    } else if(msg == MSG_CLICK) {

        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;

        int scroll = uu_get_attr_i(W, "scroll");
        const char *text = uu_get_attr(W, "text");
        if(!text) text = "";
        int n;
        n = count_lines(text);

        int x = W->x + W->w - 6 - 1;
        if(mx > x) {

            scroll = click_scrollbar(mx, my, W->x + W->w - 6, W->y, 6, W->h, scroll, n, ch + 2);
            uu_set_attr_i(W, "scroll", scroll);

        } else {
            int y = (my - W->y)/(ch + 2) + scroll;
            if(y >= n) y = n - 1;
            int i = start_of_line(text, y);
            x = (mx - W->x - 2)/cw;
            while(x > 0 && text[i]) {
                char c = text[i];
                if(c == '\r' || c == '\n') break;
                if(c == '\t') x -= 4;
                else x--;
                i++;
            }
            uu_set_attr_i(W, "cursor", i);
        }
        uu_set_attr_i(W, "time", 1<<8);
        return W_HANDLED;
    } else if(msg == MSG_KEY || msg ==  MSG_CHAR) {
        const char *text = uu_get_attr(W, "text");
        if(!text) return W_HANDLED;
        int cursor, line, col;
        int scroll = uu_get_attr_i(W, "scroll");
        int len, i;

        assert(text);

        len = strlen(text);
        int n = count_lines(text);

        /* Where is the cursor? */
        cursor = uu_get_attr_i(W, "cursor");
        for(i = 0, line = 0, col = 0; text[i] && i < cursor; i++ ) {
            if(text[i] == '\n') {
                line++;
                col = 0;
            } else if(text[i] == '\r') {
            } else
                col++;
        }

        if(cursor < 0) cursor = 0;
        if(cursor > len) cursor = len;

        if(msg == MSG_KEY) {
            switch(param)  {
                /* TODO: Tabs really mess up the UP/DOWN key handling. */
                case UK_UP:
                    line--;
                    if(line < 0) line = 0;
                    cursor = start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_DOWN:
                    line++;
                    if(line > n - 1) line = n - 1;
                    cursor = start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_PAGEUP:
                    line -= W->h / (ch + 2) - 2;
                    if(line < 0) line = 0;
                    cursor = start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_PAGEDOWN:
                    line += W->h / (ch + 2) - 2;
                    if(line > n - 1) line = n - 1;
                    cursor = start_of_line(text, line);
                    for(i = 0; i < col && text[cursor] && text[cursor] != '\n'; i++, cursor++);
                    uu_set_attr_i(W, "cursor", cursor);
                break;
                case UK_LEFT: if(cursor > 0) uu_set_attr_i(W, "cursor", cursor - 1); break;
                case UK_RIGHT: if(cursor < len) uu_set_attr_i(W, "cursor", cursor + 1); break;
                case UK_HOME:
                    cursor = start_of_line(text, line);
                    uu_set_attr_i(W, "cursor", cursor); break;
                case UK_END:
                    cursor = start_of_line(text, line);
                    for(; text[cursor] && text[cursor] != '\n'; cursor++);
                    uu_set_attr_i(W, "cursor", cursor); break;
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
        int e = (W->h - 2) / (ch + 2) - 1;
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

        return W_HANDLED;
    } else if(msg == MSG_TICK) {
        if(W->flags & FLAG_FOCUS) {
            int elapsed = uu_get_attr_i(W, "time") + param;
            uu_set_attr_i(W, "time", elapsed);
            int blink = uu_get_attr_i(W, "blink");
            int nblink = (elapsed >> 8) & 0x01;
            if(blink ^ nblink) {
                uu_set_attr_i(W, "blink", nblink);
                return W_OK;
            }
        }
    } else {
        return uw_button(W, msg, param);
    }
    return W_OK;
}

/* Animated widget that just shows that the GUI is still
responding to events for development purposes */
int uw_ticker(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        uu_set_attr(W, "sequence", "/-\\|");
        uu_set_attr_i(W, "index", 0);
        uu_set_attr_i(W, "time", 0);
        if(!W->w) W->w = 8;
        if(!W->h) W->h = 8;
        return W_OK;
    } else if(msg == MSG_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        int index = uu_get_attr_i(W, "time") >> 8;
        const char *seq = uu_get_attr(W, "sequence");
        if(!seq || !seq[0]) return W_OK;

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        ud_set_font(W->font);
        uprintf(W->x, W->y, "%c", seq[index % strlen(seq)]);
    } else if(msg == MSG_TICK) {
        int elapsed = uu_get_attr_i(W, "time") + param;
        uu_set_attr_i(W, "time", elapsed);
        if((elapsed >> 7) & 0x01)
            W->flags |= FLAG_DIRTY;
    }
    return W_OK;
}

int uw_timer(uWidget *W, int msg, int param) {
    if(msg == MSG_START) {
        uu_set_attr_i(W, "time", 0);
        uu_set_attr_i(W, "after", 10000);
        return W_OK;
    } else if(msg == MSG_TICK) {
        int elapsed = uu_get_attr_i(W, "time") + param;
        int after = uu_get_attr_i(W, "after");
        if(elapsed > after) {
            int result = W_OK;
            if(W->action) {
                ugi_widget_action callback = W->action;
                result = callback(W);
            }
            uu_set_attr_i(W, "time", 0);
            return result;
        } else {
            uu_set_attr_i(W, "time", elapsed);
        }
    }
    return W_OK;
}

/* ---------------------------------------------------------------------------- */

#define MAX_DLG 10
#define MAX_MENU 10

static uDialog *dialog_stack[MAX_DLG];
static int d_top = 0;

/* If swapping out Dialogs by using a combination of
    ugi_end(old)-ugi_start(new) then we can't free() the dialog
    inside ugi_end() - If you call ugi_end() inside an event handler
    the rest of the event handler might still try to dereference the
    dialog or its widgets. So instead we add it to a list of
    dialogs that have to be disposed, and free() it later.
 */
static uDialog *dispose_list[MAX_DLG];
static int ndispose = 0;

struct menu_stack_item {
    uMenu *menu;
    int x, y;
    int w;
    int scroll;
    int lines;
    void *udata;
} menu_stack[MAX_MENU];
static int m_top = 0;

uDialog *ugi_start() {
    uDialog *D = malloc(sizeof *D);
    D->aa = 16;
    D->n = 0;
    D->widgets = calloc(D->aa, sizeof *D->widgets);
    assert(d_top < MAX_DLG);
    dialog_stack[d_top] = D;
    d_top++;
    return D;
}

uWidget *ugi_add(uDialog *D, ugi_widget_fun fun, int x, int y, int w, int h) {
    if(D->n == D->aa) {
        D->aa <<= 1;
        D->widgets = realloc(D->widgets, D->aa * sizeof *D->widgets);
    }
    assert(D->n < D->aa && D->n >= 0);
    int idx = D->n++;
    uWidget *W = &D->widgets[idx];
    memset(W, 0, sizeof *W);
    W->fun = fun;
    W->x = x;
    W->y = y;
    W->w = w;
    W->h = h;

    W->aa = 8;
    W->n = 0;
    W->attrs = calloc(W->aa, sizeof *W->attrs);

    W->flags |= FLAG_DIRTY;

    W->font = ugi_font;

    W->D = D;
    W->idx = idx;

    widget_msg(W, MSG_START, 0);
    return W;
}

uWidget *ugi_get_by_id(uDialog *D, const char *id) {
    int i;
    for(i = 0; i < D->n; i++) {
        uWidget *W = &D->widgets[i];
        const char *wid = uu_get_attr(W, "id");
        if(wid && !strcmp(wid, id))
            return W;
    }
    return NULL;
}

void ugi_end() {
    assert(d_top > 0);
    dispose_list[ndispose++] = dialog_stack[--d_top];
    ugi_repaint_all();
}

static void dialog_dispose_all() {
    while(ndispose > 0) {
        uDialog *D = dispose_list[--ndispose];
        int i, j;
        for(i = 0; i < D->n; i++) {
            uWidget *W = &D->widgets[i];
            widget_msg(W, MSG_END, 0);
            for(j = 0; j < W->n; j++) {
                free(W->attrs[j].key);
                free(W->attrs[j].val);
            }
            free(W->attrs);
        }
        free(D->widgets);
        free(D);
    }
}

int ugi_paint() {

    int ch = ud_text_height(ugi_font, " ");
    if(d_top > 0) {
        int i;
        uDialog *D = dialog_stack[d_top - 1];
        if(ugi_font)
            ud_set_font(ugi_font);
        for(i = 0; i < D->n; i++) {
            uWidget *W = &D->widgets[i];
            if(W->flags & FLAG_HIDDEN) continue;
            if(W->flags & FLAG_DIRTY) {
                widget_msg(W, MSG_DRAW, 0);
                W->flags &= ~FLAG_DIRTY;
            }
        }

        for(i = 0; i < m_top; i++) {
            assert(i < MAX_MENU);
            int mw = 0, mh = 0, my = 0;
            int menu_x = menu_stack[i].x;
            int menu_y = menu_stack[i].y;
            uMenu *menu = menu_stack[i].menu, *mi;

            int nitems = 0;
            for(mi = menu; mi->title; mi++) {
                int tw = ud_text_width(ugi_font, mi->title);
                if(tw > mw)
                    mw = tw;
                nitems++;
            }
            mw += 10;
            if(menu_stack[i].w > mw) {
                mw = menu_stack[i].w;
            }
            if(menu_stack[i].lines > 0 && menu_stack[i].lines < nitems) {
                mh = menu_stack[i].lines * (ch + 2) + 1;
            } else {
                mh = nitems * (ch + 2) + 1;
            }

            ud_set_color(ugc_background);
            ud_fill_box(menu_x - 1, menu_y - 1, menu_x + mw, menu_y + mh);
            ud_set_color(ugc_foreground);
            ud_box(menu_x, menu_y, menu_x + mw - 1, menu_y + mh - 1);

            int item = 0;
            for(mi = menu; mi->title; mi++, item++) {
                if(item < menu_stack[i].scroll)
                    continue;
                if(menu_stack[i].lines && item >= menu_stack[i].scroll + menu_stack[i].lines)
                    break;
                ud_text(menu_x + 2, menu_y + my + 2, mi->title);
                my += (ch+2);
            }
            if(menu_stack[i].lines && nitems > menu_stack[i].lines)
                draw_scrollbar(menu_x + mw - 6, menu_y, 6, mh, menu_stack[i].scroll, nitems, ch+2);
        }
        dialog_dispose_all();
        return 1;
    }
    return 0;
}

int ugi_message(int msg, int param) {
    if(d_top > 0) {

        if(m_top) return W_OK;

        uDialog *D = dialog_stack[d_top - 1];
        int r, i, j, u = -1, up = -1;
        for(i = 0; i < D->n; i++) {
            uWidget *W = &D->widgets[i];
            if(W->flags & FLAG_HIDDEN)
                continue;
            if((msg == MSG_CHAR || msg == MSG_KEY) && !(W->flags & FLAG_FOCUS)) {
                continue;
            }
            r = widget_msg(W, msg, param);
            if(r == W_CLOSE) {
                ugi_end();
            } else if(r == W_HANDLED) {
                break;
            } else if(r == W_UNFOCUS) {
                u = i;
            } else if(r == W_UNFOCUS_PREV) {
                up = i;
            } else if(r == W_WANTFOCUS) {
                w_focus(D, W);
                break;
            }
        }
        if(u >= 0) {
            i = u;
            widget_msg(&D->widgets[i], MSG_LOSEFOCUS, 0);
            for(j = (i + 1) % D->n; j != i; j = (j + 1) % D->n) {
                uWidget *W = &D->widgets[j];
                if(W->flags & FLAG_HIDDEN) continue;
                int k = widget_msg(W, MSG_WANTFOCUS, 0);
                if(k == W_WANTFOCUS) {
                    widget_msg(&D->widgets[j], MSG_GETFOCUS, 0);
                    break;
                }
            }
        } else if(up >= 0) {
            i = up;
            widget_msg(&D->widgets[i], MSG_LOSEFOCUS, 0);
            for(j = i - 1; j != i; j = j - 1) {
                if(j < 0) j = D->n - 1;
                uWidget *W = &D->widgets[j];
                if(W->flags & FLAG_HIDDEN) continue;
                int k = widget_msg(W, MSG_WANTFOCUS, 0);
                if(k == W_WANTFOCUS) {
                    widget_msg(&D->widgets[j], MSG_GETFOCUS, 0);
                    break;
                }
            }
        }
        dialog_dispose_all();
        return 1;
    }
    return 0;
}

int ugi_click(int mx, int my) {
    int ch = ud_text_height(ugi_font, " ");
    if(d_top > 0) {
        if(m_top) {
            assert(m_top < MAX_MENU);
            int i = m_top-1;
            uMenu *active_menu = menu_stack[i].menu, *mi;
            int menu_x = menu_stack[i].x;
            int menu_y = menu_stack[i].y;

            int mnw = 0, mnh = 0, mny = 0, nitems=0;
            for(mi = active_menu; mi->title; mi++) {
                int tw = ud_text_width(ugi_font, mi->title);
                if(tw > mnw)
                    mnw = tw;
                nitems++;
            }
            mnw += 10;
            if(menu_stack[i].w > mnw) {
                mnw = menu_stack[i].w;
            }
            if(menu_stack[i].lines > 0 && menu_stack[i].lines < nitems) {
                mnh = menu_stack[i].lines * (ch + 2);
            } else {
                mnh = nitems * (ch + 2);
            }

            if(nitems > menu_stack[i].lines && mx >= menu_x + mnw - 6 && mx < menu_x + mnw
                && my >= menu_y && my < menu_y + mnh) {
                menu_stack[i].scroll = click_scrollbar(mx, my, menu_x + mnw - 6, menu_y, 6, mnh, menu_stack[i].scroll, nitems, ch+2);
                return W_OK;
            }

            if(mx >= menu_x && mx < menu_x + mnw && my >= menu_y && my < menu_y + mnh) {
                mny = - menu_stack[i].scroll * (ch+2);
                for(mi = active_menu; mi->title; mi++, mny += (ch+2)) {
                    if(my >= menu_y + mny && my < menu_y + mny + (ch+2)) {
                        if(mi->submenus) {
                            ugi_menu_popup(mi->submenus, menu_x + mnw/2, menu_y + mny, NULL);
                            return W_OK;
                        } else if(mi->action) {
                            mi->action(mi, menu_stack[i].udata);
                            break;
                        }
                    }
                }
            }

            m_top = 0;
            ugi_repaint_all();
            return W_OK;
        }
        int r, i;
        uDialog *D = dialog_stack[d_top - 1];
        for(i = 0; i < D->n; i++) {
            uWidget *W = &D->widgets[i];
            if(W->flags & FLAG_HIDDEN) continue;
            if(uu_in_widget(W, mx, my)) {
                r = widget_msg(W, MSG_CLICK, (mx << 16) | my);
                if(r == W_HANDLED) {
                    if(!(W->flags & FLAG_FOCUS)) {
                        w_focus(D, W);
                    }
                    return W_OK;
                } else if(r == W_CLOSE) {
                    ugi_end();
                    return W_CLOSE;
                }
            }
        }
        dialog_dispose_all();
        return W_OK;
    }
    return 0;
}

void ugi_repaint_all() {
    int i;
    if(d_top > 0) {
        uDialog *D = dialog_stack[d_top - 1];
        for(i = 0; i < D->n; i++)
            D->widgets[i].flags |= FLAG_DIRTY;
    }
}

int ugi_menu_popup(uMenu *m, int x, int y, void *udata) {
    int i = m_top++;
    if(i == MAX_MENU) return 0;
    menu_stack[i].scroll = 0;
    menu_stack[i].lines = 0;
    menu_stack[i].x = x;
    menu_stack[i].y = y;
    menu_stack[i].w = 0;
    menu_stack[i].menu = m;
    menu_stack[i].udata = udata;
    return i;
}

int ugi_menu_popup_ex(uMenu *m, int x, int y, int width, int lines, void *udata) {
    int i = m_top++;
    if(i == MAX_MENU) return 0;
    menu_stack[i].scroll = 0;
    menu_stack[i].lines = lines;
    menu_stack[i].x = x;
    menu_stack[i].y = y;
    menu_stack[i].w = width;
    menu_stack[i].menu = m;
    menu_stack[i].udata = udata;
    return i;
}
