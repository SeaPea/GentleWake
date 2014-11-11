#include <pebble.h>
#include "mainwin.h"
#include "settings.h"
#include "common.h"

// Main program unit
  
//#define DEBUG
  
#define WAKEUP_REASON_ALARM 0
#define WAKEUP_REASON_SNOOZE 1
#define WAKEUP_REASON_MONITOR 2
#define MOVEMENT_THRESHOLD 20000
#define REST_MOVEMENT 300
  
static bool s_alarms_on = true;
static alarm s_alarms[7];
static char s_info[25];
static WakeupId s_wakeup_id = 0;
static bool s_snoozing = false;
static int s_snooze_count = 0;
static bool s_alarm_active = false;
static bool s_monitoring = false;
static int s_last_reset_day = -1;
static int s_vibe_count = 0;
static uint64_t s_last_easylight = 0;
static bool s_accel_service_sub = false;
static int s_next_alarm = -1;
static bool s_light_shown = false;
static int s_last_x;
static int s_last_y;
static int s_last_z;
static int s_movement = 0;

// Vibrate alarm paterns - 2nd dimension: [next vibe delay (sec), vibe segment index, vibe segment length]
static int vibe_patterns[24][3] = {{3, 0, 1}, {3, 0, 1}, {4, 1, 3}, {4, 1, 3}, {4, 2, 5}, {4, 2, 5}, 
                                   {3, 3, 1}, {3, 3, 1}, {4, 4, 3}, {4, 4, 3}, {5, 5, 5}, {5, 5, 5}, 
                                   {3, 6, 1}, {3, 6, 1}, {4, 7, 3}, {4, 7, 3}, {5, 8, 5}, {5, 8, 5}};
static const uint32_t vibe_segments[12][5] = {{150, 0, 0, 0, 0}, {150, 500, 150, 0, 0}, {150, 500, 150, 500, 150},
                                              {300, 0, 0, 0, 0}, {300, 500, 300, 0, 0}, {300, 500, 300, 500, 300},
                                              {600, 0, 0, 0, 0}, {600, 500, 600, 0, 0}, {600, 500, 600, 500, 600}};
static AppTimer *s_vibe_timer = NULL;

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
  LASTRESETDAY_KEY = 8,
  EASYLIGHT_KEY = 9
};

// Calculate which daily alarm (if any) will be next
// (Takes into account if the alarm for today was reset like when Smart Alarm is active and turned off
//  before the alarm time)
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

// Generates the text to show what alarm is next
static void gen_info_str(int next_alarm) {
  
  char day_str[4];
  char time_str[8];
  
  if (next_alarm == -1) {
    strncpy(s_info, "NO ALARMS SET", sizeof(s_info));
  } else {    
    daynameshort(next_alarm, day_str, sizeof(day_str));
    gen_alarm_str(&s_alarms[next_alarm], time_str, sizeof(time_str));
    snprintf(s_info, sizeof(s_info), "Next Alarm:\n%s %s", day_str, time_str);
  }
}

// Timer handler that sets the wakeup time after a short delay
// (allows UI to refresh beforehand since this sometimes takes a second or 2 for some reason)
static void set_wakeup_delayed(void *data) {
  int next_alarm = s_next_alarm;
  
  // Clear any previous wakeup time
  if (s_wakeup_id != 0) {
    wakeup_cancel_all();
    s_wakeup_id = 0;
  }
  
  if (next_alarm != -1) {
    // If there is a next alarm or snooze alarm (next_alarm == -2 when setting snooze wakeup)
    
    if (s_snoozing || s_alarm_active) {
      // if snoozing set a wakeup for the snooze period
      // (or even if the alarm is active set a snooze wakeup in case something happens during the alarm)
      
      int snooze_period = 0;
      
      if (s_settings.dynamic_snooze && s_snooze_count > 0) {
        // Shrink snooze time based on snooze count (down to a min. of 3 minutes)
        snooze_period = (s_settings.snooze_delay * 60) / s_snooze_count;
        if (snooze_period < 180) snooze_period = 180;
      } else {
        // Set normal snooze period
        snooze_period = s_settings.snooze_delay * 60;
      }
      
      time_t snooze_until = time(NULL) + snooze_period;
      
      // Show the snooze wakeup time
      if (s_snoozing) show_snooze(snooze_until);
      
      // Schedule the wakeup
      s_wakeup_id = wakeup_schedule(snooze_until, WAKEUP_REASON_SNOOZE, true);
    } else {
      // Set wakeup for next alarm
      struct tm *t;
      time_t curr_time;
      
      // Get current time
      curr_time = time(NULL);
      t = localtime(&curr_time);
      
      WeekDay alarmday;
      if (next_alarm == t->tm_wday && (s_alarms[next_alarm].hour > t->tm_hour || 
                                       (s_alarms[next_alarm].hour == t->tm_hour && 
                                        s_alarms[next_alarm].minute > t->tm_min)))
        // If next alarm is today, use the TODAY enum
        alarmday = TODAY;
      else
        // Else convert the day number to the WeekDay enum
        alarmday = ad2wd(next_alarm);
      
      // Get the time for the next alarm
      time_t alarm_time = clock_to_timestamp(alarmday, s_alarms[next_alarm].hour, s_alarms[next_alarm].minute);
      
      // If the smart alarm is on but not active, set the wakeup to the alarm time minus the monitor period
      if (s_settings.smart_alarm && !s_monitoring) 
        alarm_time -= s_settings.monitor_period == 0 ? 300 : s_settings.monitor_period * 60;
      
      // If the alarm time is in the past (setting a smart alarm for just a few minutes ahead for example)
      // then adjust it to be 2 seconds in the future
      if (alarm_time < curr_time) alarm_time = curr_time + 2;
      
      // Schedule the wakeup
      s_wakeup_id = wakeup_schedule(alarm_time, 
                                    s_settings.smart_alarm ? WAKEUP_REASON_MONITOR : WAKEUP_REASON_ALARM, true);
    }
  }
  
  // Always make sure wakeup ID is saved immediately
  persist_write_int(WAKEUPID_KEY, s_wakeup_id);
}

// Sets an app wakeup for the specified alarm day
// (next_alarm == -1 means no alarms, next_alarm == -2 means snooze wakeup)
static void set_wakeup(int next_alarm) {
  s_next_alarm = next_alarm;
  // Delay actually setting the wakeup so the UI can update since it takes a second or 2 sometimes
  app_timer_register(100, set_wakeup_delayed, NULL);
}

// Turns off an active alarm or cancels a snooze and sets wakeup for next alarm
static void reset_alarm() {
  struct tm *t;
  time_t temp;
  int next;
  
  // Get current time
  temp = time(NULL);
  t = localtime(&temp);
  
  s_alarm_active = false;
  s_snoozing = false;
  s_monitoring = false;
  s_snooze_count = 0;
  persist_write_int(SNOOZECOUNT_KEY, s_snooze_count);
  s_last_reset_day = t->tm_wday;
  // Save last reset immediately in case something goes wrong
  persist_write_int(LASTRESETDAY_KEY, s_last_reset_day);
  
  // Update UI with next alarm details
  show_alarm_ui(false);
  next = get_next_alarm();
  gen_info_str(next);
  update_info(s_info);
  
  // Set the next alarm wakeup
  set_wakeup(get_next_alarm());
}

// Snoozes active alarm
static void snooze_alarm() {
  s_snoozing = true;
  s_snooze_count++;
  // Save snooze count in case app closes while snoozing
  persist_write_int(SNOOZECOUNT_KEY, s_snooze_count);
  // Set snooze wakeup
  set_wakeup(-2);
}

static void vibe_alarm();

// Timer event to start another vibrate segment
static void handle_vibe_timer(void *data) {
  s_vibe_timer = NULL;
  vibe_alarm();
}

// Activate vibration for the alarm
static void vibe_alarm() {
  if (s_vibe_timer) {
    app_timer_cancel(s_vibe_timer);
    s_vibe_timer = NULL;
  }
  
  if (s_alarm_active && ! s_snoozing) {
    // If still active and not snoozing
    
    if (s_vibe_count >= (int)(sizeof(vibe_patterns) / sizeof(vibe_patterns[0]))) {
      // If we've reach the end of the vibrate patterns...
      
      if (((s_settings.dynamic_snooze ? 3 : s_settings.snooze_delay) * s_snooze_count) > 60)
        // Reset alarm if snoozed for more than 1 hour
        reset_alarm();
      else
        // Auto-snooze if not turned off
        snooze_alarm();
    } else {
      // Make increasingly long vibrate patterns for the alarm
      
      // Setup timer event for next vibe using pattern array
      if (s_vibe_count < (int)(sizeof(vibe_patterns) / sizeof(vibe_patterns[0]))) {
        s_vibe_timer = app_timer_register(vibe_patterns[s_vibe_count][0]*1000, handle_vibe_timer, NULL);
      }
      
      // Start current vibe using pattern and segment arrays
      VibePattern pat;
      pat.durations = vibe_segments[vibe_patterns[s_vibe_count][1]];
      pat.num_segments = vibe_patterns[s_vibe_count][2];
      vibes_enqueue_custom_pattern(pat);
      
      s_vibe_count++;
    }
  }
}

// Callback function to indicate when the settings have been closed
// so that various items can be updated
static void settings_update() {
  // Reset the last reset day in case alarms were changed
  s_last_reset_day = -1;
  // Save last reset immediately in case something goes wrong
  persist_write_int(LASTRESETDAY_KEY, s_last_reset_day);
  
  // Update next alarm info
  int next_alarm = get_next_alarm();
  gen_info_str(next_alarm);
  update_info(s_info);
  
  // Set next wakeup in case alarms were changed
  set_wakeup(next_alarm);
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_snoozing && !s_monitoring) {
    // Disable Back button click when snoozing or monitoring sleep so we don't accidentally exit
    
    if (s_alarm_active) {
      // Any button single click snoozes when alarm active
      snooze_alarm();
    } else {
      // If alarm not active, not snoozing, and not monitoring, exit app
      hide_mainwin();
    }
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_snoozing && !s_monitoring) {
    // Disable Up button when snoozing or monitoring
    
    if (s_alarm_active) {
      // Any button single click snoozes when alarm active
      snooze_alarm();
    } else {
      // Turn all alarms on or off
      s_alarms_on = !s_alarms_on;
      update_onoff(s_alarms_on);
      set_wakeup(s_alarms_on ? get_next_alarm() : -1);
    }
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Single click snoozes when alarm active, Select button single click is disabled otherwise
  if (s_alarm_active && !s_snoozing && !s_monitoring) snooze_alarm();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_snoozing && !s_monitoring) {
    // Disable Up button when snoozing or monitoring
    
    if (s_alarm_active)
      // Any button single click snoozes when alarm active
      snooze_alarm();
    else
      // Show settings screen with current alarms and setings and a callback for when closed
      show_settings(s_alarms, &s_settings, settings_update);
  }
}

static void multiclick_handler(ClickRecognizerRef recognizer, void *context) {
  // If alarm is active (or snoozing) or smart alarm is active, double clicking
  // any button will reset the alarm
  if (s_alarm_active || s_monitoring) reset_alarm();
}

// Trap single and double clicks for ALL buttons
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_BACK, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_UP, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 2, 300, true, multiclick_handler);
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 2, 300, true, multiclick_handler);
}

// Start the alarm, including vibrating the Pebble
static void start_alarm() {
  s_alarm_active = true;
  s_snoozing = false;
  s_monitoring = false;
  show_alarm_ui(true);
  // Set snooze wakeup in case app is closed with the alarm vibrating
  set_wakeup(-2);

  // Start alarm vibrate
  s_vibe_count = 0;
  vibe_alarm();
}

// Timer event to unsubscribe the accelerometer service after a delay
// (without the delay it could be called during the service callback, which crashes the app)
static void unsub_accel_delay(void *data) {
  accel_data_service_unsubscribe();
  s_accel_service_sub = false;
}

// Handle accelerometer data while smart alarm is active or alarm is active/snoozing
// to detect stirring or lifting watch for Easy Light respectively
static void accel_handler(AccelData *data, uint32_t num_samples) {
  if (s_alarm_active || s_snoozing || s_monitoring) {
    if (s_alarm_active || s_snoozing) {
      for (int i = 0; i < (int)num_samples; i++) {
        // If watch screen is held vertically (as if looking at the time) while alarm is on or snoozing,
        // turn the light on for a few seconds
        if (!data[i].did_vibrate) {
          if ((data[i].x > -1250 && data[i].x < -750) || (data[i].x > 750 && data[i].x < 1250) ||
              (data[i].y > -1250 && data[i].y < -750)) {
            if (!s_light_shown && (data[i].timestamp - s_last_easylight) > 3000) {
              // Record when light was last shown and has been shown in this position
              // so it doesn't keep coming on
              s_last_easylight = data[i].timestamp;
              s_light_shown = true;
              light_enable_interaction();
              break;
            } 
          } else {
            // When watch is lowered, reset this flag
            s_light_shown = false;
          }
        }
      }
    } else {
      // Smart Alarm is active, so check for an accumultaive amount of movement, which may indicate stirring
      
      // Initialize last x, y, z readings
      if (s_last_x == 0) s_last_x = data[0].x;
      if (s_last_y == 0) s_last_y = data[0].y;
      if (s_last_z == 0) s_last_z = data[0].z;
      
      int diff;
      
      // Get the accel difference for each direction for the last sample period and as positive values
      // and add to the movement counter
      for (int i = 0; i < (int)num_samples; i++) {
        if (!data[i].did_vibrate) {
          diff = s_last_x - data[i].x;
          s_movement += (diff > 0 ? diff : -diff);
          diff = s_last_y - data[i].y;
          s_movement += (diff > 0 ? diff : -diff);
          diff = s_last_z - data[i].z;
          s_movement += (diff > 0 ? diff : -diff);
          s_last_x = data[i].x;
          s_last_y = data[i].y;
          s_last_z = data[i].z;
        }
      }
      
      // At rest, movement value can accumulate by about 200, so subtract X on every call so
      // that sustained movement is required to trigger the alarm
      s_movement -= REST_MOVEMENT;
      
      if (s_movement < 0)
        // Movement counter cannot be negative
        s_movement = 0;
      else if (s_movement > MOVEMENT_THRESHOLD)
        // If movement counter is over the threshold, activate alarm
        start_alarm();
      
#ifdef DEBUG
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Movement: %d", s_movement);
#endif
    }
  } else if (s_accel_service_sub) {
    // Stop monitoring for movement if nothing is active
    // (Delayed by 250ms since Pebble doesn't like the accel service being unsubscribed during this call)
    app_timer_register(250, unsub_accel_delay, NULL);
  }
}

// Handler for when the wakeup time occurs
static void wakeup_handler(WakeupId id, int32_t reason) {
  // Reset flags since either normal alarm or smart alarm is now active 
  s_last_reset_day = -1;
  persist_write_int(LASTRESETDAY_KEY, s_last_reset_day);
  s_snoozing = false;
  s_monitoring = false;
  
  if (reason != WAKEUP_REASON_SNOOZE) {
    s_snooze_count = 0;
    // Save zero snooze count immediately in case something goes wrong
    persist_write_int(SNOOZECOUNT_KEY, s_snooze_count);
  }
  
  if (reason == WAKEUP_REASON_MONITOR) {
    // Start monitoring activity for stirring
    s_monitoring = true;
    s_last_x = 0;
    s_last_y = 0;
    s_last_z = 0;
    s_movement = 0;
    // Set wakeup for the actual alarm time in case we're dead to the world or something goes wrong during monitoring
    set_wakeup(get_next_alarm());
    // Show monitoring with actual alarm time (wakeup IDs are just the time)
    show_monitoring((time_t)s_wakeup_id);
  } else {
    // Activate the alarm
    start_alarm();
  }
  
  // Monitor movement for either Smart Alarm or Easy Light
  if (!s_accel_service_sub && (s_monitoring || s_settings.easy_light)) {
    accel_data_service_subscribe(5, accel_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    s_accel_service_sub = true;
  }
}

// Handle clock change events
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Show the current time on the main screen
  if ((units_changed & MINUTE_UNIT) != 0) {
    update_clock();
  }
}

static void init(void) {
  // Load all the settings
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
  if (persist_exists(EASYLIGHT_KEY))
    s_settings.easy_light = persist_read_int(EASYLIGHT_KEY) == 1 ? true : false;
  else
    s_settings.easy_light = true;
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
  if (persist_exists(SNOOZECOUNT_KEY))
    s_snooze_count = persist_read_int(SNOOZECOUNT_KEY);
  
  // Show the main screen and update the UI
  show_mainwin();
  init_click_events(click_config_provider);
  update_onoff(s_alarms_on);
  settings_update();
  
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  wakeup_service_subscribe(wakeup_handler);
  
  if (s_alarms_on) {
    if(launch_reason() == APP_LAUNCH_WAKEUP) {
      // The app was started by a wakeup event
    
      // Get details and handle the event
      WakeupId id = 0;
      int32_t reason = 0;
      wakeup_get_launch_event(&id, &reason);
      wakeup_handler(id, reason);
    } else {
      // Make sure a wakeup event is set if needed
      time_t wakeuptime = 0;
      if (s_wakeup_id == 0 || !wakeup_query(s_wakeup_id, &wakeuptime))
        set_wakeup(get_next_alarm());
    }
  }
}

static void deinit(void) {
  // Save everything that needs saving
  persist_write_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  persist_write_int(SNOOZEDELAY_KEY, s_settings.snooze_delay);
  persist_write_int(DYNAMICSNOOZE_KEY, s_settings.dynamic_snooze ? 1 : 0);
  persist_write_int(EASYLIGHT_KEY, s_settings.easy_light ? 1 : 0);
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