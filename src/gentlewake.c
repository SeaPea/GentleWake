#include <pebble.h>
#include "mainwin.h"
#include "settings.h"
#include "common.h"

static bool s_alarms_on = true;
static alarm s_alarms[7];
static char s_info[25];

static struct Settings_st s_settings;
  
enum Settings_en {
  ALARMS_KEY = 0,
  SNOOZEDELAY_KEY = 1,
  DYNAMICSNOOZE_KEY = 2,
  SMARTALARM_KEY = 3,
  MONITORPERIOD_KEY = 4,
  ALARMSON_KEY = 5
};

static void gen_info_str() {
  
  bool all_off = true;
  struct tm *t;
  time_t temp;
  int next = 0;
  char day_str[4];
  char time_str[8];
  
  for (int i = 0; i <= 6; i++) {
    if (s_alarms[i].enabled) {
      all_off = false;
      break;
    }
  }
  
  if (all_off) {
    strncpy(s_info, "NO ALARMS SET", sizeof(s_info));
  } else {
  // Get current time
    temp = time(NULL);
    t = localtime(&temp);
    
    for (int d = t->tm_wday; d <= (t->tm_wday + 7); d++) {
      next = d % 7;
      if (s_alarms[next].enabled && ((s_alarms[next].hour > t->tm_hour) || 
                                  (s_alarms[next].hour == t->tm_hour && s_alarms[next].minute > t->tm_min))) break;
    } 
    
    daynameshort(next, day_str, sizeof(day_str));
    gen_alarm_str(&s_alarms[next], time_str, sizeof(time_str));
    snprintf(s_info, sizeof(s_info), "Next Alarm: %s %s", day_str, time_str);
  }
}

static void info_update_proc() {
  gen_info_str();
  update_info(s_info);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Snooze
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_alarms_on = !s_alarms_on;
  update_onoff(s_alarms_on);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  show_settings(s_alarms, &s_settings, info_update_proc);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void init(void) {
  if (persist_exists(ALARMS_KEY))
    persist_read_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  if (persist_exists(SNOOZEDELAY_KEY))
    s_settings.snooze_delay = persist_read_int(SNOOZEDELAY_KEY);
  else
    s_settings.snooze_delay = 9;
  if (persist_exists(DYNAMICSNOOZE_KEY))
    s_settings.dynamic_snooze = persist_read_int(DYNAMICSNOOZE_KEY) == 1 ? true : false;
  else
    s_settings.dynamic_snooze = true;
  if (persist_exists(SMARTALARM_KEY))
    s_settings.smart_alarm = persist_read_int(SMARTALARM_KEY) == 1 ? true : false;
  else
    s_settings.smart_alarm = true;
  if (persist_exists(MONITORPERIOD_KEY))
    s_settings.monitor_period = persist_read_int(MONITORPERIOD_KEY);
  else
    s_settings.monitor_period = 30;
  if (persist_exists(ALARMSON_KEY))
    s_alarms_on = persist_read_int(ALARMSON_KEY) == 1 ? true : false;
  
  show_mainwin();
  init_click_events(click_config_provider);
  update_onoff(s_alarms_on);
  info_update_proc();
}

static void deinit(void) {
  persist_write_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  persist_write_int(SNOOZEDELAY_KEY, s_settings.snooze_delay);
  persist_write_int(DYNAMICSNOOZE_KEY, s_settings.dynamic_snooze ? 1 : 0);
  persist_write_int(SMARTALARM_KEY, s_settings.smart_alarm ? 1 : 0);
  persist_write_int(MONITORPERIOD_KEY, s_settings.monitor_period);
  persist_write_int(ALARMSON_KEY, s_alarms_on ? 1 : 0);
  
  hide_mainwin();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}