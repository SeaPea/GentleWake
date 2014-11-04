
typedef void (*AlarmTimeCallBack)(int day, int hour, int minute);

void show_alarmtime(int day, int hour, int minute, AlarmTimeCallBack set_event);
void hide_alarmtime(void);