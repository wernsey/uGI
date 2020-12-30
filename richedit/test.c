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

static void c_lexer(uWidget *W, unsigned char *colors, int len) {
    int i, sol = 1;
    for(i = 0; i < len; i++) {
        char c = rt_char(W, i);
        if(sol) {
            while(i < len - 1 && isspace(c)) c = rt_char(W, ++i);
            if(i < len && c == '#') {
                while(i < len && rt_char(W, i) != '\n')
                    colors[i++] = GREEN;
                    //colors[i++] = (RED << 4) | GREEN;
                continue;
            }
        }

        if(strchr("\"'", c)) {
            char term = c;
            colors[i] = RED;
            while((c = rt_char(W, ++i)) && c != term && c != '\n') {
                if(c == '\\') {
                    colors[i] = RED;
                    c = rt_char(W, i++);
                }
                colors[i] = RED;
            }
            colors[i] = RED;
        }
         else if(c == '/' && rt_char(W, i + 1) == '/') {
            colors[i++] = PURPLE;
            colors[i++] = PURPLE;
            while((c = rt_char(W, i)) && c != '\n') {
                colors[i++] = PURPLE;
            }
        }
        else if(c == '/' && rt_char(W, i + 1) == '*') {
            colors[i++] = PURPLE;
            colors[i++] = PURPLE;
            while((c=rt_char(W,i)) && !(c == '*' && rt_char(W,i+1) == '/'))
                colors[i++] = PURPLE;
            colors[i++] = PURPLE;
            if(i < len)
                colors[i] = PURPLE;
        }
        else if(isalpha(c) || c == '_') {
            int s = i, k = LIGHT_BLUE, j=0;

            char kbuf[12];
            while((c = rt_char(W, i)) && (isalnum(c) || c == '_')) {
                if(j < sizeof kbuf - 1)
                    kbuf[j++] = c;
                i++;
            }
            kbuf[j] = '\0';

            for(j = 0; keywords[j]; j++) {
                if(!strcmp(keywords[j], kbuf)) {
                    k = CYAN;
                    break;
                }
            }

            while(s < i)
                colors[s++] = k;
            i--;
        }
        else if(isdigit(c)) {
            if(c == '0' && tolower(rt_char(W, i+1) == 'x')) {
                colors[i++] = ORANGE;
                colors[i++] = ORANGE;
                while((c = rt_char(W, i)) && strchr("1234567890abcdef", tolower(c)))
                    colors[i++] = ORANGE;
            } else {
                while((c = rt_char(W, i)) && strchr("1234567890.eE", c))
                    colors[i++] = ORANGE;
            }
            i--;
        }
        else
            colors[i] = LIGHT_GREY;
        sol = (c == '\n');
    }
}

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
    uu_set_attr_p(W, UA_MENU, mainmenu);

    char *txt = readfile("test.c");
    if(!txt)
        return 0;

    W = ugi_add(D, uw_richedit, 2, 12 + 10, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 12 - 2 - 10);
    uu_set_flag(W, UA_FOCUS);
    uu_set_attr_p(W, "lexer", c_lexer);
    rt_set_text(W, txt);

    free(txt);

    return 1;
}

void deinit_gui() {
}

