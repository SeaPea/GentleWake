#include <pebble.h>

void show_mainwin(uint8_t autoclose_timeout);
void hide_mainwin(void);
void update_clock();
void init_click_events(ClickConfigProvider click_config_provider);
void update_onoff(bool on);
void update_info(char* text);
void update_autoclose_timeout(uint8_t timeout);
void show_alarm_ui(bool on);
void show_snooze(time_t snooze_time);
void show_monitoring(time_t alarm_time);