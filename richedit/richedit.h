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

char rt_char(uWidget *W, unsigned int i);

typedef void (*lexer_fun)(uWidget *W, char *color, int len);

int uw_richedit(uWidget *W, int msg, int param);

void rt_set_text(uWidget *W, const char *txt);
