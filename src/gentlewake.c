#include <pebble.h>
#include "mainwin.h"
#include "settings.h"
#include "common.h"

#define WAKEUP_REASON_ALARM 0
#define WAKEUP_REASON_SNOOZE 1
#define WAKEUP_REASON_MONITOR 2
  
static bool s_alarms_on = true;
static alarm s_alarms[7];
static char s_info[25];
static WakeupId s_wakeup_id = 0;
static bool s_snoozing = false;
static int s_snooze_count = 0;
static bool s_alarm_active = false;
static int s_last_reset_day = -1;
static int s_vibe_count = 0;

static struct Settings_st s_settings;
  
enum Settings_en {
  ALARMS_KEY = 0,
  SNOOZEDELAY_KEY = 1,
  DYNAMICSNOOZE_KEY = 2,
  SMARTALARM_KEY = 3,
  MONITORPERIOD_KEY = 4,
  ALARMSON_KEY = 5,
  WAKEUPID_KEY = 6,
  SNOOZECOUNT_KEY = 7,
  LASTRESETDAY_KEY = 8
};

static int get_next_alarm() {
  struct tm *t;
  time_t temp;
  int next;
  
  // Get current time
  temp = time(NULL);
  t = localtime(&temp);
  
  for (int d = t->tm_wday + (t->tm_wday == s_last_reset_day ? 1 : 0); d <= (t->tm_wday + 7); d++) {
    next = d % 7;
    if (s_alarms[next].enabled && (d > t->tm_wday || s_alarms[next].hour > t->tm_hour || 
                                   (s_alarms[next].hour == t->tm_hour && s_alarms[next].minute > t->tm_min)))
      return next;
  }
  
  return -1;
}

static void gen_info_str(int next_alarm) {
  
  char day_str[4];
  char time_str[8];
  
  if (next_alarm == -1) {
    strncpy(s_info, "NO ALARMS SET", sizeof(s_info));
  } else {
    
    daynameshort(next_alarm, day_str, sizeof(day_str));
    gen_alarm_str(&s_alarms[next_alarm], time_str, sizeof(time_str));
    snprintf(s_info, sizeof(s_info), "Next Alarm: %s %s", day_str, time_str);
  }
}

static void set_wakeup(int next_alarm) {
  wakeup_cancel_all();
  s_wakeup_id = 0;
  if (next_alarm != -1) {
    if (s_snoozing || s_alarm_active) {
      int snooze_period = 0;
      
      if (s_settings.dynamic_snooze && s_snooze_count > 0) {
        // Shrink snooze time based on snooze count (down to a min. of 3 minutes)
        snooze_period = (s_settings.snooze_delay * 60) / s_snooze_count;
        if (snooze_period < 180) snooze_period = 180;
      } else {
        snooze_period = s_settings.snooze_delay * 60;
      }
      
      time_t snooze_until = time(NULL) + snooze_period;
      
      s_wakeup_id = wakeup_schedule(snooze_until, WAKEUP_REASON_SNOOZE, true);
    } else {
      // Set wakeup for next alarm
      time_t alarm_time = clock_to_timestamp(ad2wd(next_alarm), s_alarms[next_alarm].hour, s_alarms[next_alarm].minute);
      if (s_settings.smart_alarm) alarm_time -= s_settings.monitor_period == 0 ? 300 : s_settings.monitor_period * 60;
      s_wakeup_id = wakeup_schedule(alarm_time, 
                                    s_settings.smart_alarm ? WAKEUP_REASON_MONITOR : WAKEUP_REASON_ALARM, true);
    }
  }
}

static void vibe_alarm() {
  // TODO: Make increasingly long vibrate patterns for the alarm
  
}

static void reset_alarm() {
  struct tm *t;
  time_t temp;
  
  // Get current time
  temp = time(NULL);
  t = localtime(&temp);
  
  s_alarm_active = false;
  s_snoozing = false;
  s_snooze_count = 0;
  s_last_reset_day = t->tm_wday;
  show_alarm_ui(false);
  set_wakeup(get_next_alarm());
}

static void snooze_alarm() {
  s_snoozing = true;
  s_snooze_count++;
  set_wakeup(-2);
}

static void settings_update() {
  int next_alarm = get_next_alarm();
  gen_info_str(next_alarm);
  update_info(s_info);
  
  set_wakeup(next_alarm);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_alarm_active) {
    snooze_alarm();
  } else {
    hide_mainwin(); // Exit app
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_alarm_active) {
    snooze_alarm();
  } else {
    s_alarms_on = !s_alarms_on;
    update_onoff(s_alarms_on);
    set_wakeup(s_alarms_on ? get_next_alarm() : -1);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_alarm_active) snooze_alarm();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_alarm_active)
    snooze_alarm();
  else
    show_settings(s_alarms, &s_settings, settings_update);
}

static void longclick_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_alarm_active) reset_alarm();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_BACK, 2000, NULL, longclick_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 2000, NULL, longclick_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 2000, NULL, longclick_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 2000, NULL, longclick_handler);
}

static void wakeup_handler(WakeupId id, int32_t reason) {
  // A wakeup event has occurred.
  s_last_reset_day = -1;
  s_snoozing = (reason == WAKEUP_REASON_SNOOZE);
  if (!s_snoozing) s_snooze_count = 0;
  
  if (reason == WAKEUP_REASON_MONITOR) {
    // Start monitoring activity for stirring
    
  } else {
    s_alarm_active = true;
    show_alarm_ui(true);
    // Set snooze wakeup in case app is closed with the alarm vibrating
    set_wakeup(-2);
    
    // Start alarm vibrate
    s_vibe_count = 0;
    vibe_alarm();
  }
}

// Handle clock change events
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if ((units_changed & MINUTE_UNIT) != 0) {
    update_clock();
  }
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
  if (persist_exists(WAKEUPID_KEY))
    s_wakeup_id = persist_read_int(WAKEUPID_KEY);
  if (persist_exists(LASTRESETDAY_KEY))
    s_last_reset_day = persist_read_int(LASTRESETDAY_KEY);
  
  show_mainwin();
  init_click_events(click_config_provider);
  update_onoff(s_alarms_on);
  settings_update();
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  wakeup_service_subscribe(wakeup_handler);
  
  if(launch_reason() == APP_LAUNCH_WAKEUP) {
    // The app was started by a wakeup event
  
    // Get details and handle the event
    WakeupId id = 0;
    int32_t reason = 0;
    wakeup_get_launch_event(&id, &reason);
    wakeup_handler(id, reason);
  }
}

static void deinit(void) {
  persist_write_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  persist_write_int(SNOOZEDELAY_KEY, s_settings.snooze_delay);
  persist_write_int(DYNAMICSNOOZE_KEY, s_settings.dynamic_snooze ? 1 : 0);
  persist_write_int(SMARTALARM_KEY, s_settings.smart_alarm ? 1 : 0);
  persist_write_int(MONITORPERIOD_KEY, s_settings.monitor_period);
  persist_write_int(ALARMSON_KEY, s_alarms_on ? 1 : 0);
  persist_write_int(WAKEUPID_KEY, s_wakeup_id);
  persist_write_int(LASTRESETDAY_KEY, s_last_reset_day);
  
  hide_mainwin();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}