#include <ctype.h>
#include <assert.h>

#include <SDL.h>

#include "../ugi.h"

#include "../other/bmp.h"

#include "ugiSDL.h"
#include "sdlmain.h"

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

    int done = 0;

    if(!init_gui()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "GUI init failed");
        done = 1;
    }

    Uint32 start = SDL_GetTicks();
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

    deinit_gui();

    bm_free(screen);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
