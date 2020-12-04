#include <ctype.h>
#include <assert.h>

#include <SDL.h>

#include "../ugi.h"

#include "../other/bmp.h"

#include "ugiSDL.h"
#include "sdlmain.h"

static SDL_Window *Window = NULL;
static SDL_Renderer *Renderer = NULL;
static SDL_Texture *Texture = NULL;

static int Done = 0;
static Uint32 Start;

/*
A throwback to my misguided youth when I dabbled in Visual Basic
*/
int DoEvents() {
    Uint32 elapsed = SDL_GetTicks() - Start;
    if(elapsed == 0) {
        SDL_Delay(10);
        return !Done;
    }
    SDL_Event e;
    if(SDL_PollEvent(&e)) {
        if(!ugisdl_process_event(&e)) {
            // ugisdl didn't handle the event, so we can check if we need to
            // do something with it
            switch(e.type) {
                case SDL_QUIT : Done = 1; break;
            }
        }
    }

    ugi_message(UM_TICK, elapsed);

    if(!ugi_paint()) {
        Done = 1;
    }

    SDL_UpdateTexture(Texture, NULL, bm_raw_data(screen), bm_width(screen)*4);

    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer, Texture, NULL, NULL);
    SDL_RenderPresent(Renderer);
    Start = SDL_GetTicks();

    return !Done;
}

static int dialog_btn_callback(uWidget *W) {
    int *var = uu_get_attr_p(W, "variable");
    if(var)
        *var = uu_get_attr_i(W, "value");
    ugi_end();
    return 1;
}

void dlg_message(const char *message) {

    uDialog *D = ugi_start();
    int top = ugi_get_top();

    uWidget *W = ugi_add(D, uw_clear_dith, 0, 0, 0, 0);

    void *font = ugi_get_default_font();
    int tw = ud_text_width(font, message);
    int th = ud_text_height(font, message);

    int ww = 120, wh = 60;
    if(ww < tw + 20)
        ww = tw + 20;
    if(wh < th + 20 + 24)
        wh = th + 20 + 24;

    int wx = (SCREEN_WIDTH - ww) / 2, wy = (SCREEN_HEIGHT - wh)/2;
    W = ugi_add(D, uw_frame, wx, wy, ww, wh);
    uu_set_attr_i(W, "close-button", 0);

    W = ugi_add(D, uw_label, wx + (ww - tw)/2, wy + 20, tw, th);
    uu_set_attr_s(W, "label", message);

    W = ugi_add(D, uw_button, wx + (ww - 40)/2, wy + wh - 16 - 4, 40, 16);
    uu_set_attr_s(W, "label", "OK");
    uu_set_attr_p(W, UA_CLICK, dialog_btn_callback);
    uu_set_attr_i(W, UA_FOCUS, 1);

    /* This won't work with Emscripten because browsers
        just don't work that way */
    int d;
    do {
        d = DoEvents();
    } while(ugi_get_top() == top && d);
}

int dlg_yes_no(const char *prompt) {

    int yes = 0;
    uDialog *D = ugi_start();
    int top = ugi_get_top();

    uWidget *W = ugi_add(D, uw_clear_dith, 0, 0, 0, 0);

    void *font = ugi_get_default_font();
    int tw = ud_text_width(font, prompt);
    int th = ud_text_height(font, prompt);

    int ww = 120, wh = 60;
    if(ww < tw + 20)
        ww = tw + 20;
    if(wh < th + 20 + 24)
        wh = th + 20 + 24;

    int wx = (SCREEN_WIDTH - ww) / 2, wy = (SCREEN_HEIGHT - wh)/2;
    W = ugi_add(D, uw_frame, wx, wy, ww, wh);
    uu_set_attr_i(W, "close-button", 0);

    W = ugi_add(D, uw_label, wx + (ww - tw)/2, wy + 20, tw, th);
    uu_set_attr_s(W, "label", prompt);

    W = ugi_add(D, uw_button, wx + ww - 40 - 4, wy + wh - 16 - 4, 40, 16);
    uu_set_attr_s(W, "label", "No");
    uu_set_attr_p(W, UA_CLICK, dialog_btn_callback);

    W = ugi_add(D, uw_button, wx + ww - 40 - 4 - 40 - 4, wy + wh - 16 - 4, 40, 16);
    uu_set_attr_s(W, "label", "Yes");
    uu_set_attr_p(W, UA_CLICK, dialog_btn_callback);
    uu_set_attr_p(W, "variable", &yes);
    uu_set_attr_i(W, "value", 1);
    uu_set_flag(W, UA_FOCUS);

    /* This won't work with Emscripten because browsers
        just don't work that way */
    int d;
    do {
        d = DoEvents();
    } while(ugi_get_top() == top && d);
    return yes;
}

int main(int argc, char *argv[]) {

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to init: %s", SDL_GetError());
        return 1;
    }

    Window = SDL_CreateWindow("uGI Test Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!Window) {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        return 1;
    }

    Renderer = SDL_CreateRenderer(Window, -1, 0);
    if(!Renderer) {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        return 1;
    }

    Texture = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!Texture) {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        return 1;
    }

    screen = bm_create(SCREEN_WIDTH, SCREEN_HEIGHT);


    if(!init_gui()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GUI init failed");
        Done = 1;
    }

    Start = SDL_GetTicks();
    while(!Done) {
        DoEvents();
    }

    deinit_gui();

    bm_free(screen);

    SDL_DestroyTexture(Texture);
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();

    return 0;
}
