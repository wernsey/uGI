#include <ctype.h>
#include <assert.h>

#include "SDL.h"

#include "ugi.h"

#include "bmp.h"

#include "bold.xbm"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

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
    return bm_text_width(screen, text);
}

int ud_text_height(void *font, const char *text) {
    if(!text)
        return 0;
    int lines = 1;
    while(*text) {
        if(*text == '\n')
            lines++;
        text++;
    }
    return 8 * lines;
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

void ud_circle(int x0, int y0, int r) {
    bm_circle(screen, x0, y0, r);
}

void ud_text(int x, int y, const char *text) {
    bm_puts(screen, x, y, text);
}

void ud_clip(int x0, int y0, int x1, int y1) {
    bm_clip(screen, x0, y0, x1, y1);
}

void init_gui();

int main(int argc, char *argv[]) {
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to init: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("uGI Test Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if(!renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!texture) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return 1;
    }

    screen = bm_create(SCREEN_WIDTH, SCREEN_HEIGHT);

    ugi_screen_width = SCREEN_WIDTH;
    ugi_screen_height = SCREEN_HEIGHT;

    // ugi_font = bm_get_font(screen);
    ugi_font = bm_make_xbm_font(bold_bits, 7);

    init_gui();

    Uint32 start = SDL_GetTicks();

    int done = 0;
    while(!done) {
        Uint32 elapsed = SDL_GetTicks() - start;
        if(elapsed == 0) {
            SDL_Delay(10);
            continue;
        }
        SDL_Event e;
        if(SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_QUIT : done = 1; break;
                case SDL_MOUSEBUTTONDOWN: {
                    if(e.button.button != SDL_BUTTON_LEFT) break;
                    ugi_click(e.button.x, e.button.y);
                } break;
                case SDL_MOUSEBUTTONUP: {
                } break;
                case SDL_MOUSEMOTION: {
                } break;
                case SDL_KEYDOWN: {
                    int k = e.key.keysym.sym;
                    switch(k) {
                        case SDLK_UP:  ugi_message(MSG_KEY, UK_UP); break;
                        case SDLK_DOWN:  ugi_message(MSG_KEY, UK_DOWN); break;
                        case SDLK_LEFT:  ugi_message(MSG_KEY, UK_LEFT); break;
                        case SDLK_RIGHT:  ugi_message(MSG_KEY, UK_RIGHT); break;
                        case SDLK_LSHIFT:
                        case SDLK_RSHIFT:  ugi_message(MSG_KEY, UK_SHIFT); break;
                        case SDLK_PAGEDOWN:  ugi_message(MSG_KEY, UK_PAGEDOWN); break;
                        case SDLK_PAGEUP:  ugi_message(MSG_KEY, UK_PAGEUP); break;
                        case SDLK_HOME:  ugi_message(MSG_KEY, UK_HOME); break;
                        case SDLK_END:  ugi_message(MSG_KEY, UK_END); break;
                        case SDLK_DELETE:  ugi_message(MSG_KEY, UK_DELETE); break;
                        default: {
                            if(!(k & 0xFFFF0000)) {
                                if(e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
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
                                ugi_message(MSG_CHAR, k);
                            }
                        }
                    }
                } break;
            }
        }

        ugi_message(MSG_TICK, elapsed);

        if(!ugi_paint()) {
            done = 1;
        }


        SDL_UpdateTexture(texture, NULL, bm_raw_data(screen), bm_width(screen)*4);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        start = SDL_GetTicks();
    }

    bm_free(screen);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

int button_callback(uWidget *W) {
    SDL_Log("Widget [%s] Clicked...", uu_get_attr(W, "label"));
    return 1;
}

int combo_callback(uWidget *W) {
    SDL_Log("Combo item [%s] selected...", uu_get_attr(W, "value"));
    return 1;
}

void menu_item(uMenu *menu, void *data) {
    SDL_Log("Menu Item [%s] selected", menu->title);
}

void show_popup(uMenu *menu, void *data) {
    uDialog *popup = ugi_start();
    uWidget *W = ugi_add(popup, uw_clear_dith, 0,0, 0,0);
    //W = ugi_add(popup, uw_clear_dith, 20 + 5, 20 + 5, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40);
    W = ugi_add(popup, uw_frame, 20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40);
    uu_set_attr(W, "label", menu->title);

    W = ugi_add(popup, uw_button, (SCREEN_WIDTH)/2, SCREEN_HEIGHT - 20 - 10 - 2, (SCREEN_WIDTH - 40)/2 - 2, 10);
    uu_set_attr(W, "label", "Click Me");
    W->flags |= FLAG_FOCUS;
}

const char *test_list(uWidget *W, int i, int *size) {
    static const char *list[] = {"foo", "bar", "baz", "fred", "quux", "blah", "alice", "bob", "carol", "dave", "eve", "frank", "gary", "harry", "iggy"};
    //static const char *list[] = {"foo", "bar", "baz"};
    if(i < 0) {
        assert(size);
        *size = (sizeof list) / (sizeof list[0]);
    } else {
        assert(i < (sizeof list) / (sizeof list[0]));
        return list[i];
    }
    return NULL;
}

uMenu subsubmenu[] = {
    {"SubSub 1", menu_item, NULL, NULL},
    {"SubSub 2", menu_item, NULL, NULL},
    {"SubSub 3", menu_item, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu submenu[] = {
    {"Item 1", menu_item, NULL, NULL},
    {"Item 2", menu_item, NULL, NULL},
    {"Item 3", menu_item, NULL, NULL},
    {"SubSub", NULL, subsubmenu, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu editmenu[] = {
    {"Cut", menu_item,  NULL, NULL},
    {"Copy", menu_item, NULL, NULL},
    {"Paste", menu_item,NULL, NULL},
    {"Undo", menu_item, NULL, NULL},
    {"Redo", menu_item, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu filemenu[] = {
    {"New", menu_item,  NULL, NULL},
    {"Open", menu_item, NULL, NULL},
    {"Save", menu_item, NULL, NULL},
    {"SubMenu", NULL, submenu, NULL},
    {"Exit", menu_item, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu mainmenu[] = {
    {"File", NULL, filemenu, NULL },
    {"Edit", NULL, editmenu, NULL },
    {"View", show_popup, NULL, NULL},
    {"Help", show_popup, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu combomenu[] = {
    {"Option A", uu_combo_menu_action, NULL, NULL },
    {"Option B", uu_combo_menu_action, NULL, NULL },
    {"Option C", uu_combo_menu_action, NULL, NULL},
    {"Option D", uu_combo_menu_action, NULL, NULL},
    {"Option E", uu_combo_menu_action, NULL, NULL},
    {"Option F", uu_combo_menu_action, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

void init_gui() {
    uDialog *D = ugi_start();
    uWidget *W;

    W = ugi_add(D, uw_clear, 0, 0, 0, 0);
    W = ugi_add(D, uw_frame, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    uu_set_attr(W, "label", "Window Title");

    W = ugi_add(D, uw_menubar, 0, 10, SCREEN_WIDTH, 0);
    uu_set_data(W, mainmenu);

    W = ugi_add(D, uw_label, 4, SCREEN_HEIGHT - 6 - 1, SCREEN_WIDTH/2, 6);
    uu_set_attr(W, "label", "Text Label");

    W = ugi_add(D, uw_button, 4, 22, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Button A");
    uu_set_action(W, button_callback);
    W = ugi_add(D, uw_button, SCREEN_WIDTH/2+4, 22, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Button B");
    uu_set_action(W, button_callback);

    W = ugi_add(D, uw_checkbox, 4, 32, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Checkbox 1");

    W = ugi_add(D, uw_checkbox, SCREEN_WIDTH/2, 32, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Checkbox 2");
    uu_set_attr_i(W, "value", 1);

    W = ugi_add(D, uw_radio,  4, 42, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 1A");
    uu_set_attr_i(W, "group", 1);
    W = ugi_add(D, uw_radio,  4, 52, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 1B");
    uu_set_attr_i(W, "value", 1);
    uu_set_attr_i(W, "group", 1);
    W = ugi_add(D, uw_radio,  4, 62, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 1C");
    uu_set_attr_i(W, "group", 1);
    W->flags |= FLAG_HIDDEN;

    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2, 42, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 2A");
    uu_set_attr_i(W, "value", 1);
    uu_set_attr_i(W, "group", 2);
    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2, 52, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 2B");
    uu_set_attr_i(W, "group", 2);
    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2, 62, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "label", "Radio 2C");
    uu_set_attr_i(W, "group", 2);

    W = ugi_add(D, uw_slider,  4, 64, SCREEN_WIDTH/2 - 4, 16);
    uu_set_attr(W, "label", "Slider");

    W = ugi_add(D, uw_text_input, 4, 82, SCREEN_WIDTH/2 - 4, 10);

    W = ugi_add(D, uw_listbox, SCREEN_WIDTH/2 + 4, 82, SCREEN_WIDTH/2 - 8, 40);
    uu_set_data(W, test_list);

    W = ugi_add(D, uw_textbox, 4, 95, SCREEN_WIDTH/2 - 4, 30);
    uu_set_attr(W, "text", "This is a\ntext editing\nwidget");

    W = ugi_add(D, uw_ticker, SCREEN_WIDTH - 11, SCREEN_HEIGHT - 11, 8,8);

    W = ugi_add(D, uw_combo, 4, 130, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "value", "Please Choose...");
    uu_set_data(W, combomenu);
    uu_set_action(W, combo_callback);

    W = ugi_add(D, uw_dial, 4, 150, 30, 30);
}

