
extern Bitmap *screen;

int ugisdl_process_event(SDL_Event *e);

/* Some widgets that are specific to ugiSDL:
(they use drawing routines that are not available in the base ugi)
*/
int usw_dial(uWidget *W, int msg, int param);
int usw_icon(uWidget *W, int msg, int param);
