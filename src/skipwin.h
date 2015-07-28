#include <pebble.h>

typedef void (*SkipSetCallBack)(time_t skip_until);

void show_skipwin(time_t skip_until, SkipSetCallBack set_event);
void hide_skipwin(void);
