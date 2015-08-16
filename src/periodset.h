
typedef void (*PeriodSetCallBack)(int minutes);

void show_periodset(char *title, int minutes, int min_minutes, int max_minutes, PeriodSetCallBack set_event);
void hide_periodset(void);
void unload_periodset(void);