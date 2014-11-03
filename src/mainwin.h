#include <pebble.h>

void show_mainwin(void);
void hide_mainwin(void);
void update_clock();
void init_click_events(ClickConfigProvider click_config_provider);
void update_onoff(bool on);
void update_info(char* text);