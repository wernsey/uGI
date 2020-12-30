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

/* Some pop-up dialogs that may be useful */

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

void dlg_messagef(const char *fmt, ...) {
    char buffer[256];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, arg);
    va_end(arg);
    dlg_message(buffer);
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

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#ifdef WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

/* It would be difficult to do this without a global variable,
but I doubt the file dialog would be used in a way where it would matter */
static char filename_buffer[512];

static int match(const char *str, const char *pattern) {
	/* Simplified from Rich Salz' matcher. See:
	https://en.wikipedia.org/wiki/Wildmat
	*/
	for(;*pattern; str++, pattern++) {
		if(*pattern == '*') {
			while(*++pattern == '*');
			if(!*pattern)
				return 1;
			while(*str)
				if(match(str++, pattern))
					return 1;
			return 0;
		} else if(*pattern != '?' && *str != *pattern)
			return 0;
	}
	return !*str;
}

#define MAX_PATTERNS 16

static char **dir_list(const char *path, const char *pattern) {
    char **list = NULL;
    int n = 0, a = 4, i;


    list = calloc(a, sizeof *list);
    struct dirent *d;

    char buffer[256];
    snprintf(buffer, sizeof buffer, "%s", pattern);
    int np = 0;
    char *patterns[MAX_PATTERNS], *saveptr, *token;
    token = strtok_r(buffer, ";", &saveptr);
    while(token != NULL && np < MAX_PATTERNS) {
        patterns[np++] = token;
        token = strtok_r(NULL, ";", &saveptr);
    }

    DIR *dir = opendir(path);
    while((d = readdir(dir))) {
        if(d->d_type != DT_DIR)
            continue;
        if(n == a - 1) {
            a <<= 1;
            list = realloc(list, a * sizeof *list);
        }
        size_t len = strlen(d->d_name) + 2;
        list[n] = malloc(len);
        snprintf(list[n++], len, "%s%c", d->d_name, DIR_SEP);
    }
    closedir(dir);

    dir = opendir(path);
    while((d = readdir(dir))) {
        if(d->d_type == DT_DIR)
            continue;
        if(n == a - 1) {
            a <<= 1;
            list = realloc(list, a * sizeof *list);
        }
        for(i = 0; i < np; i++) {
            if(patterns[i][0] && match(d->d_name, patterns[i])) {
                list[n++] = strdup(d->d_name);
            }
        }
    }
    closedir(dir);

    list[n] = NULL;

    return list;
}

static void free_dir_list(char **list) {
    int i;
    if(!list)
        return;
    for(i = 0; list[i]; i++)
        free(list[i]);
    free(list);
}

static void set_directory(uWidget *W) {
    uDialog *D = uu_get_dialog(W);

    char path[256];
    getcwd(path, sizeof path);

    W = ugi_get_by_id(D, "path");
    assert(W);
    uu_set_attr_s(W, "text", path);

    W = ugi_get_by_id(D, "pattern");
    assert(W);
    const char *pattern = uu_get_attr_s(W, "text");

    W = ugi_get_by_id(D, "file-list");
    assert(W);

    char **list = uu_get_attr_p(W, "list");
    free_dir_list(list);

    list = dir_list(path, pattern);
    uu_set_attr_p(W, "list", list);
    uu_set_attr_i(W, "index", 0);
    uu_set_attr_i(W, "scroll", 0);

    W = ugi_get_by_id(D, "filename");
    uu_set_attr_s(W, "text", "");
    filename_buffer[0] = '\0';
}

static int go_button_callback(uWidget *W) {
    uDialog *D = uu_get_dialog(W);
    W = ugi_get_by_id(D, "path");
    assert(W);
    const char *path = uu_get_attr_s(W, "text");
    chdir(path);
    set_directory(W);

    return 1;
}

static int pattern_change_callback(uWidget *W) {
    uDialog *D = uu_get_dialog(W);
    W = ugi_get_by_id(D, "path");
    char cwd[256];
    getcwd(cwd, sizeof cwd);
    uu_set_attr_s(W, "text", cwd);

    return go_button_callback(W);
}

static int up_button_callback(uWidget *W) {
    chdir("..");
    set_directory(W);
    return 1;
}

static int filename_change(uWidget *W) {
    char cwd[256];
    getcwd(cwd, sizeof cwd);
    const char *name = uu_get_attr_s(W, "text");
    snprintf(filename_buffer, sizeof filename_buffer, "%s%c%s", cwd, DIR_SEP, name);
    return 1;
}

static int file_select_change_callback(uWidget *W) {
    uDialog *D = uu_get_dialog(W);
    int idx = uu_get_attr_i(W, "index");
    char **list = uu_get_attr_p(W, "list");
    int n;
    for(n = 0; list[n]; n++);
    if(idx < 0 || idx >= n)
        return 1;
    char *item = list[idx];

    char cwd[256], newpath[256];
    getcwd(cwd, sizeof cwd);

    size_t len = strlen(item);
    assert(len > 0);
    if(item[len-1] == DIR_SEP) {
        W = ugi_get_by_id(D, "path");

        snprintf(newpath, sizeof newpath, "%s%c%s", cwd, DIR_SEP, item);

        uu_set_attr_s(W, "text", newpath);

        W = ugi_get_by_id(D, "filename");
        uu_set_attr_s(W, "text", "");
        filename_buffer[0] = '\0';

    } else {
        W = ugi_get_by_id(D, "filename");
        uu_set_attr_s(W, "text", item);

        W = ugi_get_by_id(D, "path");
        uu_set_attr_s(W, "text", cwd);

        snprintf(filename_buffer, sizeof filename_buffer, "%s%c%s", cwd, DIR_SEP, item);
    }

    return 1;
}

const char *dlg_file_select(const char *message, const char *path, const char *pattern) {

    /*
    Inspirations
    http://www.steinke.net/allegro/chooser_screen.gif
    https://ilyabirman.net/meanwhile/pictures/tp-15.png
    */

    filename_buffer[0] = '\0';
    char savedir[256];
    int ok = 0;
    int y;

    getcwd(savedir, sizeof savedir);
    if(path)
        chdir(path);
    else
        path = savedir;

    if(!pattern)
        pattern = "*";

    uDialog *D = ugi_start();
    int top = ugi_get_top();

    uWidget *W = ugi_add(D, uw_clear_dith, 0, 0, 0, 0);

    void *font = ugi_get_default_font();
    int tw = ud_text_width(font, message);

    int ww = 280, wh = 180;
    int wx = (SCREEN_WIDTH - ww) / 2, wy = (SCREEN_HEIGHT - wh)/2;
    W = ugi_add(D, uw_frame, wx, wy, ww, wh);
    uu_set_attr_i(W, "close-button", 0);
    uu_set_attr_s(W, "label", message);

    y = wy + 12;

    tw = ud_text_width(font, "Path");

    W = ugi_add(D, uw_label, wx + 4, y, tw, 14);
    uu_set_attr_s(W, "label", "Path");

    int tx = wx + 4 + tw + 8;
    W = ugi_add(D, uw_text_input, tx, y, 140, 14);
    uu_set_attr_s(W, "id", "path");
    uu_set_attr_p(W, UA_CHANGE, go_button_callback);

    tx += 140 + 4;
    W = ugi_add(D, uw_button, tx, y, 16, 14);
    uu_set_attr_s(W, "label", ">");
    uu_set_attr_p(W, UA_CLICK, go_button_callback);

    tx += 16 + 4;
    W = ugi_add(D, uw_button, tx, y, 16, 14);
    uu_set_attr_s(W, "label", "^");
    uu_set_attr_p(W, UA_CLICK, up_button_callback);
    uu_set_attr_s(W, "text", path);

    y += 14 + 2;
    int h = wh - y - 2 * (12 + 2);

    uWidget *fileList = ugi_add(D, uw_listbox, wx+4, y, ww - 8, h);
    uu_set_attr_s(fileList, "id", "file-list");
    uu_set_attr_p(fileList, UA_CHANGE, file_select_change_callback);

    tx = wx + 4 + tw + 8;
    y += h + 2;

    int sy = y;

    W = ugi_add(D, uw_label, wx + 4, y, tw, 14);
    uu_set_attr_s(W, "label", "File");

    W = ugi_add(D, uw_text_input, tx, y, 140, 14);
    uu_set_attr_s(W, "id", "filename");
    uu_set_attr_s(W, "text", "");
    uu_set_attr_p(W, UA_CHANGE, filename_change);

    y += 14 + 2;

    W = ugi_add(D, uw_label, wx + 4, y, tw, 14);
    uu_set_attr_s(W, "label", "Type");

    W = ugi_add(D, uw_text_input, tx, y, 140, 14);
    uu_set_attr_s(W, "id", "pattern");
    uu_set_attr_s(W, "text", pattern);
    uu_set_attr_p(W, UA_CHANGE, pattern_change_callback);

    y = sy;
    tx = wx + ww - 60 - 4;

    W = ugi_add(D, uw_button, tx, y, 60, 14);
    uu_set_attr_s(W, "label", "OK");
    uu_set_attr_p(W, UA_CLICK, dialog_btn_callback);
    uu_set_attr_p(W, "variable", &ok);
    uu_set_attr_i(W, "value", 1);
    uu_set_flag(W, UA_FOCUS);

    y += 14 + 2;

    W = ugi_add(D, uw_button, tx, y, 60, 14);
    uu_set_attr_s(W, "label", "Cancel");
    uu_set_attr_p(W, UA_CLICK, dialog_btn_callback);

    set_directory(W);

    int d;
    do {
        d = DoEvents();
    } while(ugi_get_top() == top && d);

    chdir(savedir);

    char **list = uu_get_attr_p(fileList, "list");
    free_dir_list(list);

    if(ok && filename_buffer[0])
        return filename_buffer;
    else
        return NULL;
}