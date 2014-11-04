#include <pebble.h>
#include "mainwin.h"
#include "settings.h"

static bool alarms_on = true;
  
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Snooze
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  alarms_on = !alarms_on;
  update_onoff(alarms_on);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_settings();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init(void) {
  show_mainwin();
  init_click_events(click_config_provider);
}

static void deinit(void) {
  hide_mainwin();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}