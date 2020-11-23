#include <SDL.h>

#include "../ugi.h"

#include "../other/bmp.h"

Bitmap *screen = NULL;

int ud_get_key_state(uKeyCode code) {
    // https://wiki.libsdl.org/SDL_Scancode

    // state is actually updated by SDL_PumpEvents(),
    // but I don't need to call it because it is
    // indirectly called by SDL_PollEvent() in the main loop
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    switch(code) {
        case UK_UP: return state[SDL_SCANCODE_UP];
        case UK_DOWN: return state[SDL_SCANCODE_DOWN];
        case UK_LEFT: return state[SDL_SCANCODE_LEFT];
        case UK_RIGHT: return state[SDL_SCANCODE_RIGHT];
        case UK_SHIFT: return state[SDL_SCANCODE_RSHIFT] || state[SDL_SCANCODE_LSHIFT];
        case UK_PAGEDOWN: return state[SDL_SCANCODE_PAGEDOWN];
        case UK_PAGEUP: return state[SDL_SCANCODE_PAGEUP];
        case UK_HOME: return state[SDL_SCANCODE_HOME];
        case UK_END: return state[SDL_SCANCODE_END];
        case UK_DELETE: return state[SDL_SCANCODE_DELETE];
    }
    return 0;
}

int ud_text_width(void *font, const char *text) {
    if(!text)
        return 0;
    return bm_font_text_width(font, text);
}

int ud_text_height(void *font, const char *text) {
    if(!text)
        return 0;
    return bm_font_text_height(font, text);
}

void ud_set_color(unsigned int color) {
    bm_set_color(screen, color);
}

void ud_set_font(void *font) {
    if(font)
        bm_set_font(screen, font);
    else
        bm_reset_font(screen);
}

void ud_box(int x0, int y0, int x1, int y1) {
    bm_rect(screen, x0, y0, x1, y1);
}

void ud_highlight_box(int x0, int y0, int x1, int y1) {
    int x, y;
    for(x = x0; x <= x1; x += 2) {
        bm_putpixel(screen, x, y0);
        bm_putpixel(screen, x, y1);
    }
    for(y = y0; y <= y1; y += 2) {
        bm_putpixel(screen, x0, y);
        bm_putpixel(screen, x1, y);
    }
}

void ud_fill_box(int x0, int y0, int x1, int y1) {
    bm_fillrect(screen, x0, y0, x1, y1);
}

void ud_dither_box(int x0, int y0, int x1, int y1) {
    bm_dithrect(screen, x0, y0, x1, y1);
}

void ud_line(int x0, int y0, int x1, int y1) {
    bm_line(screen, x0, y0, x1, y1);
}

void ud_text(int x, int y, const char *text) {
    bm_puts(screen, x, y, text);
}

void ud_clip(int x0, int y0, int x1, int y1) {
    bm_clip(screen, x0, y0, x1, y1);
}

int ud_display_width() {
    return bm_width(screen);
}

int ud_display_height() {
    return bm_height(screen);
}

/* Helper function to translate SDL events to uGI messages */

int ugisdl_process_event(SDL_Event *e) {
    switch(e->type) {
        case SDL_MOUSEBUTTONDOWN: {
            if(e->button.button != SDL_BUTTON_LEFT)
                return 0;
            ugi_click(e->button.x, e->button.y);
            return 1;
        }
        case SDL_KEYDOWN: {
            int k = e->key.keysym.sym;
            switch(k) {
                case SDLK_UP:  ugi_message(UM_KEY, UK_UP); break;
                case SDLK_DOWN:  ugi_message(UM_KEY, UK_DOWN); break;
                case SDLK_LEFT:  ugi_message(UM_KEY, UK_LEFT); break;
                case SDLK_RIGHT:  ugi_message(UM_KEY, UK_RIGHT); break;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:  ugi_message(UM_KEY, UK_SHIFT); break;
                case SDLK_PAGEDOWN:  ugi_message(UM_KEY, UK_PAGEDOWN); break;
                case SDLK_PAGEUP:  ugi_message(UM_KEY, UK_PAGEUP); break;
                case SDLK_HOME:  ugi_message(UM_KEY, UK_HOME); break;
                case SDLK_END:  ugi_message(UM_KEY, UK_END); break;
                case SDLK_DELETE:  ugi_message(UM_KEY, UK_DELETE); break;

                case SDLK_F12 : bm_save(screen, "screen.gif"); break;

                default: {
                    if(!(k & 0xFFFF0000)) {
                        if(!!(e->key.keysym.mod & KMOD_SHIFT) ^ !!(e->key.keysym.mod & KMOD_CAPS)) {

                            if(isalpha(k)) {
                                k = toupper(k);
                            } else {
                                /* Is this the way to do it?
                                I don't think it would work with different keyboard layouts */
                                static const char *in  = "`1234567890-=[],./;'\\";
                                static const char *out = "~!@#$%^&*()_+{}<>?:\"|";
                                char *p = strchr(in, k);
                                if(p) {
                                    k = out[p-in];
                                }
                            }
                        }
                        ugi_message(UM_CHAR, k);
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

/* Some custom widgets */

int usw_dial(uWidget *W, int msg, int param) {

    uRect pos = uu_get_position(W);
    ugi_widget_action cb = uu_get_action(W);

    if(msg == UM_START) {
        int r = (pos.w < pos.h ? pos.w : pos.h)/2 - 5;
        uu_set_attr_i(W, "radius", r);
        uu_set_attr_i(W, "value", 0);
        uu_set_attr_i(W, "minimum", 0);
        uu_set_attr_i(W, "maximum", 100);
    } else if(msg == UM_DRAW) {
        unsigned int bg, fg;
        uu_get_color_attrs(W, &bg, &fg);
        ud_set_font(uu_get_font(W));

        uu_clear_widget(W, bg);

        ud_set_color(fg);
        if(uu_get_flag(W, UF_FOCUS))
            uu_highlight_widget(W);

        int x0, y0, r = uu_get_attr_i(W, "radius");
        x0 = pos.x + pos.w / 2;
        y0 = pos.y + pos.h / 2;

        bm_circle(screen, x0, y0, r);

        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int val = uu_get_attr_i(W, "value");

        int a = 300 * (val - min)/(max - min) + 30 + 90;

        int y1 = sin(a * M_PI/180) * r;
        int x1 = cos(a * M_PI/180) * r;
        ud_line(x0 + x1, y0 + y1, x0 + x1/2, y0 + y1/2);

        y1 = sin((30 + 90) * M_PI/180) * (r + 5);
        x1 = cos((30 + 90) * M_PI/180) * (r + 5);
        ud_box(x0 + x1, y0 + y1, x0 + x1, y0 + y1);
        y1 = sin((330 + 90) * M_PI/180) * (r + 5);
        x1 = cos((330 + 90) * M_PI/180) * (r + 5);
        ud_box(x0 + x1, y0 + y1, x0 + x1, y0 + y1);

     } else if(msg == UM_CLICK) {
        int mx = (param >> 16) & 0xFFFF, my = param & 0xFFFF;
        int x0, y0;
        x0 = pos.x + pos.w / 2;
        y0 = pos.y + pos.h / 2;
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

        return UW_HANDLED;

    } else if(msg == UM_KEY) {
        int min = uu_get_attr_i(W, "minimum");
        int max = uu_get_attr_i(W, "maximum");
        int value = uu_get_attr_i(W, "value");
        if(param == UK_LEFT) {
            uu_set_attr_i(W, "value", value - 1);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_RIGHT) {
            uu_set_attr_i(W, "value", value + 1);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_PAGEDOWN) {
            uu_set_attr_i(W, "value", value - 10);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_PAGEUP) {
            uu_set_attr_i(W, "value", value + 10);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_HOME) {
            uu_set_attr_i(W, "value", min);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_END) {
            uu_set_attr_i(W, "value", max);
            if(cb) cb(W);
            return UW_HANDLED;
        } else if(param == UK_UP) {
            return UW_UNFOCUS_PREV;
        } else if(param == UK_DOWN) {
            return UW_UNFOCUS;
        }
        return UW_HANDLED;
    } else if(msg == UM_CHANGE) {

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
    return UW_OK;
}

int usw_icon(uWidget *W, int msg, int param) {
    uRect pos = uu_get_position(W);

    if(msg == UM_DRAW) {
        Bitmap *b = uu_get_data(W);
        if(!b) {
            unsigned int fg;
            uu_get_color_attrs(W, NULL, &fg);
            ud_set_color(fg);
            ud_box(pos.x, pos.y, pos.x + pos.w - 1, pos.y + pos.h - 1);
        }
        int mask = uu_get_attr_i(W, "mask");
        int src_x = uu_get_attr_i(W, "src_x");
        int src_y = uu_get_attr_i(W, "src_y");
        int src_w = uu_get_attr_i(W, "src_w");
        int src_h = uu_get_attr_i(W, "src_h");
        if(!src_w) src_w = bm_width(b);
        if(!src_h) src_h = bm_height(b);
        bm_blit_ex(screen, pos.x, pos.y, pos.w, pos.h, b, src_x, src_y, src_w, src_h, mask);
        return UW_OK;
    }
    return uw_button(W, msg, param);
}