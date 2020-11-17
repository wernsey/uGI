/**
 * uGI - micro Graphics Interface
 * ==============================
 *
 * A small graphical user interface module.
 *
 * You may notice similarities to Allegro 4's GUI API;
 *  _This is intentional._
 */

typedef enum {
    UM_START,
    UM_END,
    UM_TICK,
    UM_DRAW,
    UM_CLICK,
    UM_KEY,
    UM_CHAR,
    UM_RADIO,
    UM_WANTFOCUS,
    UM_GETFOCUS,
    UM_LOSEFOCUS,
    UM_CHANGE,
} uMessage;

typedef enum {
    UW_OK,
    UW_CLOSE,
    UW_HANDLED,
    UW_DIRTY,
    UW_WANTFOCUS,
    UW_UNFOCUS,
    UW_UNFOCUS_PREV,
} uResponse;

typedef enum {
    UK_UP,
    UK_DOWN,
    UK_LEFT,
    UK_RIGHT,
    UK_SHIFT,
    UK_PAGEDOWN,
    UK_PAGEUP,
    UK_HOME,
    UK_END,
    UK_DELETE,
} uKeyCode;

#define UF_DIRTY 1
#define UF_FOCUS 2
#define UF_HIDDEN 4

struct uWidget;
typedef int (*ugi_widget_fun)(struct uWidget *, int msg, int param);

/* widget actions */
typedef int (*ugi_widget_action)(struct uWidget *W);
typedef const char *(*ugi_widget_list_fun)(struct uWidget *W, int i, int *size);

struct uMenu;
typedef void (*ugi_menu_action)(struct uMenu *, void *udata);

typedef struct uWidget uWidget;

typedef struct uMenu {
    const char *title;
    ugi_menu_action action;
    struct uMenu *submenus;
    void *data;
} uMenu;

typedef struct uDialog uDialog;

typedef struct {
    int x, y, w, h;
} uRect;

extern unsigned int ugi_screen_width;
extern unsigned int ugi_screen_height;

extern void *ugi_font;

/* Some default colors */
extern unsigned int ugc_background;
extern unsigned int ugc_foreground;

int (*u_text_width)(void *font, const char *text);
int (*u_text_height)(void *font, const char *text);

int (*u_get_key_state)(uKeyCode code);

/* Utility functions, prefixed with uu_ */
uRect uu_get_position(uWidget *W);

const char *uu_get_attr(uWidget *W, const char *key);
void uu_set_attr(uWidget *W, const char *key, const char *val);

void uu_set_attr_i(uWidget *W, const char *key, int val);
int uu_get_attr_i(uWidget *W, const char *key);

void uu_set_flag(uWidget *W, unsigned int flag);
void uu_clear_flag(uWidget *W, unsigned int flag);
int uu_get_flag(uWidget *W, unsigned int flag);

void uu_set_data(uWidget *W, void *val);
void *uu_get_data(uWidget *W);

void uu_set_action(uWidget *W, void *val);
void *uu_get_action(uWidget *W);

void uu_set_font(uWidget *W, void *font);
void *uu_get_font(uWidget *W);

int uu_in_widget(uWidget *W, int x, int y);
void uu_get_color_attrs(uWidget *W, unsigned int *bgp, unsigned int *fgp);

void uu_combo_menu_action(struct uMenu *, void *udata);

void uu_clear_widget(uWidget *W, unsigned int bg);
void uu_highlight_box(int x0, int y0, int x1, int y1);
void uu_highlight_widget(uWidget *W);

int uu_count_lines(const char *text);

int uu_start_of_line(const char *text, int line);

void uu_unclip();

void uu_printf(int x, int y, const char *fmt, ...);

/* Widget functions, prefixed with uw */
int uw_clear(uWidget *W, int msg, int param);
int uw_clear_dith(uWidget *W, int msg, int param);
int uw_frame(uWidget *W, int msg, int param);
int uw_box(uWidget *W, int msg, int param);
int uw_menubar(uWidget *W, int msg, int param);
int uw_label(uWidget *W, int msg, int param);
int uw_clabel(uWidget *W, int msg, int param);
int uw_button(uWidget *W, int msg, int param);
int uw_checkbox(uWidget *W, int msg, int param);
int uw_radio(uWidget *W, int msg, int param);
int uw_slider(uWidget *W, int msg, int param);
int uw_text_input(uWidget *W, int msg, int param);
int uw_text_area(uWidget *W, int msg, int param);
int uw_listbox(uWidget *W, int msg, int param);
int uw_combo(uWidget *W, int msg, int param);
int uw_ticker(uWidget *W, int msg, int param);
int uw_timer(uWidget *W, int msg, int param);

/* uGI API itself */
uDialog *ugi_start();
uWidget *ugi_add(uDialog *D, ugi_widget_fun fun, int x, int y, int w, int h);
uWidget *ugi_get_by_id(uDialog *D, const char *id);
void ugi_end();
int ugi_paint();
int ugi_message(int msg, int param);
int ugi_click(int mx, int my);
void ugi_repaint_all();
int ugi_menu_popup(uMenu *m, int x, int y, void *udata);
int ugi_menu_popup_ex(uMenu *m, int x, int y, int width, int lines, void *udata);

/* Functions for which the user of the library needs to provide
platform specific drawing functions */
void ud_set_color(unsigned int color);

void ud_set_font(void *font);

void ud_box(int x0, int y0, int x1, int y1);

void ud_highlight_box(int x0, int y0, int x1, int y1);

void ud_fill_box(int x0, int y0, int x1, int y1);

void ud_dither_box(int x0, int y0, int x1, int y1);

void ud_line(int x0, int y0, int x1, int y1);

void ud_text(int x, int y, const char *text);

void ud_clip(int x0, int y0, int x1, int y1);

int ud_get_key_state(uKeyCode code);

int ud_text_width(void *font, const char *text);

int ud_text_height(void *font, const char *text);
