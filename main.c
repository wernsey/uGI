#include <ctype.h>
#include <assert.h>

#include "SDL.h"

#include "ugi.h"

#include "bmp.h"

#include "ugiSDL.h"

#include "bold.xbm"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

Bitmap *icon;

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

    icon = bm_load("tile.gif");

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
            if(!ugisdl_process_event(&e)) {
                // ugisdl didn't handle the event, so we can check if we need to
                // do something with it
                switch(e.type) {
                    case SDL_QUIT : done = 1; break;
                }
            }
        }

        ugi_message(UM_TICK, elapsed);

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
    bm_free(icon);

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

    W = ugi_add(popup, uw_button, (SCREEN_WIDTH)/2, SCREEN_HEIGHT - 20 - 10 - 3, (SCREEN_WIDTH - 40)/2 - 3, 10);
    uu_set_attr(W, "label", "Click Me");
    W->flags |= UF_FOCUS;
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
    W = ugi_add(D, uw_button, SCREEN_WIDTH/2 + 4, 22, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Button B");
    uu_set_action(W, button_callback);

    W = ugi_add(D, uw_checkbox, 4, 34, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Checkbox 1");

    W = ugi_add(D, uw_checkbox, SCREEN_WIDTH/2 + 4, 34, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Checkbox 2");
    uu_set_attr_i(W, "value", 1);

    W = ugi_add(D, uw_radio,  4, 46, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 1A");
    uu_set_attr_i(W, "group", 1);
    W = ugi_add(D, uw_radio,  4, 58, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 1B");
    uu_set_attr_i(W, "value", 1);
    uu_set_attr_i(W, "group", 1);
    W = ugi_add(D, uw_radio,  4, 70, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 1C");
    uu_set_attr_i(W, "group", 1);
    W->flags |= UF_HIDDEN;

    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2 + 4, 46, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 2A");
    uu_set_attr_i(W, "value", 1);
    uu_set_attr_i(W, "group", 2);
    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2 + 4, 58, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 2B");
    uu_set_attr_i(W, "group", 2);
    W = ugi_add(D, uw_radio,  SCREEN_WIDTH/2 + 4, 70, SCREEN_WIDTH/2 - 8, 10);
    uu_set_attr(W, "label", "Radio 2C");
    uu_set_attr_i(W, "group", 2);

    W = ugi_add(D, uw_slider,  4, 82, SCREEN_WIDTH/2 - 4, 16);
    uu_set_attr(W, "label", "Slider");

    W = ugi_add(D, uw_combo, 4, 100, SCREEN_WIDTH/2 - 4, 10);
    uu_set_attr(W, "value", "Please Choose...");
    uu_set_data(W, combomenu);
    uu_set_action(W, combo_callback);

    W = ugi_add(D, uw_listbox, SCREEN_WIDTH/2 + 4, 100, SCREEN_WIDTH/2 - 8, 52);
    uu_set_data(W, test_list);

    W = ugi_add(D, uw_text_area, 4, 112, SCREEN_WIDTH/2 - 4, 40);
    uu_set_attr(W, "text", "This is a\ntext editing\nwidget");

    W = ugi_add(D, uw_ticker, SCREEN_WIDTH - 11, SCREEN_HEIGHT - 11, 8,8);

    W = ugi_add(D, uw_text_input, 4, 154, SCREEN_WIDTH/2 - 4, 10);

    W = ugi_add(D, usw_dial, SCREEN_WIDTH/2 + 4, 154, 32, 32);

    W = ugi_add(D, usw_icon, SCREEN_WIDTH/2 + 4 + 34, 154, 32, 32);
    uu_set_data(W, icon);
}

