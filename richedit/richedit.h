enum {
    BLACK = 0,
    WHITE,
    RED,
    CYAN,
    PURPLE,
    GREEN,
    BLUE,
    YELLOW,
    ORANGE,
    BROWN,
    LIGHT_RED,
    DARK_GREY,
    GREY,
    LIGHT_GREEN,
    LIGHT_BLUE,
    LIGHT_GREY
};

typedef void (*lexer_fun)(const char *text, char *color, int len);

int uw_richedit(uWidget *W, int msg, int param);
