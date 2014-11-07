
typedef void (*PeriodSetCallBack)(int minutes);

void show_periodset(char *title, int minutes, int max_minutes, PeriodSetCallBack set_event);
void hide_periodset(void);
