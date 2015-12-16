
typedef void (*PeriodSetCallBack)(uint8_t minutes);

void show_periodset(char *title, uint8_t minutes, uint8_t min_minutes, uint8_t max_minutes, PeriodSetCallBack set_event);
void hide_periodset(void);
void unload_periodset(void);