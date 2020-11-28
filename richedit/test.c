#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL.h>

#include "../ugi.h"

#include "../other/bmp.h"
#include "../other/bold.xbm"

#include "../SDL/ugiSDL.h"
#include "../SDL/sdlmain.h"

#include "richedit.h"

char *readfile(const char *fname) {
	FILE *f;
	long len,r;
	char *str;

	if(!(f = fopen(fname, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	rewind(f);

	if(!(str = malloc(len+2)))
		return NULL;
	r = fread(str, 1, len, f);

	if(r != len) {
		free(str);
		return NULL;
	}

	fclose(f);
	str[len] = '\0';
	return str;
}

void menu_item(uMenu *menu, void *data) {
    SDL_Log("Menu Item [%s] selected", menu->title);
}

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
    {"Exit", menu_item, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

uMenu mainmenu[] = {
    {"File", NULL, filemenu, NULL },
    {"Edit", NULL, editmenu, NULL },
    {NULL, NULL, NULL, NULL},
};

int init_gui() {
    ugi_set_default_font(bm_make_xbm_font(bold_bits, 7));

    uDialog *D = ugi_start();
    uWidget *W;

    W = ugi_add(D, uw_clear, 0, 0, 0, 0);
    W = ugi_add(D, uw_frame, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    uu_set_attr_s(W, "label", "Rich Edit Demo");


    W = ugi_add(D, uw_menubar, 0, 10, SCREEN_WIDTH, 0);
    uu_set_data(W, mainmenu);

    char *txt = readfile("test.c");
    if(!txt)
        return 0;

    W = ugi_add(D, uw_richedit, 2, 12 + 10, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 12 - 2 - 10);
    uu_set_flag(W, UA_FOCUS);
    rt_set_text(W, txt);

    free(txt);

    return 1;
}

void deinit_gui() {
}

