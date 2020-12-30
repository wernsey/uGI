
#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 400

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

extern int init_gui();

extern void deinit_gui();

/* Use with caution */
int DoEvents();

void dlg_message(const char *message);
void dlg_messagef(const char *fmt, ...);
int dlg_yes_no(const char *prompt);
const char *dlg_file_select(const char *message, const char *path, const char *pattern);
