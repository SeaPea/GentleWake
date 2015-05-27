#include <pebble.h>
#include "mainwin.h"
#include "settings.h"
#include "stopinstructions.h"
#include "common.h"
#include "errormsg.h"
#include "mainbuttoninfo.h"

// Main program unit
  
//#define DEBUG
  
#define WAKEUP_REASON_ALARM 0
#define WAKEUP_REASON_SNOOZE 1
#define WAKEUP_REASON_MONITOR 2
#define WAKEUP_REASON_DSTCHECK 4
#define MOVEMENT_THRESHOLD_LOW 10000
#define MOVEMENT_THRESHOLD_MID 15000
#define MOVEMENT_THRESHOLD_HIGH 20000
#define REST_MOVEMENT 300
#define NEXT_ALARM_NONE -1
#define NEXT_ALARM_SNOOZE -2
#define NEXT_ALARM_SKIPWEEK -3
  
static bool s_alarms_on = true;
static alarm s_alarms[7];
static char s_info[25];
static WakeupId s_wakeup_id = 0;
static bool s_snoozing = false;
static int s_snooze_count = 0;
static bool s_alarm_active = false;
static bool s_monitoring = false;
static time_t s_last_reset_day = 0;
static time_t s_skip_until = 0;
static int s_vibe_count = 0;
static uint64_t s_last_easylight = 0;
static bool s_accel_service_sub = false;
static int s_next_alarm = -1;
static bool s_light_shown = false;
static int s_last_x = 0;
static int s_last_y = 0;
static int s_last_z = 0;
static int s_movement = 0;
static bool s_loaded = false;
static bool s_dst_check_started = false;

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
  EASYLIGHT_KEY = 9,
  SNOOZINGON_KEY = 10,
  MONITORINGON_KEY = 11,
  MOVESENSITIVITY_KEY = 12,
  SKIPNEXT_KEY = 13,
  DSTCHECKDAY_KEY = 14,
  DSTCHECKHOUR_KEY = 15,
  LASTRESETDATE_KEY = 16,
  SKIPUNTIL_KEY = 17
};

// Strip time component from a timestamp (leaving the date part)
static time_t strip_time(time_t timestamp) {
  return timestamp - (timestamp % (60*60*24));
}

// Calculates the number of days (not 24 hour periods) between 2 dates where date1 is older
// than date2 (if date2 is older a negative number will be returned)
static int64_t day_diff(time_t date1, time_t date2) {
  return ((strip_time(date2) - strip_time(date1)) / (60*60*24));
}

// Gets the UTC offset of the local time in seconds 
// (pass in an existing localtime struct tm to save creating another one, orelse pass NULL)
static time_t get_UTC_offset(struct tm *t) {
#ifdef PBL_PLATFORM_BASALT
  if (t == NULL) {
    time_t temp;
    temp = time(NULL);
    t = localtime(&temp);
  }
  
  //return t->tm_gmtoff + ((t->tm_isdst > 1) ? 3600 : 0);
  return 0;
#else
  // Aplite uses localtime instead of UTC for all time functions so always return 0
  return 0; 
#endif 
}

// Calculate which daily alarm (if any) will be next
// (Takes into account if the alarm for today was reset like when Smart Alarm is active and turned off
//  before the alarm time)
static int get_next_alarm() {
  struct tm *t;
  time_t temp;
  time_t next_date;
  int next;
  
  // Get current time
  temp = time(NULL);
  t = localtime(&temp);
  
  if (day_diff(temp, s_skip_until) >= 7) return NEXT_ALARM_SKIPWEEK;
  
  for (int d = t->tm_wday + (strip_time(temp) == s_last_reset_day ? 1 : 0); d <= (t->tm_wday + 7); d++) {
    next = d % 7;
    if (s_alarms[next].enabled && (d > t->tm_wday || s_alarms[next].hour > t->tm_hour || 
                                   (s_alarms[next].hour == t->tm_hour && s_alarms[next].minute > t->tm_min))) {
      next_date = clock_to_timestamp(ad2wd(next), 0, 0) + get_UTC_offset(t); 
      
      if (next_date >= s_skip_until)
        return next;
    }
  }
  
  return NEXT_ALARM_NONE;
}

// Calculates the skip until date for just skipping the next alarm
static time_t calc_skipnext() {
  int next_alarm = get_next_alarm();
  
  switch (next_alarm) {
    case NEXT_ALARM_NONE:
      return 0;
    case NEXT_ALARM_SKIPWEEK:
      return s_skip_until;
    default:
      return strip_time(clock_to_timestamp(ad2wd(next_alarm), 0, 0) + get_UTC_offset(NULL)) + (60*60*24);
  }
}

// Generates the text to show what alarm is next
static void gen_info_str(int next_alarm) {
  
  char day_str[4];
  char time_str[8];
  
  if (next_alarm == NEXT_ALARM_NONE) {
    strncpy(s_info, "NO ALARMS SET", sizeof(s_info));
  } else if (next_alarm == NEXT_ALARM_SKIPWEEK) {
    struct tm *skip_until = localtime(&s_skip_until);
    strftime(s_info, sizeof(s_info), "Skip Until:%n%a, %b %d", skip_until);
  } else {    
    daynameshort(next_alarm, day_str, sizeof(day_str));
    gen_alarm_str(&s_alarms[next_alarm], time_str, sizeof(time_str));
    snprintf(s_info, sizeof(s_info), "Next Alarm:\n%s %s", day_str, time_str);
  }
}

// Updates the displayed alarm time (and returns the next alarm day value)
static int update_alarm_display() {
  int next_alarm = get_next_alarm();
  gen_info_str(next_alarm);
  update_info(s_info);
  return next_alarm;
}

/*
static void test_mktime() {
  
  #ifdef PBL_PLATFORM_BASALT
  struct tm *t;
  time_t temp;
  // Get current time
  temp = time(NULL);
  t = localtime(&temp);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "GMT Offset: %d, Is DST: %d", t->tm_gmtoff, t->tm_isdst);
  #endif
  
  struct tm before_dst_tm;
  
  // Oct 31 2015 7:00am (Before DST change)
  before_dst_tm.tm_sec = 0;
  before_dst_tm.tm_min = 0;
  before_dst_tm.tm_hour = 6;
  before_dst_tm.tm_mday = 31;
  before_dst_tm.tm_mon = 9;
  before_dst_tm.tm_year = 115;
  before_dst_tm.tm_isdst = 1;
#ifdef PBL_PLATFORM_BASALT
  strcpy(before_dst_tm.tm_zone, "EDT");
  before_dst_tm.tm_gmtoff = -5*60*60;
#endif
  
  char b4_dst_str[30];
  time_t b4_dst = mktime(&before_dst_tm);
  strftime(b4_dst_str, 30, "%c %Z", &before_dst_tm);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "b4_dst: %ld", b4_dst);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "b4_dst_str: %s", b4_dst_str);
  
  struct tm after_dst_tm;
  
  // Nov 1 2015 7:00am (After DST change)
  after_dst_tm.tm_year = 115;
  after_dst_tm.tm_mon = 10;
  after_dst_tm.tm_mday = 1;
  after_dst_tm.tm_hour = 6;
  after_dst_tm.tm_min = 0;
  after_dst_tm.tm_sec = 0;
  after_dst_tm.tm_isdst = 1;
#ifdef PBL_PLATFORM_BASALT
  after_dst_tm.tm_gmtoff = -5*60*60;
#endif
  
  time_t aft_dst = mktime(&after_dst_tm);
  char aft_dst_str[30];
  after_dst_tm.tm_isdst = -1;
  strftime(aft_dst_str, 30, "%c %Z", &after_dst_tm); 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "aft_dst: %ld", aft_dst);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "aft_dst_str: %s", aft_dst_str);
  
}
*/

// Timer handler that sets the wakeup time after a short delay
// (allows UI to refresh beforehand since this sometimes takes a second or 2 for some reason)
static void set_wakeup_delayed(void *data) {
  int try_count = 0;
  int diff = 0;
  int next_alarm = s_next_alarm;
  
  // Clear any previous wakeup time
  if (s_wakeup_id != 0) {
    wakeup_cancel_all();
    s_wakeup_id = 0;
  }
  
  if (next_alarm != NEXT_ALARM_NONE) {
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
      
      if (s_wakeup_id < 0) {
        // If ID is less than zero something went wrong, so clear all wakeups and try again
        wakeup_cancel_all();
        s_wakeup_id = wakeup_schedule(snooze_until, WAKEUP_REASON_SNOOZE, true);
        
        if (s_wakeup_id < 0) {
          // If ID is still negative, show error message
          if (s_wakeup_id == E_RANGE)
            show_errormsg("Unable to set the snooze alarm due to another app alarm.");
          else {
            char msg[150];
            snprintf(msg, sizeof(msg),
                     "Unknown error while setting snooze alarm (%d). A factory reset may be required if this happens again",
                     (int)s_wakeup_id);
            show_errormsg(msg);
          }
        }
      }
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
      time_t alarm_time = clock_to_timestamp(alarmday, s_alarms[next_alarm].hour, s_alarms[next_alarm].minute) +
        get_UTC_offset(t);
      // Strip seconds
      alarm_time -= alarm_time % 60;
      
      // If the smart alarm is on but not active, set the wakeup to the alarm time minus the monitor period
      if (s_settings.smart_alarm && !s_monitoring) 
        alarm_time -= s_settings.monitor_period == 0 ? 300 : s_settings.monitor_period * 60;
      
      // If the alarm time is in the past (setting a smart alarm for just a few minutes ahead for example)
      // then adjust it to be 5 seconds in the future
      if (alarm_time < curr_time) alarm_time = curr_time + 5;
      
      int wakeup_reason = (s_settings.smart_alarm && !s_monitoring) ? WAKEUP_REASON_MONITOR : WAKEUP_REASON_ALARM;
      
      // Schedule the wakeup
      s_wakeup_id = wakeup_schedule(alarm_time, wakeup_reason, true);
      
      if (s_wakeup_id < 0) {
        // If ID is negative, something went wrong, so make sure all wakeups for this app are cancelled and 
        // try again
        wakeup_cancel_all();
        s_wakeup_id = wakeup_schedule(alarm_time, wakeup_reason, true);
        
        if (s_wakeup_id == E_RANGE) {
          // E_RANGE means some other app has the same wakeup time +/- 1 minute, so try to set
          // the wakeup for an earlier/later time up to 5 minutes before/after.
          
          if (alarm_time < curr_time + 360)
            // If alarm time less than 6 minutes in the future, try for later time
            diff = 60;
          else
            // else try for earlier time
            diff = -60;
          
          // Adjust wakeup in 1 minute decrements up to 5 minutes before/after
          while (s_wakeup_id == E_RANGE && try_count++ < 5) {
            alarm_time += diff;
            
            s_wakeup_id = wakeup_schedule(alarm_time, wakeup_reason, true);
          }
        }
        
        if (s_wakeup_id < 0) {
          char msg[150];
          
          // If ID is still negative, show error message
          if (s_wakeup_id == E_RANGE) {
            char daystr[10];
            char timestr[8];
            
            dayname(next_alarm, daystr, sizeof(daystr));
            gen_time_str(t->tm_hour, t->tm_min, timestr, sizeof(timestr));
            snprintf(msg, sizeof(msg), 
                     "Another wakeup is set for %s at %s +/-5 minutes in another app. Please either change the alarm time here or the other app.", 
                     daystr, timestr);
            
            show_errormsg(msg);
          } else {
            snprintf(msg, sizeof(msg),
                     "Unknown error while setting alarm (%d). A factory reset may be required if this happens again",
                     (int)s_wakeup_id);
            show_errormsg(msg);
          }
        }
      }
      
      // If smart alarm monitoring, update display with actual alarm time
      if (s_monitoring) show_monitoring(alarm_time);
    }
    
    if (s_settings.dst_check_day != 0) {
      // If DST check is on, set a wakeup for redoing alarms in case of a daylight savings time change
      WakeupId check_wakeup_id = 0;
      time_t check_time = clock_to_timestamp(s_settings.dst_check_day, s_settings.dst_check_hour, 0) +
        get_UTC_offset(NULL);
      check_time -= check_time % 60;
      check_wakeup_id = wakeup_schedule(check_time, WAKEUP_REASON_DSTCHECK, false);
      
      if (check_wakeup_id == E_RANGE) {
        // If another app has a wakeup at the same time, try up to 10 minutes after the check time in 
        // 1 minute increments
        try_count = 0;
        diff = 60;
        
        while (check_wakeup_id == E_RANGE && try_count++ < 10) {
          check_time += diff;
          check_wakeup_id = wakeup_schedule(check_time, WAKEUP_REASON_DSTCHECK, false);
        }
      }
    }
  }
  
  // Always make sure wakeup ID is saved immediately
  persist_write_int(WAKEUPID_KEY, s_wakeup_id);
  
  // If app was started for a DST check, close the app now that the wakeups have been redone.
  if (s_dst_check_started)
    window_stack_pop_all(false);
}

// Sets an app wakeup for the specified alarm day
// (next_alarm == -1 means no alarms, next_alarm == -2 means snooze wakeup)
static void set_wakeup(int next_alarm) {
  s_next_alarm = next_alarm;
  // Delay actually setting the wakeup so the UI can update since it takes a second or 2 sometimes
  app_timer_register(250, set_wakeup_delayed, NULL);
}

// Updates global snoozing flag and saves it in case of an exit
static void set_snoozing(bool snoozing) {
  s_snoozing = snoozing;
  persist_write_bool(SNOOZINGON_KEY, s_snoozing);
}

// Updates snooze count and saves it in case of an exit
static void set_snoozecount(int snooze_count) {
  s_snooze_count = snooze_count;
  persist_write_int(SNOOZECOUNT_KEY, s_snooze_count);
}

// Updates global monitoring flag and saves it in case of an exit
static void set_monitoring(bool monitoring) {
  s_monitoring = monitoring;
  persist_write_bool(MONITORINGON_KEY, s_monitoring);
}

// Updates last reset day and saves it in case of an exit
static void set_lastresetday(time_t last_reset_day) {
  s_last_reset_day = strip_time(last_reset_day);
  persist_write_data(LASTRESETDATE_KEY, &s_last_reset_day, sizeof(s_last_reset_day));
}

// Updates flag for skipping next alarm and saves it in case of an exit
static void set_skipuntil(time_t skip_until) {
  s_skip_until = skip_until;
  persist_write_data(SKIPUNTIL_KEY, &s_skip_until, sizeof(s_skip_until));
}

// Turns off an active alarm or cancels a snooze and sets wakeup for next alarm
static void reset_alarm() {
  int next;
  
  s_alarm_active = false;
  set_snoozing(false);
  set_monitoring(false);
  set_snoozecount(0);
  set_lastresetday(time(NULL));
  
  // Update UI with next alarm details
  show_alarm_ui(false);
  next = update_alarm_display();
  
  // Set the next alarm wakeup
  set_wakeup(next);
}

// Snoozes active alarm
static void snooze_alarm() {
  set_snoozing(true);
  set_snoozecount(s_snooze_count + 1);
  
  // Set snooze wakeup
  set_wakeup(NEXT_ALARM_SNOOZE);
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

static void save_settings_delayed(void *data) {
  // Save all settings
  persist_write_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  persist_write_int(SNOOZEDELAY_KEY, s_settings.snooze_delay);
  persist_write_bool(DYNAMICSNOOZE_KEY, s_settings.dynamic_snooze);
  persist_write_bool(EASYLIGHT_KEY, s_settings.easy_light);
  persist_write_bool(SMARTALARM_KEY, s_settings.smart_alarm);
  persist_write_int(MONITORPERIOD_KEY, s_settings.monitor_period);
  persist_write_int(MOVESENSITIVITY_KEY, s_settings.sensitivity);
  persist_write_int(DSTCHECKDAY_KEY, s_settings.dst_check_day);
  persist_write_int(DSTCHECKHOUR_KEY, s_settings.dst_check_hour);
}

// Callback function to indicate when the settings have been closed
// so that various items can be updated
static void settings_update() {
  if (s_loaded) {
    // Reset the last reset day in case alarms were changed
    set_lastresetday(0);
    // Reset skip next too
    set_skipuntil(0);
    
    // Save settings after a delay to allow UI to update
    app_timer_register(500, save_settings_delayed, NULL);
  }
  
  // Update next alarm info
  int next_alarm = update_alarm_display();
  
  // Set next wakeup in case alarms were changed
  if (s_loaded) set_wakeup(next_alarm);
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
  } else {
    show_stopinstructions();
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
      persist_write_bool(ALARMSON_KEY, s_alarms_on);
      update_onoff(s_alarms_on);
      // Reset skip
      set_skipuntil(0);
      int next_alarm = update_alarm_display();
      // Redo wakeup
      set_wakeup(s_alarms_on ? next_alarm : -1);
    }
  } else {
    show_stopinstructions();
  }
}

static void up_longclick_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_alarm_active && !s_snoozing && !s_monitoring) {
    // Disable long Up button press when alarm active, snoozing, or monitoring
    
    if (day_diff(time(NULL), s_skip_until) < 7) {
      // Repeatedly calculating the next alarm skip date, skips each subsequent alarm
      set_skipuntil(calc_skipnext());
      // Limit skips to 1 week into the future for now as more than 1 week will require
      // some tweaking to the wakeup code
      if (day_diff(time(NULL), s_skip_until) >= 7)
        set_skipuntil(0);
    } else
      set_skipuntil(0);
    
    vibes_short_pulse();
    // Update alarm display
    int next_alarm = update_alarm_display();
    // Update wakeup
    set_wakeup(next_alarm);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Single click snoozes when alarm active, Select button single click is disabled otherwise
  if (!s_snoozing && !s_monitoring) {
    if (s_alarm_active) 
      snooze_alarm();
    else
      show_mainbuttoninfo();
  } else {
    show_stopinstructions();
  }
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
  } else {
    show_stopinstructions();
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
  window_long_click_subscribe(BUTTON_ID_UP, 2000, up_longclick_handler, NULL);
}

// Start the alarm, including vibrating the Pebble
static void start_alarm() {
  s_alarm_active = true;
  set_snoozing(false);
  set_monitoring(false);
  show_alarm_ui(true);
  // Set snooze wakeup in case app is closed with the alarm vibrating
  set_wakeup(NEXT_ALARM_SNOOZE);

  // Start alarm vibrate
  s_vibe_count = 0;
  vibe_alarm();
}

// Timer event to unsubscribe the accelerometer service after a delay
// (without the delay it could be called during the service callback, which crashes the app)
static void unsub_accel_delay(void *data) {
  if (s_accel_service_sub && !s_monitoring && ((!s_alarm_active && !s_snoozing) || !s_settings.easy_light)) {
    accel_data_service_unsubscribe();
    s_accel_service_sub = false;
  }
}

// Handle accelerometer data while smart alarm is active or alarm is active/snoozing
// to detect stirring or lifting watch for Easy Light respectively
static void accel_handler(AccelData *data, uint32_t num_samples) {
  if (s_alarm_active || s_snoozing || s_monitoring) {
    if (s_alarm_active || s_snoozing) {
      if (s_settings.easy_light) {
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
      else if ((s_settings.sensitivity == MS_LOW && s_movement > MOVEMENT_THRESHOLD_HIGH) ||
               (s_settings.sensitivity == MS_MEDIUM && s_movement > MOVEMENT_THRESHOLD_MID) ||
               (s_settings.sensitivity == MS_HIGH && s_movement > MOVEMENT_THRESHOLD_LOW))
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

// Start monitoring the accelerometer for either the Smart Alarm or Easy Light
static void start_accel() {
  if (!s_accel_service_sub) {
    accel_data_service_subscribe(5, accel_handler);
    accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
    s_accel_service_sub = true;
  }
}

// Handler for when the wakeup time occurs
static void wakeup_handler(WakeupId id, int32_t reason) {
  if (reason == WAKEUP_REASON_DSTCHECK) {
    // If wakeup was for Daylight Savings Time check and we're not in the middle
    // of an active alarm or monitoring (which will update the wakeup times anyway), then
    // redo the alarm wakeups
    if (!s_alarm_active && !s_snoozing && !s_monitoring)
      set_wakeup(s_alarms_on ? get_next_alarm() : -1);
  } else {
    // Clear last reset day since either normal alarm or smart alarm is now active 
    set_lastresetday(0);
    // Also reset skip
    set_skipuntil(0);
    
    if (reason != WAKEUP_REASON_SNOOZE) {
      set_snoozecount(0);
    }
    
    if (reason == WAKEUP_REASON_MONITOR) {
      // Start monitoring activity for stirring
      set_monitoring(true);
      s_last_x = 0;
      s_last_y = 0;
      s_last_z = 0;
      s_movement = 0;
      // Set wakeup for the actual alarm time in case we're dead to the world or something goes wrong during monitoring
      set_wakeup(get_next_alarm());
    } else {
      // Activate the alarm
      start_alarm();
    }
    
    // Monitor movement for either Smart Alarm or Easy Light
    if (s_monitoring || s_settings.easy_light) {
      start_accel();
    }
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
  //test_mktime();
  
  // Load all the settings
  if (persist_exists(ALARMS_KEY))
    persist_read_data(ALARMS_KEY, s_alarms, sizeof(s_alarms));
  if (persist_exists(SNOOZEDELAY_KEY))
    s_settings.snooze_delay = persist_read_int(SNOOZEDELAY_KEY);
  else
    s_settings.snooze_delay = 9;
  if (persist_exists(DYNAMICSNOOZE_KEY))
    s_settings.dynamic_snooze = persist_read_bool(DYNAMICSNOOZE_KEY);
  else
    s_settings.dynamic_snooze = true;
  if (persist_exists(EASYLIGHT_KEY))
    s_settings.easy_light = persist_read_bool(EASYLIGHT_KEY);
  else
    s_settings.easy_light = true;
  if (persist_exists(SMARTALARM_KEY))
    s_settings.smart_alarm = persist_read_bool(SMARTALARM_KEY);
  else
    s_settings.smart_alarm = true;
  if (persist_exists(MONITORPERIOD_KEY))
    s_settings.monitor_period = persist_read_int(MONITORPERIOD_KEY);
  else
    s_settings.monitor_period = 30;
  if (persist_exists(MOVESENSITIVITY_KEY))
    s_settings.sensitivity = persist_read_int(MOVESENSITIVITY_KEY);
  else
    s_settings.sensitivity = MS_MEDIUM;
  if (persist_exists(ALARMSON_KEY))
    s_alarms_on = persist_read_bool(ALARMSON_KEY);
  if (persist_exists(WAKEUPID_KEY))
    s_wakeup_id = persist_read_int(WAKEUPID_KEY);
  if (persist_exists(LASTRESETDATE_KEY))
    persist_read_data(LASTRESETDATE_KEY, &s_last_reset_day, sizeof(s_last_reset_day));
  if (persist_exists(SNOOZECOUNT_KEY))
    s_snooze_count = persist_read_int(SNOOZECOUNT_KEY);
  if (persist_exists(SNOOZINGON_KEY))
    s_snoozing = persist_read_bool(SNOOZINGON_KEY);
  if (persist_exists(MONITORINGON_KEY))
    s_monitoring = persist_read_bool(MONITORINGON_KEY);
  if (persist_exists(SKIPNEXT_KEY)) {
    bool skip_next = persist_read_bool(SKIPNEXT_KEY);
    persist_delete(SKIPNEXT_KEY);
    if (skip_next) set_skipuntil(calc_skipnext());
  }
  if (persist_exists(SKIPUNTIL_KEY))
    persist_read_data(SKIPUNTIL_KEY, &s_skip_until, sizeof(s_skip_until));
  if (persist_exists(DSTCHECKDAY_KEY))
    s_settings.dst_check_day = persist_read_int(DSTCHECKDAY_KEY);
  else
    s_settings.dst_check_day = SUNDAY;
  if (persist_exists(DSTCHECKHOUR_KEY)) {
    s_settings.dst_check_hour = persist_read_int(DSTCHECKHOUR_KEY);
    if (s_settings.dst_check_hour < 3 || s_settings.dst_check_hour > 9)
      s_settings.dst_check_hour = 4;
  } else
    s_settings.dst_check_hour = 4;
  
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
      
      // It app was started for a DST check, exit once the wakeups have been redone (happens on a timer)
      if (reason == WAKEUP_REASON_DSTCHECK) s_dst_check_started = true;
      
      wakeup_handler(id, reason);
       
    } else {
      // Make sure a wakeup event is set if needed
      time_t wakeuptime = 0;
      if (s_wakeup_id == 0 || !wakeup_query(s_wakeup_id, &wakeuptime))
        set_wakeup(get_next_alarm());
      else if (s_wakeup_id != 0) {
        // Else if recovering from a crash or forced exit, restart any snoozing/monitoring
        if (s_snoozing) {
          s_alarm_active = true;
          show_snooze((time_t)s_wakeup_id);
          if (s_settings.easy_light) start_accel();
        } else if (s_monitoring) {
          show_monitoring((time_t)s_wakeup_id);
          start_accel();
        }
      }
    }
  }
  
  s_loaded = true;
}

static void deinit(void) {
  
  if (s_accel_service_sub) accel_data_service_unsubscribe();
  
  hide_mainwin();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}